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
        return FbeLoadRuntimeStringByKey(L"fbe.regex_help.source.text.detail",
            L"Regular expressions — Source/Code\r\n\r\nEngine\r\n"
            L"Scintilla regular expressions / C++11 regex mode\r\n"
            L"Flags: SCFIND_REGEXP | SCFIND_CXX11REGEX\r\n\r\n"
            L"Basic syntax\r\n.  ^  $  [...]  \\d  \\s  \\w  \\b  *  +  ?  {n,m}  (...)  |\r\n\r\n"
            L"Replacement\r\nScintilla performs replacement. Back-reference \\1 is supported.\r\n\r\n"
            L"Limitations\r\nUnicode (UCP) applies to PCRE2 in Design mode and is unavailable for the current Source regex engine.");
    return FbeLoadRuntimeStringByKey(L"fbe.regex_help.design.text.detail",
        L"Regular expressions — Design\r\n\r\nEngine\r\nPCRE2-16\r\n"
        L"UTF is always enabled. Unicode (UCP) is enabled by the Unicode (UCP) checkbox.\r\n\r\n"
        L"Basic syntax\r\n.  ^  $  [...]  [^...]  \\d  \\D  \\s  \\S  \\w  \\W  \\b  \\B  *  +  ?  *?  +?  ??  {n}  {n,}  {n,m}\r\n\r\n"
        L"Groups\r\n(...)  (?:...)  |  (?=...)  (?!...)  (?<=...)  (?<!...)\r\n\r\n"
        L"Unicode\r\n\\p{...}  \\P{...}  \\R\r\n\r\n"
        L"Replacement in FBE\r\nFBE implements replacement syntax, not PCRE2.\r\n"
        L"$0 or \\0 = whole match; $1..$9 or \\1..\\9 = capture groups; $+ or \\+ = last capture.\r\n"
        L"\\U uppercase, \\L lowercase, \\T title case, \\Q ends formatting; \\S Strong/Bold; \\E Emphasis/Italic.\r\n\r\n"
        L"Limitations\r\nOnly groups 1..9 are available in Replace. Cross-paragraph replacement is rejected to protect the document structure.");
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
        const HWND text = GetDlgItem(IDC_REGEX_HELP_TEXT);
        if (text)
        {
            ::SendMessage(text, EM_SETSEL, 0, 0);
            ::SendMessage(text, EM_SCROLLCARET, 0, 0);
        }
        ThemeManager::ApplyToWindow(m_hWnd);
        ::SetFocus(GetDlgItem(IDCANCEL));
        return FALSE;
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
