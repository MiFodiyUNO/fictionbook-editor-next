#include "stdafx.h"
#include "search\\ui\\ComboBoxEdit.h"
#include "search\\ui\\RegexQuickReferencePopup.h"

CAppModule _Module;

CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback)
{
    return CString(fallback ? fallback : L"");
}

namespace ThemeManager
{
void ApplyToWindow(HWND)
{
}
}

namespace
{
bool Pump()
{
    MSG message = {};
    while (::PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&message);
        ::DispatchMessage(&message);
    }
    return true;
}

bool Check(bool value)
{
    return value;
}

bool TestComboInsertion(HWND owner)
{
    HWND combo = ::CreateWindowEx(0, WC_COMBOBOX, L"abcdef", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
        0, 0, 160, 120, owner, NULL, _Module.GetModuleInstance(), NULL);
    COMBOBOXINFO comboInfo = { sizeof(comboInfo) };
    if (!::GetComboBoxInfo(combo, &comboInfo) || !comboInfo.hwndItem) return false;
    FbeComboBoxEdit::SetText(combo, L"abcdef");
    ::SetFocus(comboInfo.hwndItem);
    if (!combo || !FbeComboBoxEdit::SetSelection(combo, 3, 3)) return false;
    int start = 0;
    int end = 0;
    if (!FbeComboBoxEdit::GetSelection(combo, start, end) || start != 3 || end != 3) return false;
    FbeSearchPresets::RegexQuickReferenceEntry entry = {};
    entry.insertionText = L"\\d";
    entry.caretOffset = 2;
    entry.selectionStart = 2;
    const FbeSearchPresets::RegexQuickReferenceInsertion insertion = FbeSearchPresets::InsertRegexQuickReference(FbeComboBoxEdit::GetText(combo), start, end, entry);
    FbeComboBoxEdit::SetText(combo, insertion.text);
    if (!FbeComboBoxEdit::SetSelection(combo, insertion.selectionStart, insertion.selectionStart + insertion.selectionLength)) return false;
    if (FbeComboBoxEdit::GetText(combo) != L"abc\\ddef") return false;
    if (!FbeComboBoxEdit::GetSelection(combo, start, end) || start != 5 || end != 5) return false;

    FbeComboBoxEdit::SetText(combo, L"abcdef");
    if (!FbeComboBoxEdit::SetSelection(combo, 1, 4)) return false;
    if (!FbeComboBoxEdit::GetSelection(combo, start, end) || start != 1 || end != 4) return false;
    entry.insertionText = L"(...)";
    entry.caretOffset = 1;
    entry.selectionStart = 1;
    entry.selectionLength = 0;
    const FbeSearchPresets::RegexQuickReferenceInsertion replacement = FbeSearchPresets::InsertRegexQuickReference(FbeComboBoxEdit::GetText(combo), start, end, entry);
    FbeComboBoxEdit::SetText(combo, replacement.text);
    FbeComboBoxEdit::SetSelection(combo, replacement.selectionStart, replacement.selectionStart + replacement.selectionLength);
    const bool valid = FbeComboBoxEdit::GetText(combo) == L"a(...)ef" && FbeComboBoxEdit::GetSelection(combo, start, end) && start == 2 && end == 2;
    ::DestroyWindow(combo);
    return valid;
}

bool TestPopup(HWND owner, HWND anchor)
{
    int inserts = 0;
    RegexQuickReferencePopup* popup = new RegexQuickReferencePopup();
    if (!popup->Show(owner, anchor, FbeSearchPresets::SearchUiContext::Design, FbeSearchPresets::RegexQuickReferenceMode::Search,
        [&inserts](const FbeSearchPresets::RegexQuickReferenceEntry&) { ++inserts; }, [] {})) return false;
    const HWND popupWindow = popup->m_hWnd;
    if (!Check(::IsWindow(popupWindow)) || !Check(::GetDlgItem(popupWindow, 3)) || !Check(::GetDlgItem(popupWindow, 1)) ||
        !Check(::GetDlgItem(popupWindow, 4)) || !Check(::GetDlgItem(popupWindow, 2)) ||
        !Check(::SendMessage(::GetDlgItem(popupWindow, 1), LB_GETCOUNT, 0, 0) > 0)) return false;
    Pump();
    if (!Check(::IsWindow(popupWindow))) return false;
    ::DestroyWindow(popupWindow);
    Pump();
    return inserts == 0;
}

bool TestFullHelp(HWND owner, HWND anchor)
{
    int callbackCount = 0;
    RegexQuickReferencePopup* popup = new RegexQuickReferencePopup();
    if (!popup->Show(owner, anchor, FbeSearchPresets::SearchUiContext::Design, FbeSearchPresets::RegexQuickReferenceMode::Search,
        [](const FbeSearchPresets::RegexQuickReferenceEntry&) {}, [&callbackCount]() { ++callbackCount; })) return false;
    const HWND popupWindow = popup->m_hWnd;
    ::SendMessage(popupWindow, WM_COMMAND, MAKEWPARAM(2, BN_CLICKED), reinterpret_cast<LPARAM>(::GetDlgItem(popupWindow, 2)));
    Pump();
    return callbackCount == 1 && !::IsWindow(popupWindow);
}
}

int wmain()
{
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_WIN95_CLASSES };
    if (!::InitCommonControlsEx(&controls)) return 1;
    _Module.Init(NULL, ::GetModuleHandle(NULL));
    HWND owner = ::CreateWindowEx(0, WC_STATIC, L"owner", WS_OVERLAPPEDWINDOW, 0, 0, 320, 200, NULL, NULL, _Module.GetModuleInstance(), NULL);
    HWND anchor = ::CreateWindowEx(0, WC_BUTTON, L"?", WS_CHILD | WS_VISIBLE, 10, 10, 20, 20, owner, NULL, _Module.GetModuleInstance(), NULL);
    if (owner) ::ShowWindow(owner, SW_SHOW);
    int result = 0;
    if (!owner || !anchor) result = 1;
    else if (!TestComboInsertion(owner)) result = 2;
    else if (!TestPopup(owner, anchor)) result = 3;
    else if (!TestFullHelp(owner, anchor)) result = 4;
    if (owner) ::DestroyWindow(owner);
    _Module.Term();
    return result;
}
