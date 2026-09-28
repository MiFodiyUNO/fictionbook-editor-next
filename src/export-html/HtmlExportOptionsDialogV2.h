#pragma once

#include "HtmlExportSettings.h"
#include "HtmlExportGeneralPage.h"
#include "HtmlExportAppearancePage.h"
#include "HtmlExportImagesPage.h"
#include "HtmlExportNotesMetadataPage.h"
#include "TemplateResolver.h"
#include "resource.h"
#include "..\\common\\ModernFileDialog.h"
#include <wtl/atlctrls.h>
#include <vector>

inline void BuildHtmlModernFileTypes(const CString& value, std::vector<CString>& labels, std::vector<CString>& patterns, std::vector<COMDLG_FILTERSPEC>& filters)
{
    labels.clear(); patterns.clear(); filters.clear(); int position=0;
    while(position>=0) { int first=value.Find(L'|',position); if(first<0) break; CString label=value.Mid(position,first-position); position=first+1; int second=value.Find(L'|',position); if(second<0) break; CString pattern=value.Mid(position,second-position); position=second+1; if(label.IsEmpty()||pattern.IsEmpty()) break; labels.push_back(label); patterns.push_back(pattern); }
    for(size_t i=0;i<labels.size();++i) filters.push_back({labels[i],patterns[i]});
}

class CHtmlExportOptionsDialogV2 : public CDialogImpl<CHtmlExportOptionsDialogV2>
{
public:
    enum { IDD = IDD_HTML_EXPORT_OPTIONS_V2 };
    HtmlExportSettings m_settings;
    HtmlExportSettings m_workingSettings;
    HtmlExportGeneralPage m_generalPage;
    HtmlExportAppearancePage m_appearancePage;
    HtmlExportImagesPage m_imagesPage;
    HtmlExportNotesMetadataPage m_notesPage;
    int m_currentPage = 0;

    void LoadSettings();
    void Persist();

    BEGIN_MSG_MAP(CHtmlExportOptionsDialogV2)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        NOTIFY_HANDLER(IDC_OPTIONS_TABS, TCN_SELCHANGE, OnTabChanged)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTabChanged(int, LPNMHDR, BOOL&);
    LRESULT OnOk(WORD, WORD, HWND, BOOL&);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
    void SelectPage(int page);
    void LayoutPages();
};

class CHtmlFileDialogEventsV2 : public CComObjectRootEx<CComSingleThreadModel>, public IFileDialogEvents, public IFileDialogControlEvents
{
public:
    static const DWORD SettingsButtonId = 2001;
    BEGIN_COM_MAP(CHtmlFileDialogEventsV2)
        COM_INTERFACE_ENTRY(IFileDialogEvents)
        COM_INTERFACE_ENTRY(IFileDialogControlEvents)
    END_COM_MAP()
    HWND owner = NULL; CHtmlExportOptionsDialogV2* options = NULL;
    STDMETHOD(OnFileOk)(IFileDialog*) { return S_OK; } STDMETHOD(OnFolderChanging)(IFileDialog*, IShellItem*) { return S_OK; }
    STDMETHOD(OnFolderChange)(IFileDialog*) { return S_OK; } STDMETHOD(OnSelectionChange)(IFileDialog*) { return S_OK; }
    STDMETHOD(OnShareViolation)(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE* r) { if (r) *r=FDESVR_DEFAULT; return S_OK; }
    STDMETHOD(OnTypeChange)(IFileDialog*) { return S_OK; } STDMETHOD(OnOverwrite)(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE* r) { if (r) *r=FDEOR_DEFAULT; return S_OK; }
    STDMETHOD(OnItemSelected)(IFileDialogCustomize*, DWORD, DWORD) { return S_OK; } STDMETHOD(OnCheckButtonToggled)(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; }
    STDMETHOD(OnControlActivating)(IFileDialogCustomize*, DWORD) { return S_OK; }
    STDMETHOD(OnButtonClicked)(IFileDialogCustomize* customize, DWORD id);
};
