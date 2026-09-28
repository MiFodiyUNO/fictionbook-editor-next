#include "stdafx.h"
#include "HtmlExportGeneralPage.h"
#include "RuntimeLocalization.h"

LRESULT HtmlExportGeneralPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetDlgItemText(IDC_FORMAT_VALUE, LoadExportHtmlString(IDS_OPTIONS_FORMAT));
    SetDlgItemText(IDC_ENCODING_VALUE, LoadExportHtmlString(IDS_OPTIONS_ENCODING));
    SetDlgItemText(IDC_INCLUDE_TOC, LoadExportHtmlString(IDS_OPTIONS_INCLUDE_TOC));
    SetDlgItemText(IDC_TOC_DEPTH_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_TOC_DEPTH));
    LoadFromSettings();
    return TRUE;
}

void HtmlExportGeneralPage::LoadFromSettings()
{
    if (!m_settings) return;
    CheckDlgButton(IDC_INCLUDE_TOC, m_settings->includeToc ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_TOCDEPTH, m_settings->tocDepth, FALSE);
    UpdateEnabledState();
}

bool HtmlExportGeneralPage::SaveToSettings(HtmlExportSettings& candidate, CString& error)
{
    BOOL translated = FALSE;
    const int depth = static_cast<int>(GetDlgItemInt(IDC_TOCDEPTH, &translated, FALSE));
    if (!translated || depth < 1 || depth > 10) {
        ::SetFocus(GetDlgItem(IDC_TOCDEPTH));
        error = LoadExportHtmlString(IDS_OPTIONS_ERROR_TOC_DEPTH);
        return false;
    }
    candidate.includeToc = IsDlgButtonChecked(IDC_INCLUDE_TOC) == BST_CHECKED;
    candidate.tocDepth = depth;
    return true;
}

void HtmlExportGeneralPage::UpdateEnabledState()
{
    const BOOL enabled = IsDlgButtonChecked(IDC_INCLUDE_TOC) == BST_CHECKED;
    ::EnableWindow(GetDlgItem(IDC_TOCDEPTH), enabled);
    ::EnableWindow(GetDlgItem(IDC_TOC_DEPTH_LABEL), enabled);
}

LRESULT HtmlExportGeneralPage::OnIncludeToc(WORD, WORD, HWND, BOOL&)
{
    UpdateEnabledState();
    return 0;
}
