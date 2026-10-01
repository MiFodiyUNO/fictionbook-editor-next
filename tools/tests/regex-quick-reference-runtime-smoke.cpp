#include "stdafx.h"
#include "search\\ui\\ComboBoxEdit.h"
#include "search\\ui\\RegexQuickReferencePopup.h"
#include "ThemeManager.h"

CAppModule _Module;

CString FbeLoadRuntimeStringByKey(LPCWSTR, LPCWSTR fallback)
{
    return CString(fallback ? fallback : L"");
}

namespace ThemeManager
{
void ApplyToWindow(HWND) {}
COLORREF TextColor() { return RGB(0, 0, 0); }
COLORREF ControlColor() { return RGB(255, 255, 255); }
COLORREF BorderColor() { return RGB(96, 96, 96); }
COLORREF SeparatorColor() { return RGB(192, 192, 192); }
COLORREF SecondaryTextColor() { return RGB(96, 96, 96); }
COLORREF SelectionBackgroundColor() { return RGB(0, 120, 215); }
COLORREF SelectionTextColor() { return RGB(255, 255, 255); }
HBRUSH WindowBrush() { static HBRUSH window = ::CreateSolidBrush(RGB(255, 255, 255)); return window; }
HBRUSH Brush(ThemeColorRole role)
{
    static HBRUSH control = ::CreateSolidBrush(ControlColor());
    static HBRUSH selection = ::CreateSolidBrush(SelectionBackgroundColor());
    return role == THEME_COLOR_SELECTION_BACKGROUND ? selection : control;
}
}

namespace
{
void DispatchMessages(CMessageLoop& messageLoop)
{
    MSG message = {};
    while (::PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (!messageLoop.PreTranslateMessage(&message))
        {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
        }
    }
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

LRESULT CALLBACK OutsideWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_LBUTTONDOWN)
        ++*reinterpret_cast<int*>(::GetWindowLongPtr(window, GWLP_USERDATA));
    return ::DefWindowProc(window, message, wParam, lParam);
}

bool ShowPopup(HWND owner, HWND anchor, int& inserts, int& fullHelp, HWND& popupWindow)
{
    RegexQuickReferencePopup* popup = new RegexQuickReferencePopup();
    if (!popup->Show(owner, anchor, FbeSearchPresets::SearchUiContext::Design, FbeSearchPresets::RegexQuickReferenceMode::Search,
        [&inserts](const FbeSearchPresets::RegexQuickReferenceEntry&) { ++inserts; }, [&fullHelp]() { ++fullHelp; })) return false;
    popupWindow = popup->m_hWnd;
    HWND caption = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_CAPTION);
    HWND left = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_LEFT);
    HWND right = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_RIGHT);
    HWND fullHelpButton = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_FULL_HELP);
    wchar_t captionText[64] = {};
    wchar_t fullHelpText[64] = {};
    if (!Check(::IsWindow(popupWindow)) || !Check(caption) || !Check(left) || !Check(right) || !Check(fullHelpButton) ||
        !Check(::GetWindowText(caption, captionText, _countof(captionText)) > 0) ||
        !Check(::GetWindowText(fullHelpButton, fullHelpText, _countof(fullHelpText)) > 0) ||
        !Check(CString(captionText) == L"Design — Find") || !Check(CString(fullHelpText) == L"Full help...") ||
        !Check(::SendMessage(left, LB_GETCOUNT, 0, 0) > 0)) return false;
    return true;
}

