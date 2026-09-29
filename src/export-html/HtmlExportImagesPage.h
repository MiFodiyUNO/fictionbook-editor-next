#pragma once

#include "HtmlExportSettings.h"
#include "resource.h"
#include <wtl/atlctrls.h>
#include <vector>

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
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(IDC_IMAGES_FOLDER, OnFolderMode)
    END_MSG_MAP()

private:
    HtmlExportSettings* m_settings = NULL;
    HWND m_tooltip = NULL;
    std::vector<CString> m_tooltipTexts;
    void InitTooltips();
    void AddTooltip(UINT id, UINT textId);
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnFolderMode(WORD, WORD, HWND, BOOL&);
};
