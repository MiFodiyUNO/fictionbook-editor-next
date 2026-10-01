#include "stdafx.h"
#include "..\\..\\resource.h"
#include "RegexHelpDialog.h"
#include "..\\..\\RuntimeLocalization.h"
#include "..\\..\\Settings.h"
#include "..\\..\\ThemeManager.h"
#include "..\\..\\UiMetrics.h"
#include <richedit.h>
#include <vector>

extern CSettings _Settings;

namespace
{
CString HelpText(FbeSearchPresets::SearchUiContext context)
{
    if (context == FbeSearchPresets::SearchUiContext::Source)
    {
        CString detail = FbeLoadRuntimeStringByKey(L"fbe.regex_help.source.text.detail",
            L"Regular expressions — Source/Code\r\n\r\nEngine\r\n"
            L"Scintilla regular expressions / C++11 regex mode\r\n"
            L"Flags: SCFIND_REGEXP | SCFIND_CXX11REGEX\r\n\r\n"
            L"Basic syntax\r\n.  ^  $  [...]  \\d  \\s  \\w  \\b  *  +  ?  {n,m}  (...)  |\r\n\r\n"
            L"Replacement\r\nScintilla performs replacement. Back-reference \\1 is supported.\r\n\r\n"
            L"Limitations\r\nUnicode (UCP) applies to PCRE2 in Design mode and is unavailable for the current Source regex engine.");
        const CString advanced = FbeLoadRuntimeStringByKey(L"fbe.regex_help.source.advanced",
            L"More Source examples\r\n\r\nPractical recipes\r\nDigits: \\d+.\r\nRepeated spaces: [ \\t]{2,}.\r\nA word: (\\w+) then \\1 in replacement.\r\n\r\nLimitations\r\nOnly the documented Scintilla C++11 subset is available.");
        if (!advanced.IsEmpty()) detail += L"\r\n\r\n" + advanced;
        return detail;
    }
    CString detail = FbeLoadRuntimeStringByKey(L"fbe.regex_help.design.text.detail",
        L"Regular expressions — Design\r\n\r\nEngine\r\nPCRE2-16\r\n\r\nBasic syntax\r\n.  ^  $  [...]  [^...]  \\d  \\D  \\s  \\S  \\w  \\W  \\b  \\B  *  +  ?  *?  +?  ??  {n}  {n,}  {n,m}\r\n\r\nGroups\r\n(...)  (?:...)  |  (?=...)  (?!...)  (?<=...) (?<!...)\r\n\r\nReplacement in FBE\r\n$0 or \\0 = whole match; $1..$9 or \\1..\\9 = capture groups.\r\n\r\nLimitations\r\nOnly groups 1..9 are available in Replace. Cross-paragraph replacement is rejected.");
    const CString advanced = FbeLoadRuntimeStringByKey(L"fbe.regex_help.design.advanced",
        L"Advanced PCRE2\r\n\\K, \\G, named groups, branch reset, conditional and subroutine calls are supported and compile-tested.");
    if (!advanced.IsEmpty()) detail += L"\r\n\r\n" + advanced;
    return detail;
}
enum class HelpLineKind { Title, Heading, Body, Syntax, Example, Note };

HelpLineKind ClassifyHelpLine(const CString& line, size_t index, bool beginsBlock)
{
    // Help catalog entries use explicit blank-line-separated blocks: the first
    // line is the title and each later block begins with a heading.  This keeps
    // formatting locale-neutral and deliberately does not infer code from
    // characters such as a backslash or a square bracket.
    if (index == 0) return HelpLineKind::Title;
    if (!line.IsEmpty() && beginsBlock) return HelpLineKind::Heading;
    if (line.Left(8) == L"Example:" || line.Left(9) == L"Пример:") return HelpLineKind::Example;
    if (line.Left(12) == L"Limitations:" || line.Left(13) == L"Ограничения:") return HelpLineKind::Note;
    return HelpLineKind::Body;
}
void ApplyParagraphHeadingStyle(HWND text, const CString& help)
{
    std::vector<CString> lines;
    std::vector<int> starts;
    int start = 0;
    while (start < help.GetLength())
    {
        int end = help.Find(L'\n', start); if (end < 0) end = help.GetLength();
        int contentEnd = end; if (contentEnd > start && help[contentEnd - 1] == L'\r') --contentEnd;
        lines.push_back(help.Mid(start, contentEnd - start)); starts.push_back(start); start = end + 1;
    }
    CHARFORMAT2 heading = {}; heading.cbSize = sizeof(heading); heading.dwMask = CFM_BOLD; heading.dwEffects = CFE_BOLD;
    CHARFORMAT2 title = heading; title.dwMask |= CFM_SIZE; title.yHeight = 220;
    PARAFORMAT2 paragraph = {}; paragraph.cbSize = sizeof(paragraph); paragraph.dwMask = PFM_SPACEBEFORE | PFM_SPACEAFTER; paragraph.dySpaceBefore = 100; paragraph.dySpaceAfter = 40;
    for (size_t index = 0; index < lines.size(); ++index)
    {
        const bool beginsBlock = index > 0 && lines[index - 1].IsEmpty();
        const HelpLineKind kind = ClassifyHelpLine(lines[index], index, beginsBlock);
        if (kind != HelpLineKind::Title && kind != HelpLineKind::Heading) continue;
        ::SendMessage(text, EM_SETSEL, starts[index], starts[index] + lines[index].GetLength());
        ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(kind == HelpLineKind::Title ? &title : &heading));
        ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&paragraph));
    }
    ::SendMessage(text, EM_SETSEL, 0, 0); ::SendMessage(text, EM_SCROLLCARET, 0, 0);
}
class RegexHelpDialog : public CDialogImpl<RegexHelpDialog>
{
public:
    enum { IDD = IDD_REGEX_HELP };
    explicit RegexHelpDialog(FbeSearchPresets::SearchUiContext context) : m_context(context) {}

