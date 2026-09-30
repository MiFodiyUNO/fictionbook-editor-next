#include "stdafx.h"
#include "SearchPresetCatalog.h"
#include "..\\RuntimeLocalization.h"

namespace
{
using FbeSearchPresets::SearchPreset;
using FbeSearchPresets::SearchUiContext;

struct Definition
{
    const wchar_t* id;
    const wchar_t* nameKey;
    const wchar_t* nameFallback;
    const wchar_t* descriptionKey;
    const wchar_t* descriptionFallback;
    const wchar_t* findText;
    bool hasReplacement;
    const wchar_t* replacementText;
    bool unicodeProperties;
    SearchUiContext context;
};

const Definition kDefinitions[] = {
    { L"design.normalize-spaces", L"fbe.search_preset.normalize_spaces.name", L"Multiple spaces to one",
        L"fbe.search_preset.normalize_spaces.description", L"Replace two or more horizontal spaces with one space.",
        L"[ \\t]{2,}", true, L" ", false, SearchUiContext::Design },
    { L"design.trim-before-punctuation", L"fbe.search_preset.trim_before_punctuation.name", L"Remove spaces before punctuation",
        L"fbe.search_preset.trim_before_punctuation.description", L"Remove spaces and tabs before , ; : ! and ?.",
        L"[ \\t]+([,;:!?])", true, L"$1", false, SearchUiContext::Design },
    { L"design.trim-leading", L"fbe.search_preset.trim_leading.name", L"Remove leading spaces",
        L"fbe.search_preset.trim_leading.description", L"Remove spaces and tabs at the start of a paragraph.",
        L"^[ \\t]+", true, L"", false, SearchUiContext::Design },
    { L"design.trim-trailing", L"fbe.search_preset.trim_trailing.name", L"Remove trailing spaces",
        L"fbe.search_preset.trim_trailing.description", L"Remove spaces and tabs at the end of a paragraph.",
        L"[ \\t]+$", true, L"", false, SearchUiContext::Design },
    { L"design.tabs-to-spaces", L"fbe.search_preset.tabs_to_spaces.name", L"Tabs to spaces",
        L"fbe.search_preset.tabs_to_spaces.description", L"Replace tabs with a single space.",
        L"\\t+", true, L" ", false, SearchUiContext::Design },
    { L"design.nbsp-to-space", L"fbe.search_preset.nbsp_to_space.name", L"Non-breaking spaces to spaces",
        L"fbe.search_preset.nbsp_to_space.description", L"Replace non-breaking spaces with ordinary spaces.",
        L"\u00A0", true, L" ", false, SearchUiContext::Design },
    { L"design.duplicate-word", L"fbe.search_preset.duplicate_word.name", L"Repeated adjacent word",
        L"fbe.search_preset.duplicate_word.description", L"Find a repeated adjacent word, including Cyrillic text.",
        L"\\b(\\p{L}+)\\s+\\1\\b", false, L"", true, SearchUiContext::Design },
    { L"design.repeated-punctuation", L"fbe.search_preset.repeated_punctuation.name", L"Repeated ! or ?",
        L"fbe.search_preset.repeated_punctuation.description", L"Find repeated exclamation or question marks.",
        L"[!?]{2,}", false, L"", false, SearchUiContext::Design },
    { L"source.repeated-punctuation", L"fbe.search_preset.source_repeated_punctuation.name", L"Repeated ! or ?",
        L"fbe.search_preset.source_repeated_punctuation.description", L"Find repeated punctuation without changing XML source.",
        L"[!?]{2,}", false, L"", false, SearchUiContext::Source }
};
}

namespace FbeSearchPresets
{
void GetBuiltInPresets(SearchUiContext context, bool forReplace,
    std::vector<SearchPreset>& presets)
{
    presets.clear();
    for (size_t index = 0; index < _countof(kDefinitions); ++index)
    {
        const Definition& definition = kDefinitions[index];
        if (definition.context != context || (forReplace && !definition.hasReplacement))
            continue;

        SearchPreset preset;
        preset.id = definition.id;
        preset.name = FbeLoadRuntimeStringByKey(definition.nameKey, definition.nameFallback);
        preset.description = FbeLoadRuntimeStringByKey(definition.descriptionKey, definition.descriptionFallback);
        preset.findText = definition.findText;
        preset.hasReplacement = definition.hasReplacement;
        preset.replacementText = definition.replacementText;
        preset.regexp = true;
        preset.unicodeProperties = definition.unicodeProperties;
        preset.context = definition.context;
        preset.builtIn = true;
        presets.push_back(preset);
    }
}
}
