#include "stdafx.h"
#include "SearchPresetCatalog.h"
#include "..\\RuntimeLocalization.h"

namespace
{
using FbeSearchPresets::SearchPreset;
using FbeSearchPresets::SearchPresetCategory;
using FbeSearchPresets::SearchPresetSafety;
using FbeSearchPresets::SearchUiContext;

struct Definition
{
    const wchar_t* id;
    SearchPresetCategory category;
    SearchPresetSafety safety;
    const wchar_t* nameFallback;
    const wchar_t* descriptionFallback;
    const wchar_t* findText;
    bool regexp;
    bool hasReplacement;
    const wchar_t* replacementText;
    bool matchCase;
    bool wholeWord;
    bool unicodeProperties;
    SearchUiContext context;
};

#define WIDEN_TOKEN_IMPL(value) L##value
#define WIDEN_TOKEN(value) WIDEN_TOKEN_IMPL(value)
#define PRESET(ID, CATEGORY, SAFETY, NAME, DESCRIPTION, FIND, HAS_REPLACEMENT, REPLACEMENT, MATCH_CASE, WHOLE_WORD, UCP, CONTEXT) \
    { WIDEN_TOKEN(#ID), SearchPresetCategory::CATEGORY, SearchPresetSafety::SAFETY, NAME, DESCRIPTION, FIND, true, HAS_REPLACEMENT, REPLACEMENT, MATCH_CASE, WHOLE_WORD, UCP, SearchUiContext::CONTEXT }

const Definition kDefinitions[] = {
    PRESET(design_normalize_spaces, Whitespace, SafeReplace, L"Multiple spaces to one", L"Replace two or more horizontal spaces with one space.", L"[ \\t]{2,}", true, L" ", false, false, false, Design),
    PRESET(design_trim_before_punctuation, Punctuation, SafeReplace, L"Remove spaces before punctuation", L"Remove spaces and tabs before punctuation.", L"[ \\t]+([,;:!?])", true, L"$1", false, false, false, Design),
    PRESET(design_trim_leading, Whitespace, SafeReplace, L"Remove leading spaces", L"Remove spaces and tabs at the start of a paragraph.", L"^[ \\t]+", true, L"", false, false, false, Design),
    PRESET(design_trim_trailing, Whitespace, SafeReplace, L"Remove trailing spaces", L"Remove spaces and tabs at the end of a paragraph.", L"[ \\t]+$", true, L"", false, false, false, Design),
    PRESET(design_tabs_to_spaces, Whitespace, SafeReplace, L"Tabs to spaces", L"Replace tabs with a single space.", L"\\t+", true, L" ", false, false, false, Design),
    PRESET(design_nbsp_to_space, Whitespace, SafeReplace, L"Non-breaking spaces to spaces", L"Replace non-breaking spaces with ordinary spaces.", L"\u00A0", true, L" ", false, false, false, Design),
    PRESET(design_trim_after_opening, Punctuation, SafeReplace, L"Remove space after opening punctuation", L"Remove spaces after an opening bracket or quotation mark.", L"([([«„])[ \\t]+", true, L"$1", false, false, false, Design),
    PRESET(design_trim_before_closing, Punctuation, SafeReplace, L"Remove space before closing punctuation", L"Remove spaces before a closing bracket or quotation mark.", L"[ \\t]+([)\\]»”])", true, L"$1", false, false, false, Design),
    PRESET(design_trim_before_period, Punctuation, SafeReplace, L"Remove space before period", L"Remove spaces before a period or ellipsis.", L"[ \\t]+([.…])", true, L"$1", false, false, false, Design),
    PRESET(design_ellipsis_three_dots, Typography, SafeReplace, L"Three dots to ellipsis", L"Replace three or more consecutive dots with an ellipsis character.", L"\\.{3,}", true, L"…", false, false, false, Design),
    PRESET(design_ellipsis_spaced_dots, Typography, SafeReplace, L"Spaced dots to ellipsis", L"Replace three dots separated by spaces with an ellipsis character.", L"\\.[ \\t]*\\.[ \\t]*\\.", true, L"…", false, false, false, Design),
    PRESET(design_number_nbsp, Typography, SafeReplace, L"Number sign with non-breaking space", L"Keep a number sign and following number together.", L"№[ \\t]+(\\d+)", true, L"№\u00A0$1", false, false, false, Design),
    PRESET(design_section_nbsp, Typography, SafeReplace, L"Section sign with non-breaking space", L"Keep a section sign and following number together.", L"§[ \\t]+(\\d+)", true, L"§\u00A0$1", false, false, false, Design),
    PRESET(design_duplicate_word, Proofreading, ReviewOnly, L"Repeated adjacent word", L"Find a repeated adjacent word, including Cyrillic text.", L"\\b(\\p{L}+)\\s+\\1\\b", false, L"", false, false, true, Design),
    PRESET(design_repeated_punctuation, Proofreading, ReviewOnly, L"Repeated exclamation or question marks", L"Find repeated exclamation or question marks.", L"[!?]{2,}", false, L"", false, false, false, Design),
    PRESET(design_hidden_characters, Ocr, ReviewOnly, L"Hidden or unusual spaces", L"Find soft hyphens, zero-width characters, unusual Unicode spaces, or repeated non-breaking spaces.", L"\\x{00AD}|[\\x{200B}\\x{FEFF}]|[\\x{2000}-\\x{200A}\\x{202F}\\x{205F}\\x{3000}]|\\x{00A0}{2,}", false, L"", false, false, true, Design),
    // Idea adapted from: runtime/Scripts/06_Чистка/03_Латиница в Кириллице.js
    PRESET(design_mixed_alphabets, Ocr, ReviewOnly, L"Mixed Latin and Cyrillic word", L"Find a word containing both Latin and Cyrillic letters.", L"(?<!\\p{L})(?=[\\p{L}]*[A-Za-z])(?=[\\p{L}]*[А-ЯЁа-яё])\\p{L}+(?!\\p{L})", false, L"", false, false, true, Design),
    PRESET(design_case_or_digit_inside_word, Ocr, ReviewOnly, L"Suspicious letter case or digit inside word", L"Find lower-to-upper transitions, digits, or punctuation inside a word.", L"\\p{Ll}\\p{Lu}|\\p{L}+[.,;:!?]\\p{L}+|\\p{L}+\\d+\\p{L}+|\\d+\\p{L}+\\d+|\\p{Lu}{2,}\\p{Ll}+", false, L"", false, false, true, Design),
    PRESET(design_apostrophe_cyrillic, Ocr, ReviewOnly, L"Apostrophe before Cyrillic word", L"Find a suspicious apostrophe before a Cyrillic lowercase letter.", L"[‘'\\x60](?=[а-яё])", false, L"", false, false, false, Design),
    // Idea adapted from: runtime/Scripts/06_Чистка/05_Слипшиеся слова.js
    PRESET(design_sentence_proofreading, Proofreading, ReviewOnly, L"Suspicious sentence boundary", L"Find a lowercase paragraph start, a missing final sign, or suspicious case after punctuation.", L"^\\p{Ll}|[\\p{L}\\p{N}»”)]$|[.!?…][ \\t]+[«„“\"(\\[]?\\p{Ll}|\\p{Ll}[»”]?[ \\t]+[«„“]?\\p{Lu}\\p{Ll}+", false, L"", false, false, true, Design),
    PRESET(design_quotes_and_punctuation, Proofreading, ReviewOnly, L"Suspicious quotes or punctuation", L"Find repeated quotes, repeated punctuation, or straight double quotes.", L"[\"«»„“”]{2,}|[.,;:]{2,}|\"", false, L"", false, false, false, Design),
    PRESET(design_spaced_hyphen, Proofreading, ReviewOnly, L"Hyphen with spaces inside word", L"Find a hyphen separated from surrounding letters by spaces.", L"\\p{L}+[ \\t]+-[ \\t]+\\p{L}+", false, L"", false, false, true, Design),
    PRESET(design_number_ranges, DashesNumbers, ReviewOnly, L"Suspicious number range", L"Find numbers separated by hyphen or dash with inconsistent spaces.", L"\\d[ \\t]*[-—][ \\t]*\\d|\\d+[ \\t]+[–—-][ \\t]*\\d+|\\d+[ \\t]*[–—-][ \\t]+\\d+|\\d+[ \\t]+—[ \\t]+\\d+", false, L"", false, false, false, Design),
    PRESET(design_thousands_space, DashesNumbers, ReviewOnly, L"Ordinary space in thousands group", L"Find a normal space inside a group of thousands.", L"\\d \\d{3}\\b", false, L"", false, false, false, Design),
    // Idea adapted from: runtime/Scripts/04_Фамилия И. О..js
    PRESET(design_initials_before_name, Names, ReviewOnly, L"Initials before surname", L"Find initials followed by a capitalized surname.", L"\\b(\\p{Lu})\\.[ \\t]*(\\p{Lu})\\.[ \\t]+(\\p{Lu}\\p{Ll}+)\\b", false, L"", false, false, true, Design),
    PRESET(design_initials_after_name, Names, ReviewOnly, L"Surname followed by initials", L"Find a capitalized surname followed by two initials.", L"\\b(\\p{Lu}\\p{Ll}+)[ \\t]+(\\p{Lu})\\.[ \\t]*(\\p{Lu})\\.", false, L"", false, false, true, Design),
    PRESET(design_roman_cyrillic_ha, Ocr, ReviewOnly, L"Cyrillic Ha in Roman numeral", L"Find Cyrillic Х adjacent to Latin Roman numeral letters.", L"(?<=[IVXLCDM])Х|Х(?=[IVXLCDM])", false, L"", false, false, false, Design),

    PRESET(source_trim_trailing, XmlFormatting, SafeReplace, L"Remove trailing XML whitespace", L"Remove spaces and tabs at the end of a source line.", L"[ \\t]+$", true, L"", false, false, false, Source),
    PRESET(source_trim_before_close, XmlFormatting, SafeReplace, L"Remove space before />", L"Remove spaces before an empty XML-element close.", L"[ \\t]+/>", true, L"/>", false, false, false, Source),
    PRESET(source_blank_indentation, XmlFormatting, SafeReplace, L"Clear whitespace-only line", L"Clear a source line that contains only spaces or tabs.", L"^[ \\t]+$", true, L"", false, false, false, Source),
    PRESET(source_repeated_punctuation, Diagnostics, ReviewOnly, L"Repeated exclamation or question marks", L"Find repeated exclamation or question marks in XML source.", L"[!?]{2,}", false, L"", false, false, false, Source),
    PRESET(source_empty_paragraph, Fb2Structure, ReviewOnly, L"Empty paragraph", L"Find an FB2 paragraph without text.", L"<p>[ \\t]*</p>", false, L"", false, false, false, Source),
    PRESET(source_double_empty_line, Fb2Structure, ReviewOnly, L"Adjacent empty-line elements", L"Find two empty-line elements separated only by whitespace.", L"<empty-line/>[ \\t\\r\\n]*<empty-line/>", false, L"", false, false, false, Source),
    PRESET(source_empty_metadata, Fb2Structure, ReviewOnly, L"Empty important metadata", L"Find empty genre, author-name, or book-title elements.", L"<genre>[ \\t]*</genre>|<first-name>[ \\t]*</first-name>|<last-name>[ \\t]*</last-name>|<book-title>[ \\t]*</book-title>", false, L"", false, false, false, Source),
    PRESET(source_empty_inline, Fb2Structure, ReviewOnly, L"Empty inline formatting", L"Find empty strong, emphasis, or strikethrough elements.", L"<strong>[ \\t]*</strong>|<emphasis>[ \\t]*</emphasis>|<strikethrough>[ \\t]*</strikethrough>", false, L"", false, false, false, Source),
    PRESET(source_nested_inline, Fb2Structure, ReviewOnly, L"Nested identical inline formatting", L"Find a strong or emphasis element directly nested in the same element.", L"<strong>[ \\t]*<strong|<emphasis>[ \\t]*<emphasis", false, L"", false, false, false, Source),
    PRESET(source_import_artifacts, ImportArtifacts, ReviewOnly, L"HTML or entity import artifact", L"Find legacy HTML tags, uppercase tags, non-breaking-space entities, or numeric entities.", L"<(b|i|br|div|span|font)([ >])|</?[A-Z][A-Z0-9-]*([ >])|&nbsp;|&#[0-9]+;|&#x[0-9A-Fa-f]+;", false, L"", true, false, false, Source),
    PRESET(source_empty_id, Diagnostics, ReviewOnly, L"Empty id attribute", L"Find an empty id attribute.", L"id=\"\"", false, L"", false, false, false, Source),
    PRESET(source_external_link, LinksNotes, ReviewOnly, L"External or local file link", L"Find HTTP(S) or file links in XML source.", L"(l|xlink):href=\"(https?://|file://)[^\"]+\"", false, L"", false, false, false, Source),
    PRESET(source_suspicious_link, LinksNotes, ReviewOnly, L"Suspicious internal link", L"Find empty, undefined, bookmark, or Word/FBD return links.", L"(l|xlink):href=\"\"|(l|xlink):href=\"#undefined\"|(l|xlink):href=\"#bookmark[^\"]*\"|(l|xlink):href=\"#(_ftnref|_ednref)[^\"]*\"", false, L"", false, false, false, Source),
    PRESET(source_note_link, LinksNotes, ReviewOnly, L"Note link", L"Find a link marked as a note.", L"type=\"note\"[^>]*(l|xlink):href=\"#[^\"]+\"", false, L"", false, false, false, Source),
    PRESET(source_note_marker, LinksNotes, ReviewOnly, L"Possible note marker", L"Find numeric markers in square, curly, or round brackets.", L"\\[[0-9]+\\]|\\{[0-9]+\\}|\\([0-9]+\\)", false, L"", false, false, false, Source),
    PRESET(source_default_metadata, ImportArtifacts, ReviewOnly, L"Default blank-book metadata", L"Find the default Your/Name metadata values.", L"<first-name>Your</first-name>|<last-name>Name</last-name>", false, L"", false, false, false, Source),
    PRESET(source_undefined_image, ImportArtifacts, ReviewOnly, L"Undefined image placeholder", L"Find an image that references the undefined placeholder.", L"<image[^>]*(l|xlink):href=\"#undefined\"[^>]*/?>", false, L"", false, false, false, Source),
    PRESET(source_windows_1251, ImportArtifacts, ReviewOnly, L"Windows-1251 declaration", L"Find a Windows-1251 XML encoding declaration for review.", L"encoding=\"windows-1251\"", false, L"", false, false, false, Source)
};
#undef PRESET
#undef WIDEN_TOKEN
#undef WIDEN_TOKEN_IMPL
}

namespace FbeSearchPresets
{
namespace
{
// Keep identities used by earlier FBE Next builds stable: although catalog
// keys use underscores, a visible built-in can be selected by its id.
LPCWSTR StableBuiltInId(LPCWSTR id)
{
    if (wcscmp(id, L"design_normalize_spaces") == 0) return L"design.normalize-spaces";
    if (wcscmp(id, L"design_trim_before_punctuation") == 0) return L"design.trim-before-punctuation";
    if (wcscmp(id, L"design_trim_leading") == 0) return L"design.trim-leading";
    if (wcscmp(id, L"design_trim_trailing") == 0) return L"design.trim-trailing";
    if (wcscmp(id, L"design_tabs_to_spaces") == 0) return L"design.tabs-to-spaces";
    if (wcscmp(id, L"design_nbsp_to_space") == 0) return L"design.nbsp-to-space";
    if (wcscmp(id, L"design_duplicate_word") == 0) return L"design.duplicate-word";
    if (wcscmp(id, L"design_repeated_punctuation") == 0) return L"design.repeated-punctuation";
    if (wcscmp(id, L"source_repeated_punctuation") == 0) return L"source.repeated-punctuation";
    return id;
}
}
CString StableBuiltInLocalizationKey(LPCWSTR id, bool description)
{
    struct LegacyKey { LPCWSTR id; LPCWSTR key; };
    static const LegacyKey keys[] = {
        { L"design_normalize_spaces", L"normalize_spaces" },
        { L"design_trim_before_punctuation", L"trim_before_punctuation" },
        { L"design_trim_leading", L"trim_leading" },
        { L"design_trim_trailing", L"trim_trailing" },
        { L"design_tabs_to_spaces", L"tabs_to_spaces" },
        { L"design_nbsp_to_space", L"nbsp_to_space" },
        { L"design_duplicate_word", L"duplicate_word" },
        { L"design_repeated_punctuation", L"repeated_punctuation" },
        { L"source_repeated_punctuation", L"source_repeated_punctuation" }
    };
    for (size_t index = 0; index < _countof(keys); ++index)
    {
        if (wcscmp(id, keys[index].id) == 0)
        {
            CString key(L"fbe.search_preset.");
            key += keys[index].key;
            key += description ? L".description" : L".name";
            return key;
        }
    }
    CString key(L"fbe.search_preset.");
    key += id;
    key += description ? L".description" : L".name";
    return key;
}
CString GetPresetCategoryName(SearchPresetCategory category)
{
    switch(category)
    {
    case SearchPresetCategory::Whitespace: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.whitespace", L"Whitespace and indentation");
    case SearchPresetCategory::Punctuation: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.punctuation", L"Punctuation");
    case SearchPresetCategory::Typography: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.typography", L"Typography");
    case SearchPresetCategory::DashesNumbers: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.dashes_numbers", L"Dashes and numbers");
    case SearchPresetCategory::Ocr: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.ocr", L"OCR and recognition");
    case SearchPresetCategory::Proofreading: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.proofreading", L"Proofreading");
    case SearchPresetCategory::Names: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.names", L"Names and abbreviations");
    case SearchPresetCategory::XmlFormatting: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.xml_formatting", L"XML formatting");
    case SearchPresetCategory::Fb2Structure: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.fb2_structure", L"FB2 structure");
    case SearchPresetCategory::LinksNotes: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.links_notes", L"Links and notes");
    case SearchPresetCategory::ImportArtifacts: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.import_artifacts", L"Import artifacts");
    default: return FbeLoadRuntimeStringByKey(L"fbe.search_preset.category.diagnostics", L"Diagnostics");
    }
}

void GetBuiltInPresets(SearchUiContext context, bool forReplace, std::vector<SearchPreset>& presets)
{
    presets.clear();
    for(size_t index = 0; index < _countof(kDefinitions); ++index)
    {
        const Definition& definition = kDefinitions[index];
        if(definition.context != context || (forReplace && !definition.hasReplacement)) continue;
        SearchPreset preset;
        preset.id = StableBuiltInId(definition.id);
        preset.name = FbeLoadRuntimeStringByKey(StableBuiltInLocalizationKey(definition.id, false), definition.nameFallback);
        preset.description = FbeLoadRuntimeStringByKey(StableBuiltInLocalizationKey(definition.id, true), definition.descriptionFallback);
        preset.findText = definition.findText;
        preset.hasReplacement = definition.hasReplacement;
        preset.replacementText = definition.replacementText;
        preset.regexp = definition.regexp;
        preset.matchCase = definition.matchCase;
        preset.wholeWord = definition.wholeWord;
        preset.unicodeProperties = definition.unicodeProperties;
        preset.context = definition.context;
        preset.category = definition.category;
        preset.safety = definition.safety;
        preset.builtIn = true;
        presets.push_back(preset);
    }
}
}