    BEGIN_MSG_MAP(RegexHelpDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_CLOSE, OnWindowClose)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnThemeChanged)
        MESSAGE_HANDLER(WM_FBE_THEMECHANGED, OnThemeChanged)
        COMMAND_ID_HANDLER(IDC_REGEX_HELP_CLOSE, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
    {
        FbeApplyRuntimeDialogLocalization(m_hWnd, IDD_REGEX_HELP);
        SetWindowText(FbeLoadRuntimeStringByKey(
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"fbe.regex_help.design.caption" : L"fbe.regex_help.source.caption",
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"Regular expression help — Design" : L"Regular expression help — Source"));

        const CString help = HelpText(m_context);
        SetDlgItemText(IDC_REGEX_HELP_TEXT, help);
        const HWND text = GetDlgItem(IDC_REGEX_HELP_TEXT);
        if (text) ApplyTheme(text, help);
        ThemeManager::ApplyToWindow(m_hWnd);
        CaptureLayoutMetrics();
        RestoreSize();
        LayoutControls();
        ::SetFocus(GetDlgItem(IDC_REGEX_HELP_CLOSE));
        return FALSE;
    }

    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&) { if (m_layoutReady) LayoutControls(); return 0; }
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL&)
    {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        if (info != NULL) { info->ptMinTrackSize.x = m_minimumSize.cx; info->ptMinTrackSize.y = m_minimumSize.cy; }
        return 0;
    }
    LRESULT OnWindowClose(UINT, WPARAM, LPARAM, BOOL&) { SaveSize(); EndDialog(IDC_REGEX_HELP_CLOSE); return 0; }
    LRESULT OnThemeChanged(UINT, WPARAM, LPARAM, BOOL&) { ThemeManager::ApplyToWindow(m_hWnd); ApplyTheme(GetDlgItem(IDC_REGEX_HELP_TEXT), HelpText(m_context)); return 0; }
    LRESULT OnClose(WORD, WORD, HWND, BOOL&) { SaveSize(); EndDialog(IDC_REGEX_HELP_CLOSE); return 0; }

