#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

#include "..\..\src\fbshell\Fb2ThumbnailProvider.h"

CComModule _Module;

namespace {

bool ExpectTrue(const wchar_t* name, bool value)
{
    if (value)
        return true;

    std::wcerr << L"Проверка условия '" << name << L"' не прошла.\n";
    return false;
}

bool ExpectHResult(const wchar_t* name, HRESULT actual, HRESULT expected)
{
    if (actual == expected)
        return true;

    std::wcerr << L"Проверка HRESULT '" << name << L"' не прошла. Ожидалось 0x"
               << std::hex << expected << L", получено 0x" << actual << std::dec << L".\n";
    return false;
}

bool ExpectEqualInt(const wchar_t* name, int actual, int expected)
{
    if (actual == expected)
        return true;

    std::wcerr << L"Проверка числа '" << name << L"' не прошла. Ожидалось "
               << expected << L", получено " << actual << L".\n";
    return false;
}

bool ExpectLessOrEqualInt(const wchar_t* name, int actual, int expectedMax)
{
    if (actual <= expectedMax)
        return true;

    std::wcerr << L"Проверка числа '" << name << L"' не прошла. Ожидалось значение <= "
               << expectedMax << L", получено " << actual << L".\n";
    return false;
}

HRESULT ReadFileBytes(const wchar_t* filePath, std::vector<unsigned char>& bytes)
{
    bytes.clear();

    HANDLE fileHandle = ::CreateFileW(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(::GetLastError());

    LARGE_INTEGER fileSize = {};
    if (!::GetFileSizeEx(fileHandle, &fileSize)) {
        const HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
        ::CloseHandle(fileHandle);
        return hr;
    }

    if (fileSize.QuadPart < 0 || fileSize.QuadPart > 32 * 1024 * 1024) {
        ::CloseHandle(fileHandle);
        return E_FAIL;
    }

    bytes.resize(static_cast<size_t>(fileSize.QuadPart));
    DWORD bytesRead = 0;
    const BOOL readOk = bytes.empty()
        ? TRUE
        : ::ReadFile(fileHandle, bytes.data(), static_cast<DWORD>(bytes.size()), &bytesRead, nullptr);
    const HRESULT result = (!readOk || bytesRead != bytes.size())
        ? HRESULT_FROM_WIN32(::GetLastError())
        : S_OK;
    ::CloseHandle(fileHandle);
    return result;
}

HRESULT CreateReadOnlyStreamFromFile(const wchar_t* filePath, IStream** stream)
{
    if (stream == nullptr)
        return E_POINTER;

    *stream = nullptr;

    std::vector<unsigned char> bytes;
    const HRESULT readHr = ReadFileBytes(filePath, bytes);
    if (FAILED(readHr))
        return readHr;

    HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (memory == nullptr)
        return E_OUTOFMEMORY;

    void* buffer = ::GlobalLock(memory);
    if (buffer == nullptr) {
        const HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
        ::GlobalFree(memory);
        return hr;
    }

    if (!bytes.empty())
        std::memcpy(buffer, bytes.data(), bytes.size());
    ::GlobalUnlock(memory);

    const HRESULT hr = ::CreateStreamOnHGlobal(memory, TRUE, stream);
    if (FAILED(hr))
        ::GlobalFree(memory);
    return hr;
}

int FitDimension(int sourceDimension, int sourceMaxEdge, UINT requestedEdge)
{
    if (sourceMaxEdge <= static_cast<int>(requestedEdge)) return sourceDimension;
    return (std::max)(1, static_cast<int>(static_cast<long long>(sourceDimension) * requestedEdge / sourceMaxEdge));
}
bool GetBitmapSize(HBITMAP bitmap, int& width, int& height)
{
    width = 0;
    height = 0;

    BITMAP bitmapInfo = {};
    if (::GetObject(bitmap, sizeof(bitmapInfo), &bitmapInfo) != sizeof(bitmapInfo))
        return false;

    width = bitmapInfo.bmWidth;
    height = bitmapInfo.bmHeight;
    return true;
}

} // namespace

int wmain(int argc, wchar_t* argv[])
{
    const int requestedEdge = 256;

    if (argc != 11) {
        std::wcerr << L"Использование: fb2-thumbnail-provider-smoke.exe <png-fb2> <несколько-binary-fb2> <jpeg-fb2> <bmp-fb2> <малая-вертикальная-fb2> <крупная-горизонтальная-fb2> <крупная-alpha-вертикальная-fb2> <битый-fb2> <без-coverpage-fb2> <без-binary-fb2>\n";
        return 10;
    }

    const HRESULT initHr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initHr)) {
        std::wcerr << L"Ошибка CoInitializeEx: 0x" << std::hex << initHr << std::dec << L"\n";
        return 11;
    }

    bool success = true;

    CComObject<Fb2ThumbnailProvider>* providerWithoutInit = nullptr;
    HRESULT hr = CComObject<Fb2ThumbnailProvider>::CreateInstance(&providerWithoutInit);
    if (FAILED(hr) || providerWithoutInit == nullptr) {
        std::wcerr << L"Не удалось создать экземпляр Fb2ThumbnailProvider: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 12;
    }

    HBITMAP bitmap = nullptr;
    WTS_ALPHATYPE alphaType = WTSAT_UNKNOWN;
    hr = providerWithoutInit->Initialize(nullptr, STGM_READ);
    success = ExpectHResult(L"Initialize.nullStream", hr, E_POINTER) && success;
    hr = providerWithoutInit->GetThumbnail(requestedEdge, nullptr, &alphaType);
    success = ExpectHResult(L"GetThumbnail.nullBitmap", hr, E_POINTER) && success;
    bitmap = nullptr;
    hr = providerWithoutInit->GetThumbnail(requestedEdge, &bitmap, nullptr);
    success = ExpectHResult(L"GetThumbnail.nullAlphaType", hr, E_POINTER) && success;
    hr = providerWithoutInit->GetThumbnail(requestedEdge, &bitmap, &alphaType);
    success = ExpectHResult(L"GetThumbnail.beforeInitialize", hr, E_UNEXPECTED) && success;
    success = ExpectTrue(L"bitmap.beforeInitialize.null", bitmap == nullptr) && success;
    delete providerWithoutInit;

    struct ValidFixture { const wchar_t* path; const wchar_t* label; int width; int height; WTS_ALPHATYPE alpha; };
    const ValidFixture validFixtures[] = {
        { argv[1], L"png-tiny", 1, 1, WTSAT_RGB },
        { argv[2], L"multiple-binaries", 1, 1, WTSAT_RGB },
        { argv[3], L"jpeg", 1, 1, WTSAT_RGB },
        { argv[4], L"bmp", 1, 1, WTSAT_RGB },
        { argv[5], L"visible-portrait", 96, 128, WTSAT_ARGB },
        { argv[6], L"large-horizontal", 768, 512, WTSAT_RGB },
        { argv[7], L"large-vertical-alpha", 512, 768, WTSAT_ARGB }
    };
    const UINT requestedEdges[] = { 32, 64, 128, 256, 512 };
    for (size_t i = 0; i < _countof(validFixtures); ++i) {
        CComPtr<IStream> validStream;
        hr = CreateReadOnlyStreamFromFile(validFixtures[i].path, &validStream);
        if (FAILED(hr)) { std::wcerr << L"Unable to open valid fixture: " << validFixtures[i].label << L"\n"; ::CoUninitialize(); return 13 + static_cast<int>(i); }
        CComObject<Fb2ThumbnailProvider>* validProvider = nullptr;
        hr = CComObject<Fb2ThumbnailProvider>::CreateInstance(&validProvider);
        if (FAILED(hr) || validProvider == nullptr) { ::CoUninitialize(); return 20 + static_cast<int>(i); }
        ATL::CString initializeName; initializeName.Format(L"Initialize.valid.%s", validFixtures[i].label);
        success = ExpectHResult(initializeName, validProvider->Initialize(validStream, STGM_READ), S_OK) && success;
        ATL::CString initializeSecondName; initializeSecondName.Format(L"Initialize.secondCall.%s", validFixtures[i].label);
        success = ExpectHResult(initializeSecondName, validProvider->Initialize(validStream, STGM_READ), HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED)) && success;

        const int sourceMaxEdge = (std::max)(validFixtures[i].width, validFixtures[i].height);
        for (size_t requested = 0; requested < _countof(requestedEdges); ++requested) {
            const UINT requestedEdge = requestedEdges[requested];
            bitmap = nullptr; alphaType = WTSAT_UNKNOWN;
            ATL::CString getThumbnailName; getThumbnailName.Format(L"GetThumbnail.%s.%u", validFixtures[i].label, requestedEdge);
            success = ExpectHResult(getThumbnailName, validProvider->GetThumbnail(requestedEdge, &bitmap, &alphaType), S_OK) && success;
            int width = 0, height = 0;
            ATL::CString sizeName; sizeName.Format(L"bitmap.size.%s.%u", validFixtures[i].label, requestedEdge);
            success = ExpectTrue(sizeName, bitmap != nullptr && GetBitmapSize(bitmap, width, height)) && success;
            const int expectedWidth = FitDimension(validFixtures[i].width, sourceMaxEdge, requestedEdge);
            const int expectedHeight = FitDimension(validFixtures[i].height, sourceMaxEdge, requestedEdge);
            ATL::CString widthName; widthName.Format(L"bitmap.width.%s.%u", validFixtures[i].label, requestedEdge);
            ATL::CString heightName; heightName.Format(L"bitmap.height.%s.%u", validFixtures[i].label, requestedEdge);
            ATL::CString alphaName; alphaName.Format(L"bitmap.alpha.%s.%u", validFixtures[i].label, requestedEdge);
            success = ExpectEqualInt(widthName, width, expectedWidth) && success;
            success = ExpectEqualInt(heightName, height, expectedHeight) && success;
            success = ExpectTrue(alphaName, alphaType == validFixtures[i].alpha) && success;
            std::wcout << L"[thumbnail-provider-smoke] " << validFixtures[i].label << L": " << width << L"x" << height << L", requested=" << requestedEdge << L", alpha=" << alphaType << L"\n";
            if (bitmap != nullptr) ::DeleteObject(bitmap);
        }
        delete validProvider;
    }
    CComPtr<IStream> brokenStream;
    hr = CreateReadOnlyStreamFromFile(argv[8], &brokenStream);
    if (FAILED(hr)) {
        std::wcerr << L"Не удалось открыть битый fixture как IStream: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 15;
    }

    CComObject<Fb2ThumbnailProvider>* brokenProvider = nullptr;
    hr = CComObject<Fb2ThumbnailProvider>::CreateInstance(&brokenProvider);
    if (FAILED(hr) || brokenProvider == nullptr) {
        std::wcerr << L"Не удалось создать provider для битого файла: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 16;
    }

    hr = brokenProvider->Initialize(brokenStream, STGM_READ);
    success = ExpectHResult(L"Initialize.broken", hr, S_OK) && success;

    bitmap = nullptr;
    alphaType = WTSAT_UNKNOWN;
    hr = brokenProvider->GetThumbnail(requestedEdge, &bitmap, &alphaType);
    success = ExpectHResult(L"GetThumbnail.broken", hr, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT)) && success;
    success = ExpectTrue(L"bitmap.broken.null", bitmap == nullptr) && success;
    delete brokenProvider;

    CComPtr<IStream> missingCoverStream;
    hr = CreateReadOnlyStreamFromFile(argv[9], &missingCoverStream);
    if (FAILED(hr)) {
        std::wcerr << L"Не удалось открыть fixture без coverpage как IStream: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 17;
    }

    CComObject<Fb2ThumbnailProvider>* missingCoverProvider = nullptr;
    hr = CComObject<Fb2ThumbnailProvider>::CreateInstance(&missingCoverProvider);
    if (FAILED(hr) || missingCoverProvider == nullptr) {
        std::wcerr << L"Не удалось создать provider для файла без coverpage: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 18;
    }

    hr = missingCoverProvider->Initialize(missingCoverStream, STGM_READ);
    success = ExpectHResult(L"Initialize.missingCoverpage", hr, S_OK) && success;
    bitmap = nullptr;
    alphaType = WTSAT_UNKNOWN;
    hr = missingCoverProvider->GetThumbnail(requestedEdge, &bitmap, &alphaType);
    success = ExpectHResult(L"GetThumbnail.missingCoverpage", hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) && success;
    success = ExpectTrue(L"bitmap.missingCoverpage.null", bitmap == nullptr) && success;
    delete missingCoverProvider;

    CComPtr<IStream> missingBinaryStream;
    hr = CreateReadOnlyStreamFromFile(argv[10], &missingBinaryStream);
    if (FAILED(hr)) {
        std::wcerr << L"Не удалось открыть fixture без binary как IStream: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 19;
    }

    CComObject<Fb2ThumbnailProvider>* missingBinaryProvider = nullptr;
    hr = CComObject<Fb2ThumbnailProvider>::CreateInstance(&missingBinaryProvider);
    if (FAILED(hr) || missingBinaryProvider == nullptr) {
        std::wcerr << L"Не удалось создать provider для файла без binary: 0x"
                   << std::hex << hr << std::dec << L"\n";
        ::CoUninitialize();
        return 20;
    }

    hr = missingBinaryProvider->Initialize(missingBinaryStream, STGM_READ);
    success = ExpectHResult(L"Initialize.missingBinary", hr, S_OK) && success;
    bitmap = nullptr;
    alphaType = WTSAT_UNKNOWN;
    hr = missingBinaryProvider->GetThumbnail(requestedEdge, &bitmap, &alphaType);
    success = ExpectHResult(L"GetThumbnail.missingBinary", hr, HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) && success;
    success = ExpectTrue(L"bitmap.missingBinary.null", bitmap == nullptr) && success;
    delete missingBinaryProvider;

    ::CoUninitialize();
    return success ? 0 : 3;
}
