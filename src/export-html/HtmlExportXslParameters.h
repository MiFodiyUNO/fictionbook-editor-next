#pragma once

#include "HtmlExportSettings.h"

struct IXSLProcessor;

// The exporter delegates the value model to this adapter.  Keeping XSL
// parameter names here prevents the file writer from becoming a second UI.
namespace HtmlExportXslParameters {
void Apply(IXSLProcessor* processor, const HtmlExportSettings& settings, const CString& customCss);
void ApplyImageMode(IXSLProcessor* processor, bool saveImages, bool embedImages, const CString& imagePrefix);
}