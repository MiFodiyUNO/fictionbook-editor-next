#include "stdafx.h"
#include "..\\..\\resource.h"
#include "RegexHelpDialog.h"
#include "..\\RegexQuickReference.h"
#include "..\\..\\RuntimeLocalization.h"
#include "..\\..\\Settings.h"
#include "..\\..\\ThemeManager.h"
#include "..\\..\\UiMetrics.h"
#include <richedit.h>
#include <vector>

extern CSettings _Settings;

namespace
{
enum class HelpLineKind { Title, Heading, Body, Syntax, Example, Note };

struct HelpBlock
{
    HelpLineKind kind;
    CString text;
};

void AddHelpBlock(std::vector<HelpBlock>& blocks, HelpLineKind kind, LPCWSTR text)
{
    if(text != NULL && *text != 0) blocks.push_back(HelpBlock{ kind, CString(text) });
}

CString LocalizedHelpText(LPCWSTR key, LPCWSTR fallback)
{
    return FbeLoadRuntimeStringByKey(key, fallback);
}

void AddQuickReferenceSyntax(std::vector<HelpBlock>& blocks, FbeSearchPresets::SearchUiContext context, FbeSearchPresets::RegexQuickReferenceMode mode)
{
    std::vector<FbeSearchPresets::RegexQuickReferenceEntry> entries;
    FbeSearchPresets::GetRegexQuickReferenceEntries(context, mode, entries);
    for(size_t index = 0; index < entries.size(); ++index)
    {
        const CString description = FbeLoadRuntimeStringByKey(entries[index].descriptionKey, entries[index].descriptionFallback);
        CString line; line.Format(L"%s  —  %s", static_cast<LPCWSTR>(entries[index].displaySyntax), static_cast<LPCWSTR>(description));
        blocks.push_back(HelpBlock{ HelpLineKind::Syntax, line });
    }
}

std::vector<HelpBlock> BuildHelpBlocks(FbeSearchPresets::SearchUiContext context)
{
    std::vector<HelpBlock> blocks;
    const bool source = context == FbeSearchPresets::SearchUiContext::Source;
    AddHelpBlock(blocks, HelpLineKind::Title, FbeLoadRuntimeStringByKey(source ? L"fbe.regex_help.source.caption" : L"fbe.regex_help.design.caption", source ? L"Regular expression help — Source" : L"Regular expression help — Design"));
    if(source)
    {
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.engine", L"Engine"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.engine", L"Source search uses Scintilla regular expressions in its documented C++11 mode. This is not PCRE2."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.characters", L"Classes and escapes"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.classes", L"Use literal characters, escaping with \\, character classes [abc] and [^abc], ranges such as [a-z], and the documented \\d, \\D, \\s, \\S, \\w and \\W classes."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.anchors", L"Anchors and boundaries"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.anchors", L"^ and $ match line boundaries. \\b and \\B match word-boundary positions."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.quantifiers", L"Quantifiers"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.quantifiers", L"Use *, +, ?, {n} and {n,m} for repetition."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.groups", L"Groups, alternation and backreferences"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.groups", L"Capturing groups (...) and alternation | are supported. Use \\1 for the first captured group where Scintilla replacement accepts it."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.replacement", L"Replacement"));
        AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.source.replacement", L"Replacement is performed by Scintilla. FBE Design replacement formatting is not available here."));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.syntax", L"Syntax reference"));
        AddQuickReferenceSyntax(blocks, context, FbeSearchPresets::RegexQuickReferenceMode::Search);
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.examples", L"Practical examples"));
        AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.source_digits", L"Digits: Find \\d+"));
        AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.source_spaces", L"Repeated spaces: Find [ \\t]{2,}"));
        AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.source_capture", L"Capture: Find (\\w+), replace with \\1"));
        AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.limitations", L"Limitations"));
        AddHelpBlock(blocks, HelpLineKind::Note, LocalizedHelpText(L"fbe.regex_help.body.source.limitations", L"No UCP, Unicode property classes, lookbehind, \\K, \\G, branch reset, PCRE2 verbs, or FBE Design replacement formatting."));
        return blocks;
    }

    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.engine", L"Engine"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.engine", L"Design search uses PCRE2-16. UTF mode is always enabled."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.unicode_ucp", L"Unicode and UCP"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.unicode", L"Enable Unicode (UCP) for Unicode-aware character properties and word classes. Use \\p{L}, \\p{N} and \\P{...} for Unicode properties."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.characters", L"Characters, metacharacters and escapes"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.characters", L"Ordinary characters match themselves. Escape regex metacharacters with \\. Escape sequences include \\t, \\r, \\n and \\R."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.classes", L"Classes and ranges"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.classes", L"Use [abc], [^abc] and ranges such as [a-z]. The shorthand classes \\d, \\D, \\s, \\S, \\w and \\W are available."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.anchors", L"Anchors and boundaries"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.anchors", L"^ and $ are line anchors; \\A and \\z anchor the subject. \\b and \\B match word-boundary positions."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.quantifiers", L"Quantifiers"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.quantifiers", L"Use *, +, ?, {n} and {n,m}. They are greedy by default; append ? for lazy matching and + for possessive matching."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.groups", L"Groups, alternation and backreferences"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.groups", L"Capturing (...), non-capturing (?:...), named (?<name>...), atomic (?>...) groups and alternation | are supported. Use \\1 and \\k<name> for backreferences."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.lookaround", L"Lookaround and inline options"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.lookaround", L"Use lookahead (?=...) and (?!...), lookbehind (?<=...) and (?<!...), and inline options (?i), (?m), (?s) and (?x)."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.advanced", L"Advanced PCRE2"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.advanced", L"Conditional patterns, \\K, \\G, branch reset (?|...), numeric (?1) and named (?&name) subroutine calls are available. (*SKIP)(*FAIL) can exclude alternatives."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.syntax", L"Syntax reference"));
    AddQuickReferenceSyntax(blocks, context, FbeSearchPresets::RegexQuickReferenceMode::Search);
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.replacement", L"Replacement in FBE"));
    AddHelpBlock(blocks, HelpLineKind::Body, LocalizedHelpText(L"fbe.regex_help.body.design.replacement", L"Use $0 or \\0 for the whole match, $1..$9 or \\1..\\9 for captured groups, and $+ or \\+ for the last captured group. \\U, \\L and \\T change case; \\Q resets it; \\S applies Strong and \\E applies Emphasis."));
    AddQuickReferenceSyntax(blocks, context, FbeSearchPresets::RegexQuickReferenceMode::Replacement);
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.examples", L"Practical examples"));
    AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.design_spaces", L"Multiple spaces: Find [ \\t]{2,}, replace with one space."));
    AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.design_punctuation", L"Before punctuation: Find [ \\t]+([,;:!?]), replace with $1."));
    AddHelpBlock(blocks, HelpLineKind::Example, LocalizedHelpText(L"fbe.regex_help.example.design_word", L"Repeated word: Find \\b(\\p{L}+)\\s+\\1\\b with UCP enabled."));
    AddHelpBlock(blocks, HelpLineKind::Heading, LocalizedHelpText(L"fbe.regex_help.heading.limitations", L"Limitations"));
    AddHelpBlock(blocks, HelpLineKind::Note, LocalizedHelpText(L"fbe.regex_help.body.design.limitations", L"Only groups 1..9 are replaceable in FBE, and replacement across paragraphs is rejected."));
    return blocks;
}

