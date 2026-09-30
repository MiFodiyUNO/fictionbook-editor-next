#include "stdafx.h"
#include "search/SearchPresetCatalog.h"
#include "RuntimeLocalization.h"

#include <iostream>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC
#include "pcre2.h"

// The catalog intentionally uses the runtime localization accessor. The native
// regex contract verifies its production definitions with deterministic English
// fall-backs, without depending on a user language overlay.
CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback)
{
    return fallback ? CString(fallback) : CString();
}

namespace
{
using FbeSearchPresets::SearchPreset;
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

bool Replaces(const SearchPreset& preset, LPCWSTR subject, LPCWSTR expected)
{
    if (!preset.hasReplacement) return false;
    int error = 0;
    PCRE2_SIZE offset = 0;
    const uint32_t flags = PCRE2_UTF | (preset.unicodeProperties ? PCRE2_UCP : 0);
    pcre2_code* code = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(static_cast<LPCWSTR>(preset.findText)),
        preset.findText.GetLength(), flags, &error, &offset, NULL);
    if (!code) return false;
    PCRE2_SIZE capacity = 1024;
    std::vector<PCRE2_UCHAR> output(capacity, 0);
    const int result = pcre2_substitute(code,
        reinterpret_cast<PCRE2_SPTR>(subject), wcslen(subject), 0, PCRE2_SUBSTITUTE_GLOBAL,
        NULL, NULL, reinterpret_cast<PCRE2_SPTR>(static_cast<LPCWSTR>(preset.replacementText)),
        preset.replacementText.GetLength(), &output[0], &capacity);
    const bool ok = result >= 0 && CString(reinterpret_cast<LPCWSTR>(&output[0]), static_cast<int>(capacity)) == expected;
    pcre2_code_free(code);
    return ok;
}

bool Check(const std::vector<SearchPreset>& presets, LPCWSTR id, LPCWSTR subject, LPCWSTR expected)
{
    const SearchPreset* preset = Find(presets, id);
    return preset && Replaces(*preset, subject, expected);
}
}

int wmain()
{
    std::vector<SearchPreset> design;
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Design, false, design);
    std::vector<SearchPreset> designReplace;
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Design, true, designReplace);
    std::vector<SearchPreset> source;
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Source, false, source);
    std::vector<SearchPreset> sourceReplace;
    FbeSearchPresets::GetBuiltInPresets(SearchUiContext::Source, true, sourceReplace);

    if (design.size() != 8 || designReplace.size() != 6 || source.size() != 1 || !sourceReplace.empty()) return 1;
    if (!Check(design, L"design.normalize-spaces", L"one   two", L"one two")) return 2;
    if (!Check(design, L"design.trim-before-punctuation", L"word \t, next", L"word, next")) return 3;
    if (!Check(design, L"design.trim-leading", L" \tword", L"word")) return 4;
    if (!Check(design, L"design.trim-trailing", L"word \t", L"word")) return 5;
    if (!Check(design, L"design.tabs-to-spaces", L"one\t\ttwo", L"one two")) return 6;
    if (!Check(design, L"design.nbsp-to-space", L"one\x00A0two", L"one two")) return 7;

    const SearchPreset* duplicate = Find(design, L"design.duplicate-word");
    const SearchPreset* punctuation = Find(design, L"design.repeated-punctuation");
    const SearchPreset* sourcePunctuation = Find(source, L"source.repeated-punctuation");
    if (!duplicate || !duplicate->unicodeProperties || !Matches(*duplicate, L"тест тест")) return 8;
    if (!punctuation || punctuation->hasReplacement || !Matches(*punctuation, L"What?!")) return 9;
    if (!sourcePunctuation || sourcePunctuation->hasReplacement || !Matches(*sourcePunctuation, L"?!")) return 10;
    if (Find(source, L"design.normalize-spaces") != NULL) return 11;
    return 0;
}
