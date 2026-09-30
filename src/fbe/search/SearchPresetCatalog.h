#pragma once

#include "SearchPreset.h"
#include <vector>

namespace FbeSearchPresets
{
// Built-ins are program data and are never persisted in SearchTemplates.xml.
void GetBuiltInPresets(SearchUiContext context, bool forReplace,
    std::vector<SearchPreset>& presets);
}