private:
    void ApplyTheme(HWND text, const CString& help)
    {
        if (!text) return;
        ::SendMessage(text, EM_SETBKGNDCOLOR, 0, ThemeManager::ControlColor());
        CHARFORMAT2 body = {}; body.cbSize = sizeof(body); body.dwMask = CFM_COLOR; body.crTextColor = ThemeManager::TextColor();
        ::SendMessage(text, EM_SETSEL, 0, -1);
        ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&body));
        ApplyParagraphHeadingStyle(text, help);
    }
    void CaptureLayoutMetrics()
    {
        RECT window = {};
        GetWindowRect(&window);
        m_minimumSize = CSize(window.right - window.left, window.bottom - window.top);

        RECT client = {}; GetClientRect(&client);
        RECT text = {}; ::GetWindowRect(GetDlgItem(IDC_REGEX_HELP_TEXT), &text);
        ::MapWindowPoints(NULL, m_hWnd, reinterpret_cast<POINT*>(&text), 2);
        RECT close = {}; ::GetWindowRect(GetDlgItem(IDC_REGEX_HELP_CLOSE), &close);
        ::MapWindowPoints(NULL, m_hWnd, reinterpret_cast<POINT*>(&close), 2);
        m_margin = text.left;
        m_bottomMargin = client.bottom - close.bottom;
        m_gap = close.top - text.bottom;
        m_buttonSize = CSize(close.right - close.left, close.bottom - close.top);
        m_layoutReady = true;
    }

    void RestoreSize()
    {
        WINDOWPLACEMENT placement = {}; placement.length = sizeof(placement);
        if (!_Settings.GetRegexHelpPlacement(placement)) return;
        const int savedWidth = placement.rcNormalPosition.right - placement.rcNormalPosition.left;
        const int savedHeight = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top;
        if (savedWidth < m_minimumSize.cx || savedHeight < m_minimumSize.cy) return;
        HMONITOR monitor = ::MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = {}; info.cbSize = sizeof(info);
        if (monitor == NULL || !::GetMonitorInfo(monitor, &info)) return;
        const int width = (std::min)(savedWidth, static_cast<int>(info.rcWork.right - info.rcWork.left));
        const int height = (std::min)(savedHeight, static_cast<int>(info.rcWork.bottom - info.rcWork.top));
        int left = placement.rcNormalPosition.left;
        int top = placement.rcNormalPosition.top;
        left = (std::max)(static_cast<int>(info.rcWork.left), (std::min)(left, static_cast<int>(info.rcWork.right) - width));
        top = (std::max)(static_cast<int>(info.rcWork.top), (std::min)(top, static_cast<int>(info.rcWork.bottom) - height));
        SetWindowPos(NULL, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    void SaveSize()
    {
        WINDOWPLACEMENT placement = {}; placement.length = sizeof(placement);
        if (::GetWindowPlacement(m_hWnd, &placement)) _Settings.SetRegexHelpPlacement(placement, true);
    }

    void LayoutControls()
    {
        RECT client = {}; GetClientRect(&client);
        const int width = (std::max)(0, static_cast<int>(client.right) - static_cast<int>(client.left));
        const int height = (std::max)(0, static_cast<int>(client.bottom) - static_cast<int>(client.top));
        const int closeLeft = (std::max)(m_margin, width - m_margin - static_cast<int>(m_buttonSize.cx));
        const int closeTop = (std::max)(m_margin, height - m_bottomMargin - static_cast<int>(m_buttonSize.cy));
        const int textBottom = (std::max)(m_margin, closeTop - m_gap);
        ::SetWindowPos(GetDlgItem(IDC_REGEX_HELP_TEXT), NULL, m_margin, m_margin,
            (std::max)(0, width - 2 * m_margin), (std::max)(0, textBottom - m_margin), SWP_NOZORDER | SWP_NOACTIVATE);
        ::SetWindowPos(GetDlgItem(IDC_REGEX_HELP_CLOSE), NULL, closeLeft, closeTop, m_buttonSize.cx, m_buttonSize.cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    FbeSearchPresets::SearchUiContext m_context;
    CSize m_minimumSize = CSize(0, 0);
    CSize m_buttonSize = CSize(0, 0);
    int m_margin = 0;
    int m_bottomMargin = 0;
    int m_gap = 0;
    bool m_layoutReady = false;
};
}

void ShowRegexHelpDialog(HWND owner, FbeSearchPresets::SearchUiContext context)
{
    HMODULE richEdit = ::LoadLibraryW(L"Msftedit.dll");
    RegexHelpDialog dialog(context);
    dialog.DoModal(owner);
    if (richEdit != NULL) ::FreeLibrary(richEdit);
}
