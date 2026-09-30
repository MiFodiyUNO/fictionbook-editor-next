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
    const SearchPreset* normalize = Find(design, L"design.normalize-spaces");
    const SearchPreset* trimBeforePunctuation = Find(design, L"design.trim-before-punctuation");
    const SearchPreset* trimLeading = Find(design, L"design.trim-leading");
    const SearchPreset* trimTrailing = Find(design, L"design.trim-trailing");
    const SearchPreset* tabs = Find(design, L"design.tabs-to-spaces");
    const SearchPreset* nbsp = Find(design, L"design.nbsp-to-space");
    if (!normalize || !normalize->hasReplacement || !Matches(*normalize, L"one   two")) return 2;
    if (!trimBeforePunctuation || !trimBeforePunctuation->hasReplacement || !Matches(*trimBeforePunctuation, L"word \t, next")) return 3;
    if (!trimLeading || !trimLeading->hasReplacement || !Matches(*trimLeading, L" \tword")) return 4;
    if (!trimTrailing || !trimTrailing->hasReplacement || !Matches(*trimTrailing, L"word \t")) return 5;
    if (!tabs || !tabs->hasReplacement || !Matches(*tabs, L"one\t\ttwo")) return 6;
    if (!nbsp || !nbsp->hasReplacement || !Matches(*nbsp, L"one\x00A0two")) return 7;

    const SearchPreset* duplicate = Find(design, L"design.duplicate-word");
    const SearchPreset* punctuation = Find(design, L"design.repeated-punctuation");
    const SearchPreset* sourcePunctuation = Find(source, L"source.repeated-punctuation");
    if (!duplicate || !duplicate->unicodeProperties || !Matches(*duplicate, L"тест тест")) return 8;
    if (!punctuation || punctuation->hasReplacement || !Matches(*punctuation, L"What?!")) return 9;
    // Source is deliberately not exercised through PCRE2: production uses
    // Scintilla with SCFIND_REGEXP | SCFIND_CXX11REGEX.  Its runtime fixture
    // lives in regex-fixtures.json and is executed by test-scintilla.ps1.
    if (!sourcePunctuation || sourcePunctuation->hasReplacement || sourcePunctuation->findText != L"[!?]{2,}") return 10;
    if (Find(source, L"design.normalize-spaces") != NULL) return 11;
    return 0;
}
