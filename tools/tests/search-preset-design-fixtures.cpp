#include "stdafx.h"
#include "search/SearchPresetCatalog.h"
#include "RuntimeLocalization.h"
#include <map>
#include <iostream>
#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC
#include "pcre2.h"
CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback) { return fallback ? CString(fallback) : CString(); }
namespace {
using FbeSearchPresets::SearchPreset;
struct Fixture { LPCWSTR positive; LPCWSTR negative; LPCWSTR output; };
const std::map<std::wstring, Fixture> kFixtures = {
 {L"design.normalize-spaces", {L"one   two", L"one two", L"one two"}}, {L"design.trim-before-punctuation", {L"word ,", L"word,", L"word,"}},
 {L"design.trim-leading", {L"  word", L"word", L"word"}}, {L"design.trim-trailing", {L"word  ", L"word", L"word"}}, {L"design.tabs-to-spaces", {L"a\tb", L"a b", L"a b"}},
 {L"design.nbsp-to-space", {L"a\x00A0" L"b", L"a b", L"a b"}}, {L"design_trim_after_opening", {L"( word", L"(word", L"(word"}}, {L"design_trim_before_closing", {L"word )", L"word)", L"word)"}},
 {L"design_trim_before_period", {L"word .", L"word.", L"word."}}, {L"design_ellipsis_three_dots", {L"wait...", L"wait..", L"wait…"}}, {L"design_ellipsis_spaced_dots", {L"wait. . .", L"wait..", L"wait…"}},
 {L"design_number_nbsp", {L"№ 12", L"№12", L"№\x00A0" L"12"}}, {L"design_section_nbsp", {L"§ 12", L"§12", L"§\x00A0" L"12"}},
 {L"design.duplicate-word", {L"тест тест", L"тест другой", NULL}}, {L"design.repeated-punctuation", {L"What??", L"What?", NULL}}, {L"design_hidden_characters", {L"a\x200B" L"b", L"a b", NULL}},
 {L"design_mixed_alphabets", {L"Тeст", L"Тест", NULL}}, {L"design_lower_to_upper_inside_word", {L"abC", L"abc", NULL}}, {L"design_digit_inside_word", {L"a1b", L"a1", NULL}},
 {L"design_punctuation_inside_word", {L"a,b", L"a, b", NULL}}, {L"design_uppercase_before_lowercase", {L"OCRword", L"Word", NULL}}, {L"design_apostrophe_cyrillic", {L"'тест", L"'Test", NULL}},
 {L"design_paragraph_starts_lowercase", {L"lower start", L"Upper start", NULL}}, {L"design_paragraph_missing_final_punctuation", {L"текст]", L"текст.", NULL}}, {L"design_lowercase_after_sentence", {L"One. next", L"One. Next", NULL}},
 {L"design_possible_missing_period", {L"word Next", L"word next", NULL}}, {L"design_repeated_quotes", {L"\"\"", L"\"one\"", NULL}}, {L"design_repeated_terminal_punctuation", {L"word,,", L"word,", NULL}},
 {L"design_straight_double_quotes", {L"\"word\"", L"«word»", NULL}}, {L"design_spaced_hyphen", {L"word - word", L"word-word", NULL}}, {L"design_number_ranges", {L"1 - 2", L"1–2", NULL}},
 {L"design_thousands_space", {L"1 000", L"1000", NULL}}, {L"design_initials_before_name", {L"И. И. Иванов", L"Иванов И.И.", NULL}}, {L"design_initials_after_name", {L"Иванов И.И.", L"Иванов Иван", NULL}},
 {L"design_roman_cyrillic_ha", {L"IХ", L"IX", NULL}}
};
struct FbeMatch
{
    size_t start;
    size_t length;
    CString value;
    std::vector<CString> submatches;
};

// Mirrors FBEview.cpp:GetReplStr for the replacement syntax used by Find/Replace.
// PCRE2 supplies only matching; replacement expansion deliberately follows FBE.
struct ReplacementRun
{
    int flags;
    int start;
    int length;
};

CString GetReplStr(const CString& rstr, const FbeMatch& match)
{
    enum { Strong = 1, Emphasis = 2, Upper = 4, Lower = 8, Title = 16 };
    CString result;
    CString value;
    bool emptyParameter = false;
    int flags = 0;
    ReplacementRun current = {};
    std::vector<ReplacementRun> runs;
    for (int index = 0; index < rstr.GetLength(); ++index)
    {
        if ((rstr[index] == L'$' || rstr[index] == L'\\') && index < rstr.GetLength() - 1)
        {
            const wchar_t marker = rstr[++index];
            if (marker == L'0') value = match.value;
            else if (marker == L'+') value = match.submatches.empty() ? CString() : match.submatches.back();
            else if (marker >= L'1' && marker <= L'9')
            {
                const size_t group = static_cast<size_t>(marker - L'1');
                value = group < match.submatches.size() ? match.submatches[group] : CString();
                emptyParameter = value.IsEmpty();
            }
            else if (marker == L'T') { flags |= Title; continue; }
            else if (marker == L'U') { flags |= Upper; continue; }
            else if (marker == L'L') { flags |= Lower; continue; }
            else if (marker == L'S') { flags |= Strong; continue; }
            else if (marker == L'E') { flags |= Emphasis; continue; }
            else if (marker == L'Q') { flags = 0; continue; }
            else continue;
        }
        if (current.flags != flags && current.flags && current.start < result.GetLength())
        {
            current.length = result.GetLength() - current.start;
            runs.push_back(current);
            current.flags = 0;
        }
        if (flags)
        {
            current.flags = flags;
            current.start = result.GetLength();
        }
        if (!emptyParameter)
        {
            if (!value.IsEmpty()) { result += value; value.Empty(); }
            else result += rstr[index];
        }
        else emptyParameter = false;
    }
    if (current.flags && current.start < result.GetLength())
    {
        current.length = result.GetLength() - current.start;
        runs.push_back(current);
    }
    TCHAR* characters = result.GetBuffer(result.GetLength());
    for (std::vector<ReplacementRun>::const_iterator run = runs.begin(); run != runs.end(); ++run)
    {
        if (run->flags & Upper) CharUpperBuff(characters + run->start, run->length);
        else if (run->flags & Lower) CharLowerBuff(characters + run->start, run->length);
        else if ((run->flags & Title) && run->length > 0)
        {
            CharUpperBuff(characters + run->start, 1);
            CharLowerBuff(characters + run->start + 1, run->length - 1);
        }
    }
    result.ReleaseBuffer(result.GetLength());
    return result;
}
bool Match(const SearchPreset& preset, LPCWSTR subject, CString* replacementResult = NULL) {
 int error=0; PCRE2_SIZE offset=0; const uint32_t flags=PCRE2_UTF|(preset.unicodeProperties?PCRE2_UCP:0);
 pcre2_code* code=pcre2_compile((PCRE2_SPTR)(LPCWSTR)preset.findText,preset.findText.GetLength(),flags,&error,&offset,NULL); if(!code)return false;
 pcre2_match_data* data=pcre2_match_data_create_from_pattern(code,NULL); int result=data?pcre2_match(code,(PCRE2_SPTR)subject,wcslen(subject),0,0,data,NULL):PCRE2_ERROR_NOMEMORY;
 if (result >= 0 && replacementResult)
 {
     std::vector<FbeMatch> matches;
     PCRE2_SIZE searchOffset = 0;
     while (true)
     {
         const int count = pcre2_match(code, (PCRE2_SPTR)subject, wcslen(subject), searchOffset, 0, data, NULL);
         if (count < 0) break;
         PCRE2_SIZE* vector = pcre2_get_ovector_pointer(data);
         FbeMatch match;
         match.start = static_cast<size_t>(vector[0]);
         match.length = static_cast<size_t>(vector[1] - vector[0]);
         match.value = CString(subject + vector[0], static_cast<int>(match.length));
         for (int group = 1; group < count; ++group)
         {
             const PCRE2_SIZE groupStart = vector[group * 2];
             const PCRE2_SIZE groupEnd = vector[group * 2 + 1];
             match.submatches.push_back(groupStart == PCRE2_UNSET ? CString() : CString(subject + groupStart, static_cast<int>(groupEnd - groupStart)));
         }
         matches.push_back(match);
         searchOffset = vector[1];
         if (searchOffset >= wcslen(subject)) break;
     }
     CString output(subject);
     for (std::vector<FbeMatch>::reverse_iterator match = matches.rbegin(); match != matches.rend(); ++match)
     {
         output.Delete(static_cast<int>(match->start), static_cast<int>(match->length));
         output.Insert(static_cast<int>(match->start), GetReplStr(preset.replacementText, *match));
     }
     *replacementResult = output;
 }
 if(data)pcre2_match_data_free(data); pcre2_code_free(code); return result>=0;
}
}
int wmain(){std::vector<SearchPreset> presets;FbeSearchPresets::GetBuiltInPresets(FbeSearchPresets::SearchUiContext::Design,false,presets);for(size_t i=0;i<presets.size();++i){std::map<std::wstring,Fixture>::const_iterator it=kFixtures.find((LPCWSTR)presets[i].id);if(it==kFixtures.end()||!Match(presets[i],it->second.positive)||Match(presets[i],it->second.negative)){std::wcerr<<L"Design fixture failed: "<<(LPCWSTR)presets[i].id<<std::endl;return 1;}if(presets[i].hasReplacement){CString output;if(!it->second.output||!Match(presets[i],it->second.positive,&output)||output!=it->second.output){std::wcerr<<L"Design replacement failed: "<<(LPCWSTR)presets[i].id<<std::endl;return 2;}}}return kFixtures.size()==presets.size()?0:3;}