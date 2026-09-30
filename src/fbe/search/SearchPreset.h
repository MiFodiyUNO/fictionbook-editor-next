#pragma once

#include <atlstr.h>

namespace FbeSearchPresets
{
// The body editor uses PCRE2, while Source uses Scintilla's C++11 regex mode.
// Keep this explicit: neither presets nor help infer it from an HWND.
enum class SearchUiContext
{
    Design,
    Source
};

struct SearchPreset
{
    CString id;
    CString name;
    CString description;
    CString findText;
    CString replacementText;
    bool hasReplacement;
    bool regexp;
    bool matchCase;
    bool wholeWord;
    bool unicodeProperties;
    SearchUiContext context;
    bool builtIn;

    SearchPreset()
        : hasReplacement(false), regexp(false), matchCase(false), wholeWord(false),
          unicodeProperties(false), context(SearchUiContext::Design), builtIn(false) {}
};
}