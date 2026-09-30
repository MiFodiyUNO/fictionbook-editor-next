#pragma once

#include <windows.h>
#include <atlstr.h>
#include <vector>

namespace FbeComboBoxEdit
{
inline bool GetSelection(HWND combo, int& start, int& end)
{
    if (!combo)
        return false;
    const LRESULT selection = ::SendMessage(combo, CB_GETEDITSEL, 0, 0);
    if (selection == CB_ERR)
        return false;
    start = LOWORD(selection);
    end = HIWORD(selection);
    return true;
}

inline bool SetSelection(HWND combo, int start, int end)
{
    if (!combo)
        return false;
    return ::SendMessage(combo, CB_SETEDITSEL, 0, MAKELPARAM(start, end)) != CB_ERR;
}

inline CString GetText(HWND combo)
{
    CString text;
    if (!combo)
        return text;
    const int length = ::GetWindowTextLength(combo);
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    ::GetWindowText(combo, &buffer[0], length + 1);
    return CString(&buffer[0]);
}

inline void SetText(HWND combo, const CString& text)
{
    if (combo)
        ::SetWindowText(combo, text);
}
}
