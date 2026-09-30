#include "stdafx.h"
#include "..\\..\\resource.h"
#include "RegexHelpDialog.h"
#include "..\\..\\RuntimeLocalization.h"
#include "..\\..\\ThemeManager.h"

namespace
{
CString HelpText(FbeSearchPresets::SearchUiContext context)
{
    if (context == FbeSearchPresets::SearchUiContext::Source)
        return FbeLoadRuntimeStringByKey(L"fbe.regex_help.source.text",
            L"Regular expressions — Source/Code\r\n\r\n"
            L"Engine: Scintilla regular expressions / C++11 regex mode\r\n"
            L"Flags: SCFIND_REGEXP | SCFIND_CXX11REGEX\r\n\r\n"
            L"Supported common syntax: .  ^  $  [...]  \\d  \\s  \\w  \\b  *  +  ?  {n,m}  (...)  |\r\n\r\n"
            L"Replacement uses Scintilla. Back-reference \\1 is supported.\r\n\r\n"
            L"Unicode (UCP) applies to PCRE2 in Design mode and is unavailable for the current Source regex engine.");
    return FbeLoadRuntimeStringByKey(L"fbe.regex_help.design.text",
        L"Regular expressions — Design\r\n\r\n"
        L"Engine: PCRE2-16\r\nUTF is always enabled. Unicode (UCP) is enabled by the Unicode (UCP) checkbox.\r\n\r\n"
        L"Syntax: .  ^  $  [...]  [^...]  \\d  \\D  \\s  \\S  \\w  \\W  \\b  \\B  *  +  ?  *?  +?  ??  {n}  {n,}  {n,m}\r\n"
        L"Groups and alternatives: (...)  (?:...)  |  (?=...)  (?!...)  (?<=...)  (?<!...)\r\n"
        L"Unicode: \\p{...}  \\P{...}  \\R\r\n\r\n"
        L"Replace syntax is implemented by FBE, not PCRE2.\r\n"
        L"$0 or \\0 = whole match; $1..$9 or \\1..\\9 = capture groups; $+ or \\+ = last capture.\r\n"
        L"\\U uppercase, \\L lowercase, \\T title case, \\Q ends formatting; \\S Strong/Bold; \\E Emphasis/Italic.\r\n"
        L"Only groups 1..9 are available in Replace. Cross-paragraph replacement is rejected to protect the document structure.");
}

class RegexHelpDialog : public CDialogImpl<RegexHelpDialog>
{
public:
    enum { IDD = IDD_REGEX_HELP };
    explicit RegexHelpDialog(FbeSearchPresets::SearchUiContext context) : m_context(context) {}

    BEGIN_MSG_MAP(RegexHelpDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
    {
        FbeApplyRuntimeDialogLocalization(m_hWnd, IDD_REGEX_HELP);
        SetWindowText(FbeLoadRuntimeStringByKey(
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"fbe.regex_help.design.caption" : L"fbe.regex_help.source.caption",
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"Regular expression help — Design" : L"Regular expression help — Source"));
        SetDlgItemText(IDC_REGEX_HELP_TEXT, HelpText(m_context));
        ThemeManager::ApplyToWindow(m_hWnd);
        return TRUE;
    }

    LRESULT OnClose(WORD, WORD, HWND, BOOL&) { EndDialog(IDCANCEL); return 0; }

private:
    FbeSearchPresets::SearchUiContext m_context;
};
}

void ShowRegexHelpDialog(HWND owner, FbeSearchPresets::SearchUiContext context)
{
    RegexHelpDialog dialog(context);
    dialog.DoModal(owner);
}