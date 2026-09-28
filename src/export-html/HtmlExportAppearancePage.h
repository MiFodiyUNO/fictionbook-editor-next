#pragma once

#include "HtmlExportSettings.h"
#include "resource.h"
#include <wtl/atlctrls.h>

class HtmlExportAppearancePage : public CDialogImpl<HtmlExportAppearancePage>
{
public:
    enum { IDD = IDD_HTML_EXPORT_APPEARANCE_PAGE };
    void Attach(HtmlExportSettings* settings) { m_settings = settings; }
    void LoadFromSettings();
    bool SaveToSettings(HtmlExportSettings& candidate, CString& error);
    void UpdateEnabledState();

    BEGIN_MSG_MAP(HtmlExportAppearancePage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_CLEAR_CSS, OnClearCss)
        COMMAND_ID_HANDLER(IDC_BROWSE_CSS, OnBrowseCss)
        COMMAND_ID_HANDLER(IDC_FONT_FAMILY, OnFontFamily)
    END_MSG_MAP()

private:
    HtmlExportSettings* m_settings = NULL;
    void FillCombo(UINT id, const UINT* strings, size_t count, int selection);
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnClearCss(WORD, WORD, HWND, BOOL&);
    LRESULT OnBrowseCss(WORD, WORD, HWND, BOOL&);
    LRESULT OnFontFamily(WORD, WORD, HWND, BOOL&);
};
