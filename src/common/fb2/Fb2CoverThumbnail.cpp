#include "Fb2CoverThumbnail.h"

#include <windows.h>
#include <objidl.h>
#include <atlbase.h>
#include <atlstr.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <wincodec.h>

namespace {

#pragma comment(lib, "windowscodecs.lib")

const int kMaximumSourceImageDimension = 16384;
const long long kMaximumSourceImagePixels = 64000000LL;

HRESULT CreateStreamFromBytes(const std::vector<unsigned char>& bytes, IStream** stream)
{
    if (stream == nullptr) return E_POINTER;
    *stream = nullptr;
    if (bytes.empty()) return E_INVALIDARG;

    HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (memory == nullptr) return E_OUTOFMEMORY;
    void* buffer = ::GlobalLock(memory);
    if (buffer == nullptr) {
        const HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
        ::GlobalFree(memory);
        return hr;
    }
    std::memcpy(buffer, bytes.data(), bytes.size());
    ::GlobalUnlock(memory);
    const HRESULT hr = ::CreateStreamOnHGlobal(memory, TRUE, stream);
    if (FAILED(hr)) ::GlobalFree(memory);
    return hr;
}

bool HasSafeImageDimensions(IWICBitmapSource* source, UINT& width, UINT& height, ATL::CString* errorMessage)
{
    width = 0; height = 0;
    if (source == nullptr || FAILED(source->GetSize(&width, &height)) || width == 0 || height == 0 ||
        width > kMaximumSourceImageDimension || height > kMaximumSourceImageDimension ||
        static_cast<long long>(width) * height > kMaximumSourceImagePixels) {
        if (errorMessage != nullptr) *errorMessage = L"Cover image dimensions exceed the thumbnail safety limit.";
        return false;
    }
    return true;
}

bool CreateDibFromWicSource(IWICImagingFactory* factory, IWICBitmapSource* source,
    FB2CoverThumbnail::DecodedImage& image, ATL::CString* errorMessage)
{
    if (factory == nullptr || source == nullptr) return false;
    UINT width = 0, height = 0;
    if (!HasSafeImageDimensions(source, width, height, errorMessage)) return false;

    CComPtr<IWICFormatConverter> converter;
    HRESULT hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    }
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"The Windows image stack could not convert the cover: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }

    const UINT stride = width * 4;
    const UINT bufferSize = stride * height;
    std::vector<unsigned char> pixels(bufferSize);
    hr = converter->CopyPixels(nullptr, stride, bufferSize, pixels.data());
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"The Windows image stack could not read cover pixels: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }

    bool hasAlpha = false;
    for (size_t index = 3; index < pixels.size(); index += 4) {
        if (pixels[index] != 0xFF) { hasAlpha = true; break; }
    }

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = -static_cast<LONG>(height); // top-down BGRA DIB
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* dibPixels = nullptr;
    HDC screen = ::GetDC(nullptr);
    HBITMAP bitmap = ::CreateDIBSection(screen, &info, DIB_RGB_COLORS, &dibPixels, nullptr, 0);
    if (screen != nullptr) ::ReleaseDC(nullptr, screen);
    if (bitmap == nullptr || dibPixels == nullptr) {
        if (bitmap != nullptr) ::DeleteObject(bitmap);
        if (errorMessage != nullptr) *errorMessage = L"Failed to create a 32-bit bitmap for thumbnail pixels.";
        return false;
    }
    std::memcpy(dibPixels, pixels.data(), pixels.size());
    image.Reset();
    image.bitmap = bitmap;
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.hasAlpha = hasAlpha;
    return true;
}

