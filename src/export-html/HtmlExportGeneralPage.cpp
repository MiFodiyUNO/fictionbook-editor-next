#include "stdafx.h"
#include "HtmlExportGeneralPage.h"
#include "RuntimeLocalization.h"
#include "TemplateResolver.h"
#include "utils.h"
#include "..\\common\\ModernFileDialog.h"

#include <vector>

namespace {
void BuildTemplateFileTypes(const CString& value, std::vector<CString>& labels, std::vector<CString>& patterns, std::vector<COMDLG_FILTERSPEC>& filters)
{
    int position = 0;
    while (position >= 0) {
        const int first = value.Find(L'|', position);
        if (first < 0) break;
        CString label = value.Mid(position, first - position);
        position = first + 1;
        const int second = value.Find(L'|', position);
        if (second < 0) break;
        CString pattern = value.Mid(position, second - position);
        position = second + 1;
        if (label.IsEmpty() || pattern.IsEmpty()) break;
        labels.push_back(label);
        patterns.push_back(pattern);
    }
    for (size_t index = 0; index < labels.size(); ++index) filters.push_back({ labels[index], patterns[index] });
}
}

LRESULT HtmlExportGeneralPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetDlgItemText(IDC_FORMAT_VALUE, LoadExportHtmlString(IDS_OPTIONS_FORMAT));
    SetDlgItemText(IDC_ENCODING_VALUE, LoadExportHtmlString(IDS_OPTIONS_ENCODING));
    SetDlgItemText(IDC_INCLUDE_TOC, LoadExportHtmlString(IDS_OPTIONS_INCLUDE_TOC));
    SetDlgItemText(IDC_TOC_DEPTH_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_TOC_DEPTH));
    SetDlgItemText(IDC_TEMPLATE_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_TEMPLATE_LABEL));
    LoadFromSettings();
    InitTooltips();
    return TRUE;
}

void HtmlExportGeneralPage::LoadFromSettings()
{
    if (!m_settings) return;
    CheckDlgButton(IDC_INCLUDE_TOC, m_settings->includeToc ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_TOCDEPTH, m_settings->tocDepth, FALSE);
    SetDlgItemText(IDC_TEMPLATE, m_settings->templatePath);
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
    candidate.templatePath = U::GetWindowText(GetDlgItem(IDC_TEMPLATE));
    if (candidate.templatePath.IsEmpty() || !::PathFileExists(candidate.templatePath)) {
        ::SetFocus(GetDlgItem(IDC_TEMPLATE));
        error = LoadExportHtmlString(IDS_ERROR_OPEN_FILE);
        return false;
    }
    candidate.usingCustomTemplate = !ExportHtmlPathsEqual(candidate.templatePath, U::GetProgDirFile(L"html.xsl"));
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

LRESULT HtmlExportGeneralPage::OnBrowseTemplate(WORD, WORD, HWND, BOOL&)
{
    std::vector<CString> labels, patterns;
    std::vector<COMDLG_FILTERSPEC> filters;
    BuildTemplateFileTypes(LoadExportHtmlString(IDS_OPEN_TEMPLATE_FILTER), labels, patterns, filters);
    ModernFileDialog::Request request;
    request.fileMustExist = true;
    request.pathMustExist = true;
    request.defaultExtension = L"xsl";
    request.filters = filters.data();
    request.filterCount = static_cast<UINT>(filters.size());
    const ModernFileDialog::Result result = ModernFileDialog::Show(m_hWnd, request);
    if (result.outcome == ModernFileDialog::Outcome::Cancelled) return 0;
    if (result.outcome == ModernFileDialog::Outcome::Failed) {
        FbeDiagnostic::HResult(L"file-dialog", L"FD204", result.error, L"Browse HTML XSL template");
        return 0;
    }
    if (!result.paths.empty()) SetDlgItemText(IDC_TEMPLATE, result.paths.front().c_str());
    return 0;
}

void HtmlExportGeneralPage::InitTooltips()
{
    m_tooltip = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, NULL, _Module.GetModuleInstance(), NULL);
    if (!m_tooltip) return;
    AddTooltip(IDC_TEMPLATE, IDS_TOOLTIP_TEMPLATE);
    AddTooltip(IDC_BROWSE, IDS_TOOLTIP_BROWSE_TEMPLATE);
}

void HtmlExportGeneralPage::AddTooltip(UINT id, UINT textId)
{
    HWND control = GetDlgItem(id);
    if (!control) return;
    m_tooltipTexts.push_back(LoadExportHtmlString(textId));
    CString& text = m_tooltipTexts.back();
    if (text.IsEmpty()) return;
    TOOLINFO info = {};
    info.cbSize = sizeof(info);
    info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    info.hwnd = m_hWnd;
    info.uId = reinterpret_cast<UINT_PTR>(control);
    info.lpszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(text));
    ::SendMessage(m_tooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&info));
}

LRESULT HtmlExportGeneralPage::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    if (m_tooltip) {
        ::DestroyWindow(m_tooltip);
        m_tooltip = NULL;
    }
    return 0;
}
