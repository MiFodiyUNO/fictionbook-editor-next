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
        return FbeLoadRuntimeStringByKey(L"fbe.regex_help.source.text.detail",
            L"Regular expressions — Source/Code\r\n\r\nEngine\r\nScintilla regular expressions / C++11 regex mode\r\n"
            L"Flags: SCFIND_REGEXP | SCFIND_CXX11REGEX\r\n\r\nSupported syntax\r\nOnly the documented Scintilla C++11 subset is available.\r\n\r\n"
            L"Classes\r\n[abc], [^abc], [a-z], \\d, \\D, \\s, \\S, \\w and \\W.\r\n\r\nAnchors\r\n^ and $ match line boundaries; \\b and \\B match word boundaries.\r\n\r\n"
            L"Quantifiers\r\nUse *, +, ?, {n} and {n,m}.\r\n\r\nGroups and alternatives\r\nCapturing groups (...) and alternation | are supported.\r\n\r\n"
            L"Back-references\r\nUse \\1 in a replacement for the first captured group.\r\n\r\nReplacement\r\nScintilla performs replacement.\r\n\r\n"
            L"Examples\r\nDigits: Find \\d+.\r\n\r\nLimitations\r\nThis is not PCRE2; Unicode (UCP) is only for PCRE2 in Design mode.");
    }
    // The advanced entry is the complete Design manual and prevents base/advanced duplication.
    return FbeLoadRuntimeStringByKey(L"fbe.regex_help.design.advanced",
        L"Regular expressions — Design\r\n\r\nEngine\r\nPCRE2-16\r\n\r\nEscaping and classes\r\nUse \\ to quote metacharacters.\r\n\r\nUnicode and UCP\r\nUTF is always on; enable UCP for Unicode properties.\r\n\r\n"
        L"Anchors\r\n^/$, \\A/\\z and \\b/\\B.\r\n\r\nQuantifiers\r\n* + ? and {n,m}; lazy and possessive forms are available.\r\n\r\n"
        L"Groups, named groups and alternatives\r\nCapturing, non-capturing, atomic and named groups are supported.\r\n\r\nLookaround\r\nLookahead and lookbehind are available.\r\n\r\n"
        L"Inline options\r\n(?i), (?m), (?s) and (?x).\r\n\r\nAdvanced PCRE2\r\nCompile-tested advanced constructs are available.\r\n\r\n"
        L"Replacement in FBE\r\n$0/\\0 and capture references are supported.\r\n\r\nExamples\r\nUse [ \\t]{2,} for repeated spaces.\r\n\r\nLimitations\r\nOnly groups 1..9 are replaceable.");
}

enum class HelpLineKind { Title, Heading, Body, Syntax, Example, Note };

HelpLineKind ClassifyHelpLine(size_t index, bool beginsBlock, int section, FbeSearchPresets::SearchUiContext context)
{
    // The catalog has an explicit blank-line-separated structure.  Formatting
    // follows the section position, never a guess based on regex punctuation.
    if (index == 0) return HelpLineKind::Title;
    if (beginsBlock) return HelpLineKind::Heading;
    const int examples = context == FbeSearchPresets::SearchUiContext::Source ? 9 : 14;
    const int limitations = examples + 1;
    if (section == examples) return HelpLineKind::Example;
    if (section == limitations) return HelpLineKind::Note;
    return section >= 2 ? HelpLineKind::Syntax : HelpLineKind::Body;
}

void ApplyParagraphHeadingStyle(HWND text, const CString& help, FbeSearchPresets::SearchUiContext context)
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
    CHARFORMAT2 syntax = {}; syntax.cbSize = sizeof(syntax); syntax.dwMask = CFM_FACE; ::lstrcpynW(syntax.szFaceName, L"Consolas", LF_FACESIZE);
    PARAFORMAT2 headingParagraph = {}; headingParagraph.cbSize = sizeof(headingParagraph); headingParagraph.dwMask = PFM_SPACEBEFORE | PFM_SPACEAFTER; headingParagraph.dySpaceBefore = 100; headingParagraph.dySpaceAfter = 40;
    PARAFORMAT2 exampleParagraph = {}; exampleParagraph.cbSize = sizeof(exampleParagraph); exampleParagraph.dwMask = PFM_STARTINDENT | PFM_SPACEAFTER; exampleParagraph.dxStartIndent = 180; exampleParagraph.dySpaceAfter = 40;
    int section = 0;
    for (size_t index = 0; index < lines.size(); ++index)
    {
        const bool beginsBlock = index > 0 && lines[index - 1].IsEmpty();
        if (beginsBlock) ++section;
        const HelpLineKind kind = ClassifyHelpLine(index, beginsBlock, section, context);
        if (lines[index].IsEmpty()) continue;
        ::SendMessage(text, EM_SETSEL, starts[index], starts[index] + lines[index].GetLength());
        if (kind == HelpLineKind::Title || kind == HelpLineKind::Heading)
        {
            ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(kind == HelpLineKind::Title ? &title : &heading));
            ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&headingParagraph));
        }
        else if (kind == HelpLineKind::Syntax || kind == HelpLineKind::Example)
        {
            ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&syntax));
            if (kind == HelpLineKind::Example) ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&exampleParagraph));
        }
        else if (kind == HelpLineKind::Note)
        {
            ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&headingParagraph));
        }
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
        ApplyParagraphHeadingStyle(text, help, m_context);
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
