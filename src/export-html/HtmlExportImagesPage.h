#pragma once

#include "HtmlExportSettings.h"
#include "resource.h"
#include <wtl/atlctrls.h>

class HtmlExportImagesPage : public CDialogImpl<HtmlExportImagesPage>
{
public:
    enum { IDD = IDD_HTML_EXPORT_IMAGES_PAGE };
    void Attach(HtmlExportSettings* settings) { m_settings = settings; }
    void LoadFromSettings();
    bool SaveToSettings(HtmlExportSettings& candidate, CString& error);
    void UpdateEnabledState();

    BEGIN_MSG_MAP(HtmlExportImagesPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_IMAGES_FOLDER, OnFolderMode)
    END_MSG_MAP()

private:
    HtmlExportSettings* m_settings = NULL;
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnFolderMode(WORD, WORD, HWND, BOOL&);
};
