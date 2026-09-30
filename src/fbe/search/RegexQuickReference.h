#pragma once

#include "SearchPreset.h"
#include <algorithm>
#include <vector>

namespace FbeSearchPresets
{
enum class RegexQuickReferenceMode { Search, Replacement };
enum class RegexQuickReferenceCategory { Characters, Quantifiers, Groups, Options, Replacement };

struct RegexQuickReferenceEntry
{
    CString displaySyntax;
    CString descriptionKey;
    CString descriptionFallback;
    CString insertionText;
    int caretOffset;
    int selectionStart;
    int selectionLength;
    SearchUiContext context;
    RegexQuickReferenceMode mode;
    RegexQuickReferenceCategory category;
};

struct RegexQuickReferenceInsertion
{
    CString text;
    int caret;
    int selectionStart;
    int selectionLength;
};

inline RegexQuickReferenceInsertion InsertRegexQuickReference(const CString& text, int selectionStart, int selectionEnd,
    const RegexQuickReferenceEntry& entry)
{
    const int start = (std::max)(0, (std::min)(selectionStart, text.GetLength()));
    const int end = (std::max)(start, (std::min)(selectionEnd, text.GetLength()));
    RegexQuickReferenceInsertion result;
    result.text = text.Left(start) + entry.insertionText + text.Mid(end);
    result.caret = start + entry.caretOffset;
    result.selectionStart = start + entry.selectionStart;
    result.selectionLength = entry.selectionLength;
    return result;
}

inline void GetRegexQuickReferenceEntries(SearchUiContext context, RegexQuickReferenceMode mode,
    std::vector<RegexQuickReferenceEntry>& entries)
{
    entries.clear();
    const struct Definition { LPCWSTR syntax, key, fallback, insertion; int caret, selectionStart, selectionLength; SearchUiContext context; RegexQuickReferenceMode mode; RegexQuickReferenceCategory category; } definitions[] = {
        // PCRE2-16, Design search: characters, classes and anchors.
        { L".", L"fbe.regex_quick.any", L"any character", L".", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\.", L"fbe.regex_quick.literal", L"literal metacharacter", L"\\.", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\\\", L"fbe.regex_quick.literal", L"literal backslash", L"\\\\", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\(", L"fbe.regex_quick.literal", L"literal opening parenthesis", L"\\(", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\[", L"fbe.regex_quick.literal", L"literal opening bracket", L"\\[", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\{", L"fbe.regex_quick.literal", L"literal opening brace", L"\\{", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\t", L"fbe.regex_quick.tab", L"tab", L"\\t", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\r", L"fbe.regex_quick.cr", L"carriage return", L"\\r", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\n", L"fbe.regex_quick.lf", L"line feed", L"\\n", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\R", L"fbe.regex_quick.linebreak", L"Unicode line break", L"\\R", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\d", L"fbe.regex_quick.digit", L"digit", L"\\d", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\D", L"fbe.regex_quick.not_digit", L"not a digit", L"\\D", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\s", L"fbe.regex_quick.space", L"whitespace", L"\\s", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\S", L"fbe.regex_quick.not_space", L"not whitespace", L"\\S", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\w", L"fbe.regex_quick.word", L"word character", L"\\w", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\W", L"fbe.regex_quick.not_word", L"not a word character", L"\\W", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\p{L}", L"fbe.regex_quick.unicode_letter", L"Unicode letter", L"\\p{L}", 5, 3, 1, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\p{N}", L"fbe.regex_quick.unicode_number", L"Unicode number", L"\\p{N}", 5, 3, 1, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\P{...}", L"fbe.regex_quick.unicode_not_property", L"not a Unicode property", L"\\P{}", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\xNN", L"fbe.regex_quick.hex", L"two-digit hexadecimal character", L"\\x00", 3, 2, 2, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\x{NNNN}", L"fbe.regex_quick.hex", L"Unicode hexadecimal character", L"\\x{}", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"[abc]", L"fbe.regex_quick.character_class", L"character class", L"[]", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"[^abc]", L"fbe.regex_quick.negated_class", L"negated character class", L"[^]", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"[a-z]", L"fbe.regex_quick.range_class", L"character range", L"[]", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"^", L"fbe.regex_quick.line_start", L"start of line", L"^", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"$", L"fbe.regex_quick.line_end", L"end of line", L"$", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\A", L"fbe.regex_quick.subject_start", L"start of subject", L"\\A", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\z", L"fbe.regex_quick.subject_end", L"absolute end of subject", L"\\z", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\b", L"fbe.regex_quick.word_boundary", L"word boundary", L"\\b", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\B", L"fbe.regex_quick.not_word_boundary", L"not a word boundary", L"\\B", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"a|b", L"fbe.regex_quick.alternative", L"alternative", L"|", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        // PCRE2-16 quantifiers, groups and inline options.
        { L"?", L"fbe.regex_quick.zero_or_one", L"zero or one", L"?", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"*", L"fbe.regex_quick.zero_or_more", L"zero or more", L"*", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"+", L"fbe.regex_quick.one_or_more", L"one or more", L"+", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n}", L"fbe.regex_quick.exactly", L"exactly n", L"{1}", 2, 1, 1, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n,}", L"fbe.regex_quick.at_least", L"at least n", L"{1,}", 2, 1, 1, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n,m}", L"fbe.regex_quick.range", L"from n to m", L"{1,3}", 2, 1, 3, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"*?", L"fbe.regex_quick.lazy", L"lazy quantifier", L"*?", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"+?", L"fbe.regex_quick.lazy", L"lazy quantifier", L"+?", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"??", L"fbe.regex_quick.lazy", L"lazy quantifier", L"??", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n,m}?", L"fbe.regex_quick.lazy", L"lazy range", L"{1,3}?", 2, 1, 3, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"*+", L"fbe.regex_quick.possessive", L"possessive quantifier", L"*+", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"++", L"fbe.regex_quick.possessive", L"possessive quantifier", L"++", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"?+", L"fbe.regex_quick.possessive", L"possessive quantifier", L"?+", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n,m}+", L"fbe.regex_quick.possessive", L"possessive range", L"{1,3}+", 2, 1, 3, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"(...)", L"fbe.regex_quick.group", L"capturing group", L"()", 1, 1, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?:...)", L"fbe.regex_quick.non_capturing_group", L"non-capturing group", L"(?:)", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?>...)", L"fbe.regex_quick.atomic_group", L"atomic group", L"(?>)", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?=...)", L"fbe.regex_quick.lookahead", L"positive lookahead", L"(?=)", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?!...)", L"fbe.regex_quick.negative_lookahead", L"negative lookahead", L"(?!)", 3, 3, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?<=...)", L"fbe.regex_quick.lookbehind", L"positive lookbehind", L"(?<=)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?<!...)", L"fbe.regex_quick.negative_lookbehind", L"negative lookbehind", L"(?<!)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"\\1", L"fbe.regex_quick.capture", L"capture group", L"\\1", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"(?i)", L"fbe.regex_quick.ignore_case", L"ignore case", L"(?i)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?-i)", L"fbe.regex_quick.disable_option", L"case-sensitive", L"(?-i)", 5, 5, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?m)", L"fbe.regex_quick.multiline", L"multiline", L"(?m)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?-m)", L"fbe.regex_quick.disable_option", L"disable multiline", L"(?-m)", 5, 5, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?s)", L"fbe.regex_quick.dotall", L"dot matches newline", L"(?s)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?-s)", L"fbe.regex_quick.disable_option", L"dot excludes newline", L"(?-s)", 5, 5, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?x)", L"fbe.regex_quick.extended", L"extended mode", L"(?x)", 4, 4, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        { L"(?-x)", L"fbe.regex_quick.disable_option", L"disable extended mode", L"(?-x)", 5, 5, 0, SearchUiContext::Design, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Options },
        // Design replacement is FBE syntax, not PCRE2 syntax.
        { L"$0", L"fbe.regex_quick.whole_match", L"whole match", L"$0", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\0", L"fbe.regex_quick.whole_match", L"whole match", L"\\0", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"$1", L"fbe.regex_quick.capture", L"capture group", L"$1", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\1", L"fbe.regex_quick.capture", L"capture group", L"\\1", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"$+", L"fbe.regex_quick.last_capture", L"last capture", L"$+", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\U", L"fbe.regex_quick.uppercase", L"uppercase", L"\\U", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\L", L"fbe.regex_quick.lowercase", L"lowercase", L"\\L", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\T", L"fbe.regex_quick.titlecase", L"title case", L"\\T", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\Q", L"fbe.regex_quick.reset_formatting", L"reset formatting", L"\\Q", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\S", L"fbe.regex_quick.strong", L"Strong / bold", L"\\S", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        { L"\\E", L"fbe.regex_quick.emphasis", L"Emphasis / italic", L"\\E", 2, 2, 0, SearchUiContext::Design, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement },
        // Scintilla C++11 regex subset, deliberately without PCRE2-only items.
        { L".", L"fbe.regex_quick.any", L"any character", L".", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"^", L"fbe.regex_quick.line_start", L"start of line", L"^", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"$", L"fbe.regex_quick.line_end", L"end of line", L"$", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"[abc]", L"fbe.regex_quick.character_class", L"character class", L"[]", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"[^abc]", L"fbe.regex_quick.negated_class", L"negated character class", L"[^]", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\d", L"fbe.regex_quick.digit", L"digit", L"\\d", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\D", L"fbe.regex_quick.not_digit", L"not a digit", L"\\D", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\s", L"fbe.regex_quick.space", L"whitespace", L"\\s", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\S", L"fbe.regex_quick.not_space", L"not whitespace", L"\\S", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\w", L"fbe.regex_quick.word", L"word character", L"\\w", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\W", L"fbe.regex_quick.not_word", L"not a word character", L"\\W", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\b", L"fbe.regex_quick.word_boundary", L"word boundary", L"\\b", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"\\B", L"fbe.regex_quick.not_word_boundary", L"not a word boundary", L"\\B", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Characters },
        { L"*", L"fbe.regex_quick.zero_or_more", L"zero or more", L"*", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"+", L"fbe.regex_quick.one_or_more", L"one or more", L"+", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"?", L"fbe.regex_quick.zero_or_one", L"zero or one", L"?", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n}", L"fbe.regex_quick.exactly", L"exactly n", L"{1}", 2, 1, 1, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"{n,m}", L"fbe.regex_quick.range", L"from n to m", L"{1,3}", 2, 1, 3, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Quantifiers },
        { L"(...)", L"fbe.regex_quick.group", L"capturing group", L"()", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"|", L"fbe.regex_quick.alternative", L"alternative", L"|", 1, 1, 0, SearchUiContext::Source, RegexQuickReferenceMode::Search, RegexQuickReferenceCategory::Groups },
        { L"\\1", L"fbe.regex_quick.capture", L"capture group", L"\\1", 2, 2, 0, SearchUiContext::Source, RegexQuickReferenceMode::Replacement, RegexQuickReferenceCategory::Replacement }
    };
    for(size_t i = 0; i < _countof(definitions); ++i) if(definitions[i].context == context && definitions[i].mode == mode) {
        RegexQuickReferenceEntry entry = {};
        entry.displaySyntax = definitions[i].syntax; entry.descriptionKey = definitions[i].key; entry.descriptionFallback = definitions[i].fallback;
        entry.insertionText = definitions[i].insertion; entry.caretOffset = definitions[i].caret; entry.selectionStart = definitions[i].selectionStart; entry.selectionLength = definitions[i].selectionLength;
        entry.context = context; entry.mode = mode; entry.category = definitions[i].category; entries.push_back(entry);
    }
}
}