void ComputeFitSize(int sourceWidth, int sourceHeight, unsigned int maxEdge, int& targetWidth, int& targetHeight)
{
    targetWidth = 0; targetHeight = 0;
    if (sourceWidth <= 0 || sourceHeight <= 0 || maxEdge == 0) return;
    const int sourceMaxEdge = (std::max)(sourceWidth, sourceHeight);
    if (sourceMaxEdge <= static_cast<int>(maxEdge)) {
        targetWidth = sourceWidth;
        targetHeight = sourceHeight;
        return;
    }
    targetWidth = (std::max)(1, static_cast<int>(static_cast<long long>(sourceWidth) * maxEdge / sourceMaxEdge));
    targetHeight = (std::max)(1, static_cast<int>(static_cast<long long>(sourceHeight) * maxEdge / sourceMaxEdge));
}

} // namespace

namespace FB2CoverThumbnail {

DecodedImage::DecodedImage() : bitmap(nullptr), width(0), height(0), hasAlpha(false) {}
DecodedImage::~DecodedImage() { Reset(); }
void DecodedImage::Reset()
{
    if (bitmap != nullptr) { ::DeleteObject(bitmap); bitmap = nullptr; }
    width = 0; height = 0; hasAlpha = false;
}
bool DecodedImage::IsEmpty() const { return bitmap == nullptr; }

bool TryDecode(const std::vector<unsigned char>& bytes, DecodedImage& image, ATL::CString* errorMessage)
{
    image.Reset();
    if (errorMessage != nullptr) errorMessage->Empty();
    if (bytes.empty()) {
        if (errorMessage != nullptr) *errorMessage = L"Cover image bytes were not provided.";
        return false;
    }
    CComPtr<IStream> stream;
    HRESULT hr = CreateStreamFromBytes(bytes, &stream);
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"Failed to create a stream from cover bytes: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }
    CComPtr<IWICImagingFactory> factory;
    hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        if (errorMessage != nullptr) *errorMessage = L"The Windows image stack could not be initialized.";
        return false;
    }
    CComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
    CComPtr<IWICBitmapFrameDecode> frame;
    if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"The Windows image stack could not decode the cover: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }
    return CreateDibFromWicSource(factory, frame, image, errorMessage);
}

bool TryResizeToFit(const DecodedImage& sourceImage, unsigned int maxEdge, DecodedImage& resizedImage, ATL::CString* errorMessage)
{
    resizedImage.Reset();
    if (errorMessage != nullptr) errorMessage->Empty();
    if (sourceImage.bitmap == nullptr || sourceImage.width <= 0 || sourceImage.height <= 0) {
        if (errorMessage != nullptr) *errorMessage = L"A valid decoded image was not provided for resizing.";
        return false;
    }
    if (maxEdge == 0) {
        if (errorMessage != nullptr) *errorMessage = L"Requested thumbnail size is zero.";
        return false;
    }
    int targetWidth = 0, targetHeight = 0;
    ComputeFitSize(sourceImage.width, sourceImage.height, maxEdge, targetWidth, targetHeight);
    if (targetWidth <= 0 || targetHeight <= 0) {
        if (errorMessage != nullptr) *errorMessage = L"Failed to calculate thumbnail dimensions.";
        return false;
    }
    CComPtr<IWICImagingFactory> factory;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    CComPtr<IWICBitmap> source;
    if (SUCCEEDED(hr)) hr = factory->CreateBitmapFromHBITMAP(sourceImage.bitmap, nullptr, WICBitmapUsePremultipliedAlpha, &source);
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"The Windows image stack could not prepare the cover for resize: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }
    if (targetWidth == sourceImage.width && targetHeight == sourceImage.height)
        return CreateDibFromWicSource(factory, source, resizedImage, errorMessage);

    CComPtr<IWICBitmapScaler> scaler;
    hr = factory->CreateBitmapScaler(&scaler);
    if (SUCCEEDED(hr)) hr = scaler->Initialize(source, targetWidth, targetHeight, WICBitmapInterpolationModeFant);
    if (FAILED(hr)) {
        if (errorMessage != nullptr) errorMessage->Format(L"The Windows image stack could not resize the cover: 0x%08X", static_cast<unsigned int>(hr));
        return false;
    }
    return CreateDibFromWicSource(factory, scaler, resizedImage, errorMessage);
}

} // namespace FB2CoverThumbnail
