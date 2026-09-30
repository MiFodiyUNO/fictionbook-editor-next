#include "stdafx.h"
#include "RegexQuickReferencePopup.h"
#include "..\\..\\RuntimeLocalization.h"
#include "..\\..\\UiMetrics.h"

RegexQuickReferencePopup::RegexQuickReferencePopup() {}
LRESULT RegexQuickReferencePopup::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
    RECT client = {}; GetClientRect(&client);
    const UINT dpi = UiMetrics::DpiForWindow(m_hWnd); const int gap = UiMetrics::ScaleForDpi(6, dpi); const int captionHeight = UiMetrics::ScaleForDpi(18, dpi); const int buttonHeight = UiMetrics::ScaleForDpi(22, dpi);
    m_caption.Create(m_hWnd, CRect(gap, gap, client.right - gap, gap + captionHeight), Caption(), WS_CHILD | WS_VISIBLE, 0, 3);
    const int middle = client.right / 2;
    m_left.Create(m_hWnd, CRect(gap, gap + captionHeight, middle - gap / 2, client.bottom - buttonHeight - gap * 2), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 1);
    m_right.Create(m_hWnd, CRect(middle + gap / 2, gap + captionHeight, client.right - gap, client.bottom - buttonHeight - gap * 2), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 4);
    m_fullHelp.Create(m_hWnd, CRect(gap, client.bottom - buttonHeight - gap, client.right - gap, client.bottom - gap), FbeLoadRuntimeStringByKey(L"fbe.regex_quick.full_help", L"Full help..."), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 2);
    const HFONT font = UiMetrics::DialogFont(); m_caption.SetFont(font); m_left.SetFont(font); m_right.SetFont(font); m_fullHelp.SetFont(font);
    std::vector<int> leftIndexes, rightIndexes;
    for(size_t index = 0; index < m_entries.size(); ++index) {
        const bool characters = m_entries[index].category == FbeSearchPresets::RegexQuickReferenceCategory::Characters;
        if(m_mode == FbeSearchPresets::RegexQuickReferenceMode::Search ? characters : index < (m_entries.size() + 1) / 2) leftIndexes.push_back(static_cast<int>(index)); else rightIndexes.push_back(static_cast<int>(index));
    }
    AddRows(m_left, m_leftRows, leftIndexes); AddRows(m_right, m_rightRows, rightIndexes);
    if(!m_leftRows.empty()) { m_left.SetCurSel(m_leftRows.size() > 1 ? 1 : 0); m_left.SetFocus(); }
    else if(!m_rightRows.empty()) { m_right.SetCurSel(m_rightRows.size() > 1 ? 1 : 0); m_right.SetFocus(); }
    return 0;
}
void RegexQuickReferencePopup::AddRows(CListBox& list, std::vector<int>& rows, const std::vector<int>& indexes) {
    rows.clear(); FbeSearchPresets::RegexQuickReferenceCategory category = static_cast<FbeSearchPresets::RegexQuickReferenceCategory>(-1);
    for(size_t i = 0; i < indexes.size(); ++i) { const int index = indexes[i]; if(m_entries[index].category != category) { category = m_entries[index].category; list.AddString(L"— " + CategoryCaption(category) + L" —"); rows.push_back(-1); } CString row = m_entries[index].displaySyntax + L"    " + FbeLoadRuntimeStringByKey(m_entries[index].descriptionKey, m_entries[index].descriptionFallback); list.AddString(row); rows.push_back(index); }
}
bool RegexQuickReferencePopup::Show(HWND owner, HWND anchor, FbeSearchPresets::SearchUiContext context, FbeSearchPresets::RegexQuickReferenceMode mode, const std::function<void(const FbeSearchPresets::RegexQuickReferenceEntry&)>& insert, const std::function<void()>& fullHelp) {
    m_insert = insert; m_openFullHelp = fullHelp; m_context = context; m_mode = mode; FbeSearchPresets::GetRegexQuickReferenceEntries(context, mode, m_entries); if(m_entries.empty()) return false;
    const UINT dpi = UiMetrics::DpiForWindow(anchor); RECT rc = {}; ::GetWindowRect(anchor, &rc); const int width = UiMetrics::ScaleForDpi(390, dpi), height = UiMetrics::ScaleForDpi(270, dpi); HMONITOR monitor = MonitorFromWindow(anchor, MONITOR_DEFAULTTONEAREST); MONITORINFO info = { sizeof(info) }; GetMonitorInfo(monitor, &info);
    int x = rc.right + width <= info.rcWork.right ? rc.right : rc.left - width; int y = rc.bottom + height <= info.rcWork.bottom ? rc.bottom : rc.top - height;
    x = max(info.rcWork.left, min(x, info.rcWork.right - width)); y = max(info.rcWork.top, min(y, info.rcWork.bottom - height));
    return Create(owner, CRect(x, y, x + width, y + height), NULL, WS_POPUP | WS_BORDER, WS_EX_TOOLWINDOW) != NULL && ShowWindow(SW_SHOW) != FALSE;
}
CString RegexQuickReferencePopup::Caption() const {
    const bool source = m_context == FbeSearchPresets::SearchUiContext::Source;
    const bool replacement = m_mode == FbeSearchPresets::RegexQuickReferenceMode::Replacement;
    return FbeLoadRuntimeStringByKey(source ? (replacement ? L"fbe.regex_quick.source.replace" : L"fbe.regex_quick.source.search") : (replacement ? L"fbe.regex_quick.design.replace" : L"fbe.regex_quick.design.search"),
        source ? (replacement ? L"Source — Replace" : L"Source — Find") : (replacement ? L"Design — Replace" : L"Design — Find"));
}
CString RegexQuickReferencePopup::CategoryCaption(FbeSearchPresets::RegexQuickReferenceCategory category) const {
    switch(category) {
    case FbeSearchPresets::RegexQuickReferenceCategory::Characters: return FbeLoadRuntimeStringByKey(L"fbe.regex_quick.category.characters", L"Characters and anchors");
    case FbeSearchPresets::RegexQuickReferenceCategory::Quantifiers: return FbeLoadRuntimeStringByKey(L"fbe.regex_quick.category.quantifiers", L"Quantifiers");
    case FbeSearchPresets::RegexQuickReferenceCategory::Groups: return FbeLoadRuntimeStringByKey(L"fbe.regex_quick.category.groups", L"Groups and assertions");
    case FbeSearchPresets::RegexQuickReferenceCategory::Options: return FbeLoadRuntimeStringByKey(L"fbe.regex_quick.category.options", L"Inline options");
    default: return FbeLoadRuntimeStringByKey(L"fbe.regex_quick.category.replacement", L"Replacement");
    }
}
BOOL RegexQuickReferencePopup::PreTranslateMessage(MSG* message) {
    if(message->message != WM_KEYDOWN) return FALSE;
    if(message->wParam == VK_ESCAPE) { DestroyWindow(); return TRUE; }
    if(message->wParam == VK_RETURN) { Activate(); return TRUE; }
    if(message->wParam == VK_F1) { BOOL ignored = FALSE; OnFullHelp(0, 0, NULL, ignored); return TRUE; }
    if(message->wParam == VK_LEFT) { MoveColumn(false); return TRUE; }
    if(message->wParam == VK_RIGHT) { MoveColumn(true); return TRUE; }
    return FALSE;
}
void RegexQuickReferencePopup::MoveColumn(bool right) { CListBox& destination = right ? m_right : m_left; std::vector<int>& rows = right ? m_rightRows : m_leftRows; if(rows.empty()) return; destination.SetCurSel(rows.size() > 1 ? 1 : 0); destination.SetFocus(); }
void RegexQuickReferencePopup::Activate() { CListBox& list = ::GetFocus() == m_right.m_hWnd ? m_right : m_left; std::vector<int>& rows = ::GetFocus() == m_right.m_hWnd ? m_rightRows : m_leftRows; const int row = list.GetCurSel(); const int index = row >= 0 && static_cast<size_t>(row) < rows.size() ? rows[row] : -1; if(index >= 0 && static_cast<size_t>(index) < m_entries.size() && m_insert) { m_insert(m_entries[index]); DestroyWindow(); } }
LRESULT RegexQuickReferencePopup::OnActivate(WORD, WORD, HWND, BOOL&) { Activate(); return 0; }
LRESULT RegexQuickReferencePopup::OnFullHelp(WORD, WORD, HWND, BOOL&) { DestroyWindow(); if(m_openFullHelp) m_openFullHelp(); return 0; }
LRESULT RegexQuickReferencePopup::OnKeyDown(UINT, WPARAM key, LPARAM, BOOL&) { if(key == VK_ESCAPE) DestroyWindow(); else if(key == VK_RETURN) Activate(); else if(key == VK_F1) { BOOL ignored = FALSE; OnFullHelp(0, 0, NULL, ignored); } return 0; }
LRESULT RegexQuickReferencePopup::OnKillFocus(UINT, WPARAM, LPARAM, BOOL&) { HWND focus = ::GetFocus(); if(focus != m_hWnd && !::IsChild(m_hWnd, focus)) PostMessage(WM_CLOSE); return 0; }
