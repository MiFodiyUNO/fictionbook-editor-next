#pragma once

#include "HtmlExportSettings.h"
#include "resource.h"
#include <wtl/atlctrls.h>

class HtmlExportNotesMetadataPage : public CDialogImpl<HtmlExportNotesMetadataPage>
{
public:
    enum { IDD = IDD_HTML_EXPORT_NOTES_PAGE };
    void Attach(HtmlExportSettings* settings) { m_settings = settings; }
    void LoadFromSettings();
    bool SaveToSettings(HtmlExportSettings& candidate, CString& error);
    void UpdateEnabledState();

    BEGIN_MSG_MAP(HtmlExportNotesMetadataPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_METADATA, OnMetadata)
    END_MSG_MAP()

private:
    HtmlExportSettings* m_settings = NULL;
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMetadata(WORD, WORD, HWND, BOOL&);
};