CString JoinHelpBlocks(const std::vector<HelpBlock>& blocks, std::vector<int>& starts)
{
    CString text;
    starts.clear();
    for(size_t index = 0; index < blocks.size(); ++index)
    {
        if(!text.IsEmpty()) text += L"\r\n";
        starts.push_back(text.GetLength());
        text += blocks[index].text;
    }
    return text;
}

void ApplyHelpBlockStyles(HWND text, const std::vector<HelpBlock>& blocks)
{
    std::vector<int> starts; const CString joined = JoinHelpBlocks(blocks, starts);
    CHARFORMAT2 heading = {}; heading.cbSize = sizeof(heading); heading.dwMask = CFM_BOLD; heading.dwEffects = CFE_BOLD;
    CHARFORMAT2 title = heading; title.dwMask |= CFM_SIZE; title.yHeight = 220;
    CHARFORMAT2 syntax = {}; syntax.cbSize = sizeof(syntax); syntax.dwMask = CFM_FACE; ::lstrcpynW(syntax.szFaceName, L"Consolas", LF_FACESIZE);
    PARAFORMAT2 titleParagraph = {}; titleParagraph.cbSize = sizeof(titleParagraph); titleParagraph.dwMask = PFM_SPACEAFTER; titleParagraph.dySpaceAfter = 120;
    PARAFORMAT2 headingParagraph = {}; headingParagraph.cbSize = sizeof(headingParagraph); headingParagraph.dwMask = PFM_SPACEBEFORE | PFM_SPACEAFTER; headingParagraph.dySpaceBefore = 140; headingParagraph.dySpaceAfter = 40;
    PARAFORMAT2 bodyParagraph = {}; bodyParagraph.cbSize = sizeof(bodyParagraph); bodyParagraph.dwMask = PFM_SPACEAFTER; bodyParagraph.dySpaceAfter = 10;
    PARAFORMAT2 exampleParagraph = {}; exampleParagraph.cbSize = sizeof(exampleParagraph); exampleParagraph.dwMask = PFM_STARTINDENT | PFM_SPACEAFTER; exampleParagraph.dxStartIndent = 180; exampleParagraph.dySpaceAfter = 30;
    PARAFORMAT2 noteParagraph = {}; noteParagraph.cbSize = sizeof(noteParagraph); noteParagraph.dwMask = PFM_SPACEBEFORE; noteParagraph.dySpaceBefore = 80;
    for(size_t index = 0; index < blocks.size(); ++index)
    {
        ::SendMessage(text, EM_SETSEL, starts[index], starts[index] + blocks[index].text.GetLength());
        const HelpLineKind kind = blocks[index].kind;
        if(kind == HelpLineKind::Title)
        {
            ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&title));
            ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&titleParagraph));
        }
        else if(kind == HelpLineKind::Heading)
        {
            ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&heading));
            ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&headingParagraph));
        }
        else if(kind == HelpLineKind::Syntax || kind == HelpLineKind::Example)
        {
            ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&syntax));
            if(kind == HelpLineKind::Example) ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&exampleParagraph));
        }
        else if(kind == HelpLineKind::Body) ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&bodyParagraph));
        else if(kind == HelpLineKind::Note) ::SendMessage(text, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&noteParagraph));
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
        COMMAND_ID_HANDLER(IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
    {
        FbeApplyRuntimeDialogLocalization(m_hWnd, IDD_REGEX_HELP);
        SetWindowText(FbeLoadRuntimeStringByKey(
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"fbe.regex_help.design.caption" : L"fbe.regex_help.source.caption",
            m_context == FbeSearchPresets::SearchUiContext::Design ? L"Regular expression help — Design" : L"Regular expression help — Source"));

        m_blocks = BuildHelpBlocks(m_context);
        std::vector<int> starts;
        SetDlgItemText(IDC_REGEX_HELP_TEXT, JoinHelpBlocks(m_blocks, starts));
        const HWND text = GetDlgItem(IDC_REGEX_HELP_TEXT);
        if (text) ApplyTheme(text);
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
    LRESULT OnThemeChanged(UINT, WPARAM, LPARAM, BOOL&) { ThemeManager::ApplyToWindow(m_hWnd); ApplyTheme(GetDlgItem(IDC_REGEX_HELP_TEXT)); return 0; }
    LRESULT OnClose(WORD, WORD, HWND, BOOL&) { SaveSize(); EndDialog(IDC_REGEX_HELP_CLOSE); return 0; }

private:
    void ApplyTheme(HWND text)
    {
        if (!text) return;
        ::SendMessage(text, EM_SETBKGNDCOLOR, 0, ThemeManager::WindowColor());
        CHARFORMAT2 body = {}; body.cbSize = sizeof(body); body.dwMask = CFM_COLOR; body.crTextColor = ThemeManager::TextColor();
        ::SendMessage(text, EM_SETSEL, 0, -1);
        ::SendMessage(text, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&body));
        ApplyHelpBlockStyles(text, m_blocks);
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
    std::vector<HelpBlock> m_blocks;
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
