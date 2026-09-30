#pragma once

#include "..\\RegexQuickReference.h"
#include <functional>

class RegexQuickReferencePopup : public CWindowImpl<RegexQuickReferencePopup>
{
public:
    DECLARE_WND_CLASS(L"FBERegexQuickReferencePopup")
    RegexQuickReferencePopup();
    bool Show(HWND owner, HWND anchor, FbeSearchPresets::SearchUiContext context, FbeSearchPresets::RegexQuickReferenceMode mode,
        const std::function<void(const FbeSearchPresets::RegexQuickReferenceEntry&)>& insert, const std::function<void()>& fullHelp);
    BOOL PreTranslateMessage(MSG* message);
    void OnFinalMessage(HWND) { delete this; }
    BEGIN_MSG_MAP(RegexQuickReferencePopup)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        COMMAND_HANDLER(1, LBN_DBLCLK, OnActivate)
        COMMAND_HANDLER(4, LBN_DBLCLK, OnActivate)
        COMMAND_ID_HANDLER(2, OnFullHelp)
    END_MSG_MAP()
private:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnKeyDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnKillFocus(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnActivate(WORD, WORD, HWND, BOOL&);
    LRESULT OnFullHelp(WORD, WORD, HWND, BOOL&);
    void Activate();
    void AddRows(CListBox& list, std::vector<int>& rows, const std::vector<int>& indexes);
    void MoveColumn(bool right);
    CString Caption() const;
    CString CategoryCaption(FbeSearchPresets::RegexQuickReferenceCategory category) const;
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
};