bool TestPopupMessageLoop(HWND owner, HWND anchor, CMessageLoop& messageLoop)
{
    int inserts = 0;
    int fullHelp = 0;
    HWND popupWindow = NULL;

    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) return false;
    ::PostMessage(::GetDlgItem(popupWindow, IDC_REGEX_QUICK_LEFT), WM_KEYDOWN, VK_ESCAPE, 0);
    DispatchMessages(messageLoop);
    if (::IsWindow(popupWindow) || inserts != 0 || fullHelp != 0) return false;

    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) return false;
    ::PostMessage(::GetDlgItem(popupWindow, IDC_REGEX_QUICK_LEFT), WM_KEYDOWN, VK_RETURN, 0);
    DispatchMessages(messageLoop);
    if (::IsWindow(popupWindow) || inserts != 1 || fullHelp != 0) return false;

    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) return false;
    const HWND left = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_LEFT);
    const HWND right = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_RIGHT);
    // The first row is a category heading; hover on the next row must select
    // a real entry without inserting it and clear the other column.
    ::PostMessage(left, WM_MOUSEMOVE, 0, MAKELPARAM(8, 22));
    DispatchMessages(messageLoop);
    if (::SendMessage(left, LB_GETCURSEL, 0, 0) <= 0 || ::SendMessage(right, LB_GETCURSEL, 0, 0) != LB_ERR || inserts != 1) return false;
    ::PostMessage(left, WM_KEYDOWN, VK_RIGHT, 0);
    DispatchMessages(messageLoop);
    if (::GetFocus() != right) return false;
    ::PostMessage(right, WM_KEYDOWN, VK_LEFT, 0);
    DispatchMessages(messageLoop);
    if (::GetFocus() != left) return false;
    ::DestroyWindow(popupWindow);
    DispatchMessages(messageLoop);

    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) return false;
    HWND fullHelpButton = ::GetDlgItem(popupWindow, IDC_REGEX_QUICK_FULL_HELP);
    ::PostMessage(popupWindow, WM_COMMAND, MAKEWPARAM(IDC_REGEX_QUICK_FULL_HELP, BN_CLICKED), reinterpret_cast<LPARAM>(fullHelpButton));
    DispatchMessages(messageLoop);
    if (::IsWindow(popupWindow) || inserts != 1 || fullHelp != 1) return false;

    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) return false;
    ::PostMessage(::GetDlgItem(popupWindow, IDC_REGEX_QUICK_LEFT), WM_KEYDOWN, VK_F1, 0);
    DispatchMessages(messageLoop);
    if (::IsWindow(popupWindow) || inserts != 1 || fullHelp != 2) return false;

    int outsideClicks = 0;
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = OutsideWindowProc;
    windowClass.hInstance = _Module.GetModuleInstance();
    windowClass.lpszClassName = L"FBERegexQuickReferenceOutside";
    if (!::RegisterClass(&windowClass) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    HWND outside = ::CreateWindowEx(0, windowClass.lpszClassName, L"outside", WS_CHILD | WS_VISIBLE,
        60, 60, 80, 40, owner, NULL, _Module.GetModuleInstance(), NULL);
    if (!outside) return false;
    ::SetWindowLongPtr(outside, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&outsideClicks));
    if (!ShowPopup(owner, anchor, inserts, fullHelp, popupWindow)) { ::DestroyWindow(outside); return false; }
    ::PostMessage(outside, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(4, 4));
    DispatchMessages(messageLoop);
    const bool outsideResult = !::IsWindow(popupWindow) && outsideClicks == 1;
    ::DestroyWindow(outside);
    return outsideResult;
}
}

int wmain()
{
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_WIN95_CLASSES };
    if (!::InitCommonControlsEx(&controls)) return 1;
    _Module.Init(NULL, ::GetModuleHandle(NULL));
    CMessageLoop messageLoop;
    if (!_Module.AddMessageLoop(&messageLoop)) { _Module.Term(); return 1; }
    HWND owner = ::CreateWindowEx(0, WC_STATIC, L"owner", WS_OVERLAPPEDWINDOW, 0, 0, 320, 200, NULL, NULL, _Module.GetModuleInstance(), NULL);
    HWND anchor = ::CreateWindowEx(0, WC_BUTTON, L"?", WS_CHILD | WS_VISIBLE, 10, 10, 20, 20, owner, NULL, _Module.GetModuleInstance(), NULL);
    if (owner) ::ShowWindow(owner, SW_SHOW);
    int result = 0;
    if (!owner || !anchor) result = 1;
    else if (!TestComboInsertion(owner)) result = 2;
    else if (!TestPopupMessageLoop(owner, anchor, messageLoop)) result = 3;
    if (owner) ::DestroyWindow(owner);
    _Module.RemoveMessageLoop();
    _Module.Term();
    return result;
}
