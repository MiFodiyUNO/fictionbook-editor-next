#pragma once

#include "..\\RegexQuickReference.h"
#include "..\\..\\resource.h"
#include "..\\..\\ThemeManager.h"
#include <functional>

class RegexQuickReferencePopup : public CWindowImpl<RegexQuickReferencePopup>, public CMessageFilter
{
public:
    DECLARE_WND_CLASS(L"FBERegexQuickReferencePopup")
    RegexQuickReferencePopup();
    ~RegexQuickReferencePopup();
    bool Show(HWND owner, HWND anchor, FbeSearchPresets::SearchUiContext context, FbeSearchPresets::RegexQuickReferenceMode mode,
        const std::function<void(const FbeSearchPresets::RegexQuickReferenceEntry&)>& insert, const std::function<void()>& fullHelp);
    BOOL PreTranslateMessage(MSG* message);
    // A successful Show transfers ownership to the HWND; OnFinalMessage deletes this.
    void OnFinalMessage(HWND) { delete this; }
    BEGIN_MSG_MAP(RegexQuickReferencePopup)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_NCDESTROY, OnNcDestroy)
        NOTIFY_CODE_HANDLER(TTN_GETDISPINFOW, OnToolTipGetDispInfo)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_FBE_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        COMMAND_HANDLER(IDC_REGEX_QUICK_LEFT, LBN_DBLCLK, OnActivate)
        COMMAND_HANDLER(IDC_REGEX_QUICK_RIGHT, LBN_DBLCLK, OnActivate)
        COMMAND_ID_HANDLER(IDC_REGEX_QUICK_FULL_HELP, OnFullHelp)
    END_MSG_MAP()
private:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnNcDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnToolTipGetDispInfo(int, LPNMHDR, BOOL&);
    LRESULT OnKeyDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnKillFocus(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnThemeChanged(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnActivate(WORD, WORD, HWND, BOOL&);
    LRESULT OnFullHelp(WORD, WORD, HWND, BOOL&);
    void Activate();
    void AddRows(CListBox& list, std::vector<int>& rows, const std::vector<int>& indexes);
    void DrawListItem(const DRAWITEMSTRUCT& draw, const std::vector<int>& rows);
    void MoveColumn(bool right);
    bool MoveSelection(HWND listWindow, int direction);
    int FirstEntryRow(const std::vector<int>& rows) const;
    CString Caption() const;
    CString CategoryCaption(FbeSearchPresets::RegexQuickReferenceCategory category) const;
    bool DescriptionIsTruncated(HWND list, int row, const std::vector<int>& rows, CString& text) const;
    void AddDescriptionToolTip(HWND list);
    CStatic m_caption;
    CListBox m_left;
    CListBox m_right;
    CButton m_fullHelp;
    std::vector<FbeSearchPresets::RegexQuickReferenceEntry> m_entries;
    std::vector<int> m_leftRows;
    std::vector<int> m_rightRows;
    std::function<void(const FbeSearchPresets::RegexQuickReferenceEntry&)> m_insert;
    std::function<void()> m_openFullHelp;
    FbeSearchPresets::SearchUiContext m_context;
    FbeSearchPresets::RegexQuickReferenceMode m_mode;
    CMessageLoop* m_messageLoop;
    HFONT m_monospaceFont;
    int m_syntaxColumnWidth;
    HWND m_toolTip = NULL;
    CString m_tooltipText;
};
