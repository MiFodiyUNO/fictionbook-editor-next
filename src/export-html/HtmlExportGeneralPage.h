#pragma once

#include "HtmlExportSettings.h"
#include "resource.h"
#include <wtl/atlctrls.h>

class HtmlExportGeneralPage : public CDialogImpl<HtmlExportGeneralPage>
{
public:
    enum { IDD = IDD_HTML_EXPORT_GENERAL_PAGE };
    void Attach(HtmlExportSettings* settings) { m_settings = settings; }
    void LoadFromSettings();
    bool SaveToSettings(HtmlExportSettings& candidate, CString& error);
    void UpdateEnabledState();

    BEGIN_MSG_MAP(HtmlExportGeneralPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_INCLUDE_TOC, OnIncludeToc)
    END_MSG_MAP()

private:
    HtmlExportSettings* m_settings = NULL;
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIncludeToc(WORD, WORD, HWND, BOOL&);
};
