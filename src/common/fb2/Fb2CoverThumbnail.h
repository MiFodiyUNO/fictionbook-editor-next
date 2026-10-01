#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <wincodec.h>
#include <vector>

namespace FB2CoverThumbnail {

struct DecodedImage {
    CComPtr<IWICBitmapSource> source;
    HBITMAP bitmap;
    int width;
    int height;
    bool hasAlpha;

    DecodedImage();
    ~DecodedImage();

    void Reset();
    bool IsEmpty() const;
};

bool TryDecode(const std::vector<unsigned char>& bytes, DecodedImage& image, ATL::CString* errorMessage = nullptr);
bool TryResizeToFit(const DecodedImage& sourceImage, unsigned int maxEdge, DecodedImage& resizedImage, ATL::CString* errorMessage = nullptr);

} // namespace FB2CoverThumbnail
