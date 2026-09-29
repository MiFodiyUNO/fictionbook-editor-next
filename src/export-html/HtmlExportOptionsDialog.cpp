#include "stdafx.h"
#include "HtmlExportOptionsDialog.h"
#include "RuntimeLocalization.h"
#include "TemplateResolver.h"
#include "resource.h"

void CHtmlExportOptionsDialog::LoadSettings()
{
    HtmlExportSettingsStore::Load(_Settings, m_settings);
    m_workingSettings=m_settings;
}
void CHtmlExportOptionsDialog::Persist()
{
    HtmlExportSettingsStore::Save(_Settings,m_settings);
}
LRESULT CHtmlExportOptionsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetWindowText(LoadExportHtmlString(IDS_HTML_EXPORT_OPTIONS_TITLE));
    CTabCtrl tabs = GetDlgItem(IDC_OPTIONS_TABS);
    const UINT titles[] = { IDS_OPTIONS_TAB_GENERAL, IDS_OPTIONS_TAB_APPEARANCE, IDS_OPTIONS_TAB_IMAGES, IDS_OPTIONS_TAB_NOTES };
    for (int index = 0; index < static_cast<int>(_countof(titles)); ++index) {
        TCITEM item = {};
        item.mask = TCIF_TEXT;
        CString title = LoadExportHtmlString(titles[index]);
        item.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(title));
        tabs.InsertItem(index, &item);
    }
    m_generalPage.Attach(&m_workingSettings);
    m_generalPage.SetSplitSupported(m_splitSupported);
    m_appearancePage.Attach(&m_workingSettings);
    m_imagesPage.Attach(&m_workingSettings);
    m_notesPage.Attach(&m_workingSettings);
    m_generalPage.Create(m_hWnd);
    m_appearancePage.Create(m_hWnd);
    m_imagesPage.Create(m_hWnd);
    m_notesPage.Create(m_hWnd);
    LayoutPages();
    SelectPage(0);
    return TRUE;
}
LRESULT CHtmlExportOptionsDialog::OnSize(UINT, WPARAM, LPARAM, BOOL&)
{
    LayoutPages();
    return 0;
}
LRESULT CHtmlExportOptionsDialog::OnDpiChanged(UINT, WPARAM, LPARAM lParam, BOOL&)
{
    const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
    if (suggested)
        SetWindowPos(NULL, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
    LayoutPages();
    return 0;
}
LRESULT CHtmlExportOptionsDialog::OnTabChanged(int, LPNMHDR, BOOL&)
{
    SelectPage(CTabCtrl(GetDlgItem(IDC_OPTIONS_TABS)).GetCurSel());
    return 0;
}
void CHtmlExportOptionsDialog::LayoutPages()
{
    CTabCtrl tabs = GetDlgItem(IDC_OPTIONS_TABS);
    if (!tabs.IsWindow()) return;
    CRect page;
    tabs.GetWindowRect(&page);
    ScreenToClient(&page);
    tabs.AdjustRect(FALSE, &page);
    HWND pages[] = { m_generalPage.m_hWnd, m_appearancePage.m_hWnd, m_imagesPage.m_hWnd, m_notesPage.m_hWnd };
    for (size_t index = 0; index < _countof(pages); ++index)
        if (pages[index]) ::SetWindowPos(pages[index], NULL, page.left, page.top, page.Width(), page.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}
void CHtmlExportOptionsDialog::SelectPage(int page)
{
    if (page < 0 || page > 3) return;
    m_currentPage = page;
    CTabCtrl(GetDlgItem(IDC_OPTIONS_TABS)).SetCurSel(page);
    HWND pages[] = { m_generalPage.m_hWnd, m_appearancePage.m_hWnd, m_imagesPage.m_hWnd, m_notesPage.m_hWnd };
    for (int index = 0; index < static_cast<int>(_countof(pages)); ++index)
        if (pages[index]) ::ShowWindow(pages[index], index == page ? SW_SHOW : SW_HIDE);
}
LRESULT CHtmlExportOptionsDialog::OnOk(WORD, WORD, HWND, BOOL&)
{
    HtmlExportSettings candidate = m_workingSettings;
    CString error;
    if (!m_generalPage.SaveToSettings(candidate, error)) { SelectPage(0); ShowExportHtmlTaskDialog(m_hWnd, IDS_ERROR, NULL, error, TDCBF_OK_BUTTON, TD_ERROR_ICON); return 0; }
    if (!m_appearancePage.SaveToSettings(candidate, error)) { SelectPage(1); ShowExportHtmlTaskDialog(m_hWnd, IDS_ERROR, NULL, error, TDCBF_OK_BUTTON, TD_ERROR_ICON); return 0; }
    if (!m_imagesPage.SaveToSettings(candidate, error)) { SelectPage(2); ShowExportHtmlTaskDialog(m_hWnd, IDS_ERROR, NULL, error, TDCBF_OK_BUTTON, TD_ERROR_ICON); return 0; }
    if (!m_notesPage.SaveToSettings(candidate, error)) { SelectPage(3); ShowExportHtmlTaskDialog(m_hWnd, IDS_ERROR, NULL, error, TDCBF_OK_BUTTON, TD_ERROR_ICON); return 0; }
    m_workingSettings = candidate;
    m_settings = m_workingSettings;
    EndDialog(IDOK);
    return 0;
}
LRESULT CHtmlExportOptionsDialog::OnCancel(WORD, WORD, HWND, BOOL&) { EndDialog(IDCANCEL); return 0; }
STDMETHODIMP CHtmlFileDialogEvents::OnButtonClicked(IFileDialogCustomize* customize, DWORD id)
{
    if (id==SettingsButtonId && options) { HWND h=owner; CComPtr<IOleWindow> w; if(customize && SUCCEEDED(customize->QueryInterface(IID_PPV_ARGS(&w)))) w->GetWindow(&h); options->DoModal(h); }
    return S_OK;
}
