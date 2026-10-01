#include "stdafx.h"
#include "search/SearchPresetCatalog.h"
#include "RuntimeLocalization.h"
#include "Scintilla.h"
#include <map>
#include <string>
#include <vector>
#include <iostream>

CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback) { return fallback ? CString(fallback) : CString(); }

namespace {
std::string Utf8(const CString& value) {
    const int bytes = ::WideCharToMultiByte(CP_UTF8, 0, value, value.GetLength(), NULL, 0, NULL, NULL);
    std::string result(bytes, '\0');
    if (bytes) ::WideCharToMultiByte(CP_UTF8, 0, value, value.GetLength(), &result[0], bytes, NULL, NULL);
    return result;
}
bool Matches(HWND editor, const CString& pattern, const char* subject) {
    const std::string regex = Utf8(pattern);
    ::SendMessage(editor, SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(subject));
    ::SendMessage(editor, SCI_SETTARGETSTART, 0, 0);
    ::SendMessage(editor, SCI_SETTARGETEND, ::SendMessage(editor, SCI_GETLENGTH, 0, 0), 0);
    ::SendMessage(editor, SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_MATCHCASE | SCFIND_CXX11REGEX, 0);
    return ::SendMessage(editor, SCI_SEARCHINTARGET, regex.size(), reinterpret_cast<LPARAM>(regex.c_str())) >= 0;
}
struct Fixture { const char* positive; const char* negative; };
bool Replaces(HWND editor, const CString& pattern, const CString& replacement, const char* subject, const char* expected) {
    const std::string regex = Utf8(pattern), replacementUtf8 = Utf8(replacement);
    ::SendMessage(editor, SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(subject));
    ::SendMessage(editor, SCI_SETTARGETSTART, 0, 0);
    ::SendMessage(editor, SCI_SETTARGETEND, ::SendMessage(editor, SCI_GETLENGTH, 0, 0), 0);
    ::SendMessage(editor, SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_MATCHCASE | SCFIND_CXX11REGEX, 0);
    if (::SendMessage(editor, SCI_SEARCHINTARGET, regex.size(), reinterpret_cast<LPARAM>(regex.c_str())) < 0) return false;
    ::SendMessage(editor, SCI_REPLACETARGET, replacementUtf8.size(), reinterpret_cast<LPARAM>(replacementUtf8.c_str()));
    const sptr_t length = ::SendMessage(editor, SCI_GETLENGTH, 0, 0);
    std::string actual(static_cast<size_t>(length) + 1, '\0');
    ::SendMessage(editor, SCI_GETTEXT, actual.size(), reinterpret_cast<LPARAM>(&actual[0]));
    actual.resize(static_cast<size_t>(length));
    return actual == expected;
}
struct ReplacementFixture { const char* input; const char* expected; };
const std::map<std::wstring, ReplacementFixture> kReplacementFixtures = {
 {L"source_trim_trailing", {"<p>x   ", "<p>x"}},
 {L"source_trim_before_close", {"<empty-line />", "<empty-line/>"}},
 {L"source_blank_indentation", {"\t   \n<p>x</p>", "\n<p>x</p>"}}
};
const std::map<std::wstring, Fixture> kFixtures = {
 {L"source_trim_trailing", {"<p>x   \n", "<p>x\n"}},
 {L"source_trim_before_close", {"<empty-line />", "<empty-line/>"}},
 {L"source_blank_indentation", {"\t   \n<p>x</p>", "<p>x</p>"}},
 {L"source.repeated-punctuation", {"<p>What??</p>", "<p>What?</p>"}},
 {L"source_empty_paragraph", {"<p> \t</p>", "<p>text</p>"}},
 {L"source_double_empty_line", {"<empty-line/> <empty-line/>", "<empty-line/>\n<p>x</p>"}},
 {L"source_empty_metadata", {"<book-title> </book-title>", "<book-title>Book</book-title>"}},
 {L"source_empty_inline", {"<strong> </strong>", "<strong>text</strong>"}},
 {L"source_nested_inline", {"<emphasis><emphasis>x</emphasis>", "<emphasis><strong>x</strong>"}},
 {L"source_legacy_html_tags", {"<div>legacy</div>", "<section>ok</section>"}},
 {L"source_uppercase_tags", {"<P>legacy</P>", "<p>ok</p>"}},
 {L"source_nbsp_entity", {"<p>&nbsp;</p>", "<p> </p>"}},
 {L"source_numeric_entity", {"<p>&#160;</p>", "<p>text</p>"}},
 {L"source_empty_id", {"<section id=\"\">", "<section id=\"s1\">"}},
 {L"source_external_link", {"<a l:href=\"https://example.test\">x</a>", "<a l:href=\"#local\">x</a>"}},
 {L"source_empty_href", {"<a l:href=\"\">x</a>", "<a l:href=\"#x\">x</a>"}},
 {L"source_undefined_href", {"<a xlink:href=\"#undefined\">x</a>", "<a xlink:href=\"#x\">x</a>"}},
 {L"source_bookmark_href", {"<a l:href=\"#bookmark1\">x</a>", "<a l:href=\"#note1\">x</a>"}},
 {L"source_word_return_href", {"<a l:href=\"#_ftnref1\">x</a>", "<a l:href=\"#note1\">x</a>"}},
 {L"source_note_link", {"<a l:href=\"#note1\" type=\"note\">x</a>", "<a l:href=\"#note1\">x</a>"}},
 {L"source_note_marker", {"<p>[12]</p>", "<p>[note]</p>"}},
 {L"source_default_metadata", {"<first-name>Your</first-name>", "<first-name>John</first-name>"}},
 {L"source_undefined_image", {"<image xlink:href=\"#undefined\"/>", "<image xlink:href=\"#cover\"/>"}},
 {L"source_windows_1251", {"<?xml version=\"1.0\" encoding=\"windows-1251\"?>", "<?xml version=\"1.0\" encoding=\"utf-8\"?>"}}
};
}
int wmain() {
    HMODULE scintilla = ::LoadLibrary(L"Scintilla.dll"); HMODULE lexilla = ::LoadLibrary(L"Lexilla.dll"); if (!scintilla || !lexilla) return 1;
    HWND editor = ::CreateWindowEx(0, L"Scintilla", L"", WS_POPUP, 0, 0, 1, 1, NULL, NULL, ::GetModuleHandle(NULL), NULL);
    if (!editor) return 2;
    ::SendMessage(editor, SCI_SETCODEPAGE, SC_CP_UTF8, 0);    std::vector<FbeSearchPresets::SearchPreset> presets;
    FbeSearchPresets::GetBuiltInPresets(FbeSearchPresets::SearchUiContext::Source, false, presets);
    for (size_t i = 0; i < presets.size(); ++i) {
        const std::map<std::wstring, Fixture>::const_iterator fixture = kFixtures.find(static_cast<LPCWSTR>(presets[i].id));
        const bool hasFixture = fixture != kFixtures.end();
        const bool positive = hasFixture && Matches(editor, presets[i].findText, fixture->second.positive);
        const bool negative = hasFixture && Matches(editor, presets[i].findText, fixture->second.negative);
        const bool noteOrders = presets[i].id != L"source_note_link" ||
            (Matches(editor, presets[i].findText, "<a type=\"note\" l:href=\"#note1\">x</a>") &&
             Matches(editor, presets[i].findText, "<a l:href=\"#note1\" type=\"note\">x</a>") &&
             Matches(editor, presets[i].findText, "<a type=\"note\" xlink:href=\"#note1\">x</a>") &&
             Matches(editor, presets[i].findText, "<a xlink:href=\"#note1\" type=\"note\">x</a>"));
        const std::map<std::wstring, ReplacementFixture>::const_iterator replacement = kReplacementFixtures.find(static_cast<LPCWSTR>(presets[i].id));
        const bool replacementOk = !presets[i].hasReplacement ||
            (replacement != kReplacementFixtures.end() && Replaces(editor, presets[i].findText, presets[i].replacementText, replacement->second.input, replacement->second.expected));
        if (!hasFixture || !positive || negative || !noteOrders || !replacementOk) {
            std::wcerr << L"Source Scintilla fixture failed: " << static_cast<LPCWSTR>(presets[i].id) << L" positive=" << positive << L" negative=" << negative << L" noteOrders=" << noteOrders << L" replacement=" << replacementOk << std::endl; return 3;
        }
    }
    if (kFixtures.size() != presets.size() || kReplacementFixtures.size() != 3) return 4;
    ::DestroyWindow(editor); ::FreeLibrary(lexilla); ::FreeLibrary(scintilla); return 0;
}