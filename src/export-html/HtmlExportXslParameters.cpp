#include "stdafx.h"
#include "HtmlExportXslParameters.h"

namespace {
void Add(IXSLProcessor* processor, LPCWSTR name, const variant_t& value)
{
    CheckError(processor->addParameter(bstr_t(name), value, _bstr_t()));
}
}

void HtmlExportXslParameters::Apply(IXSLProcessor* processor, const HtmlExportSettings& settings, const CString& customCss)
{
    Add(processor, L"includetoc", variant_t(settings.includeToc));
    Add(processor, L"tocdepth", variant_t(static_cast<long>(settings.tocDepth)));
    Add(processor, L"includemetadata", variant_t(settings.includeMetadata));
    Add(processor, L"includedesc", variant_t(settings.includeDescription)); // compatibility with custom XSL
    Add(processor, L"includeannotation", variant_t(settings.includeAnnotation));
    Add(processor, L"includetitleinfo", variant_t(settings.includeTitleInfo));
    Add(processor, L"includedocumentinfo", variant_t(settings.includeDocumentInfo));
    Add(processor, L"includepublishinfo", variant_t(settings.includePublishInfo));
    Add(processor, L"includehistory", variant_t(settings.includeHistory));
    Add(processor, L"includeauthors", variant_t(settings.includeAuthors));
    Add(processor, L"includetranslators", variant_t(settings.includeTranslators));
    Add(processor, L"includecustominfo", variant_t(settings.includeCustomInfo));
    Add(processor, L"style", variant_t(static_cast<long>(settings.style)));
    Add(processor, L"fontfamily", variant_t(static_cast<long>(settings.fontFamily)));
    Add(processor, L"customfontfamily", variant_t(static_cast<LPCTSTR>(settings.customFontFamily)));
    Add(processor, L"fontsize", variant_t(static_cast<long>(settings.fontSize)));
    Add(processor, L"lineheight", variant_t(static_cast<long>(settings.lineHeight)));
    Add(processor, L"contentmaxwidth", variant_t(static_cast<long>(settings.contentMaxWidth)));
    Add(processor, L"pagemargins", variant_t(static_cast<long>(settings.pageMargins)));
    Add(processor, L"textalignment", variant_t(static_cast<long>(settings.textAlignment)));
    Add(processor, L"headingalignment", variant_t(static_cast<long>(settings.headingAlignment)));
    Add(processor, L"covermode", variant_t(static_cast<long>(settings.coverMode)));
    Add(processor, L"imagemaxwidth", variant_t(static_cast<long>(settings.imageMaxWidth)));
    Add(processor, L"imagemaxheight", variant_t(static_cast<long>(settings.imageMaxHeight)));
    Add(processor, L"noteplacement", variant_t(static_cast<long>(settings.notePlacement))); // reserved by XSL for future placement modes
    Add(processor, L"customcss", variant_t(static_cast<LPCTSTR>(customCss)));
}

void HtmlExportXslParameters::ApplyImageMode(IXSLProcessor* processor, bool saveImages, bool embedImages, const CString& imagePrefix)
{
    Add(processor, L"saveimages", variant_t(saveImages));
    Add(processor, L"embedimages", variant_t(embedImages));
    Add(processor, L"imgprefix", variant_t(static_cast<LPCTSTR>(imagePrefix)));
}