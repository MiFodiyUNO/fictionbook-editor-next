#include "stdafx.h"
#include "search/SearchPresetCatalog.h"
#include "RuntimeLocalization.h"

#include <iostream>
#include <set>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC
#include "pcre2.h"

CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback)
{
    return fallback ? CString(fallback) : CString();
}

namespace
{
using FbeSearchPresets::SearchPreset;
using FbeSearchPresets::SearchPresetSafety;
using FbeSearchPresets::SearchUiContext;

const SearchPreset* Find(const std::vector<SearchPreset>& presets, LPCWSTR id)
{
    for (size_t index = 0; index < presets.size(); ++index)
        if (presets[index].id == id) return &presets[index];
    return NULL;
}

bool Matches(const SearchPreset& preset, LPCWSTR subject)
{
    int error = 0;
    PCRE2_SIZE offset = 0;
    const uint32_t flags = PCRE2_UTF | (preset.unicodeProperties ? PCRE2_UCP : 0);
    pcre2_code* code = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(static_cast<LPCWSTR>(preset.findText)),
        preset.findText.GetLength(), flags, &error, &offset, NULL);
    if (!code) return false;
    pcre2_match_data* data = pcre2_match_data_create_from_pattern(code, NULL);
    const int match = data ? pcre2_match(code, reinterpret_cast<PCRE2_SPTR>(subject), wcslen(subject), 0, 0, data, NULL) : PCRE2_ERROR_NOMEMORY;
    if (data) pcre2_match_data_free(data);
    pcre2_code_free(code);
    return match >= 0;
}

bool IsValidCategory(FbeSearchPresets::SearchPresetCategory category)
{
    return category >= FbeSearchPresets::SearchPresetCategory::Whitespace && category <= FbeSearchPresets::SearchPresetCategory::Diagnostics;
}

bool IsValidSafety(SearchPresetSafety safety)
{
    return safety == SearchPresetSafety::SafeReplace || safety == SearchPresetSafety::ReviewOnly;
}
bool Compiles(const SearchPreset& preset)
{
    int error = 0;
    PCRE2_SIZE offset = 0;
    const uint32_t flags = PCRE2_UTF | (preset.unicodeProperties ? PCRE2_UCP : 0);
    pcre2_code* code = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(static_cast<LPCWSTR>(preset.findText)),
        preset.findText.GetLength(), flags, &error, &offset, NULL);
    if (!code) return false;
    pcre2_code_free(code);
    return true;
}
}

int wmain()
{
    std::vector<SearchPreset> design, designReplace, source, sourceReplace;
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Design, false, design);
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Design, true, designReplace);
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Source, false, source);
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Source, true, sourceReplace);

    if (design.size() < 34 || design.size() > 38 || source.size() < 22 || source.size() > 26) return 1;
    if (designReplace.empty() || sourceReplace.empty()) return 2;

    std::set<std::wstring> ids;
    int safeReplace = 0, reviewOnly = 0;
    for (size_t index = 0; index < design.size(); ++index)
    {
        const SearchPreset& preset = design[index];
        if (!preset.builtIn || preset.context != SearchUiContext::Design || !preset.regexp || !IsValidCategory(preset.category) || !IsValidSafety(preset.safety) || preset.id.IsEmpty() || preset.name.IsEmpty() || preset.description.IsEmpty() || !ids.insert(static_cast<LPCWSTR>(preset.id)).second) return 3;
        if (preset.safety == SearchPresetSafety::SafeReplace && !preset.hasReplacement) return 4;
        if (preset.safety == SearchPresetSafety::ReviewOnly && preset.hasReplacement) return 5;
        if (!Compiles(preset)) return 6; // Design is executed by PCRE2.
        if (preset.findText.Find(L"\\p{") >= 0 && !preset.unicodeProperties) return 7;
        if (preset.safety == SearchPresetSafety::SafeReplace) ++safeReplace; else ++reviewOnly;
    }
    for (size_t index = 0; index < source.size(); ++index)
    {
        const SearchPreset& preset = source[index];
        if (!preset.builtIn || preset.context != SearchUiContext::Source || !preset.regexp || !IsValidCategory(preset.category) || !IsValidSafety(preset.safety) || preset.id.IsEmpty() || preset.name.IsEmpty() || preset.description.IsEmpty() || !ids.insert(static_cast<LPCWSTR>(preset.id)).second) return 8;
        if (preset.safety == SearchPresetSafety::SafeReplace && !preset.hasReplacement) return 9;
        if (preset.safety == SearchPresetSafety::ReviewOnly && preset.hasReplacement) return 10;
        if (preset.unicodeProperties) return 11; // Source fixtures use real Scintilla C++11 regex.
        if (preset.safety == SearchPresetSafety::SafeReplace) ++safeReplace; else ++reviewOnly;
    }
    if (safeReplace == 0 || reviewOnly == 0) return 12;

    const SearchPreset* normalize = Find(design, L"design.normalize-spaces");
    const SearchPreset* duplicate = Find(design, L"design.duplicate-word");
    const SearchPreset* sourcePunctuation = Find(source, L"source.repeated-punctuation");
    if (!normalize || !normalize->hasReplacement || !Matches(*normalize, L"one   two")) return 13;
    if (!duplicate || !duplicate->unicodeProperties || !Matches(*duplicate, L"тест тест")) return 14;
    if (!sourcePunctuation || sourcePunctuation->hasReplacement || sourcePunctuation->findText != L"[!?][!?]+") return 15;
    if (Find(source, L"design.normalize-spaces") != NULL) return 16;
    return 0;
}