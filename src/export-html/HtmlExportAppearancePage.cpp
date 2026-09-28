#include "stdafx.h"
#include "HtmlExportAppearancePage.h"
#include "RuntimeLocalization.h"
#include "utils.h"
#include "..\\common\\ModernFileDialog.h"

#include <vector>

namespace {
void BuildCssFileTypes(const CString& source, std::vector<CString>& labels, std::vector<CString>& patterns, std::vector<COMDLG_FILTERSPEC>& filters)
{
    int position = 0;
    while (position >= 0) {
        const int separator = source.Find(L'|', position);
        if (separator < 0) break;
        CString label = source.Mid(position, separator - position);
        position = separator + 1;
        const int next = source.Find(L'|', position);
        if (next < 0) break;
        CString pattern = source.Mid(position, next - position);
        position = next + 1;
        if (label.IsEmpty() || pattern.IsEmpty()) break;
        labels.push_back(label);
        patterns.push_back(pattern);
    }
    for (size_t index = 0; index < labels.size(); ++index)
        filters.push_back({ labels[index], patterns[index] });
}
}

void HtmlExportAppearancePage::FillCombo(UINT id, const UINT* strings, size_t count, int selection)
{
    CComboBox combo = GetDlgItem(id);
    combo.ResetContent();
    for (size_t index = 0; index < count; ++index)
        combo.AddString(LoadExportHtmlString(strings[index]));
    combo.SetCurSel(selection >= 0 && selection < static_cast<int>(count) ? selection : 0);
}

LRESULT HtmlExportAppearancePage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetDlgItemText(IDC_STYLE_LABEL, LoadExportHtmlString(IDS_OPTIONS_STYLE));
    SetDlgItemText(IDC_FONT_LABEL, LoadExportHtmlString(IDS_OPTIONS_FONT));
    SetDlgItemText(IDC_FONT_SIZE_LABEL, LoadExportHtmlString(IDS_OPTIONS_FONT_SIZE));
    SetDlgItemText(IDC_LINE_HEIGHT_LABEL, LoadExportHtmlString(IDS_OPTIONS_LINE_HEIGHT));
    SetDlgItemText(IDC_CONTENT_WIDTH_LABEL, LoadExportHtmlString(IDS_OPTIONS_CONTENT_WIDTH));
    SetDlgItemText(IDC_MARGINS_LABEL, LoadExportHtmlString(IDS_OPTIONS_MARGINS));
    SetDlgItemText(IDC_TEXT_ALIGNMENT_LABEL, LoadExportHtmlString(IDS_OPTIONS_TEXT_ALIGNMENT));
    SetDlgItemText(IDC_HEADING_ALIGNMENT_LABEL, LoadExportHtmlString(IDS_OPTIONS_HEADING_ALIGNMENT));
    SetDlgItemText(IDC_CUSTOM_CSS_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_CUSTOM_CSS));
    SetDlgItemText(IDC_CLEAR_CSS, LoadExportHtmlString(IDS_OPTIONS_CSS_CLEAR));
    LoadFromSettings();
    return TRUE;
}

void HtmlExportAppearancePage::LoadFromSettings()
{
    if (!m_settings) return;
    const UINT styles[] = { IDS_OPTIONS_VALUE_CLASSIC, IDS_OPTIONS_VALUE_BOOK, IDS_OPTIONS_VALUE_MINIMAL };
    const UINT fonts[] = { IDS_OPTIONS_VALUE_SERIF, IDS_OPTIONS_VALUE_SANS, IDS_OPTIONS_VALUE_SYSTEM, IDS_OPTIONS_VALUE_CUSTOM };
    const UINT defaults[] = { IDS_OPTIONS_VALUE_DEFAULT, IDS_OPTIONS_VALUE_NORMAL, IDS_OPTIONS_VALUE_READING };
    const UINT margins[] = { IDS_OPTIONS_VALUE_DEFAULT, IDS_OPTIONS_VALUE_NORMAL, IDS_OPTIONS_VALUE_READING };
    const UINT text[] = { IDS_OPTIONS_VALUE_LEFT, IDS_OPTIONS_VALUE_JUSTIFIED };
    const UINT headings[] = { IDS_OPTIONS_VALUE_CENTER, IDS_OPTIONS_VALUE_LEFT };
    FillCombo(IDC_STYLE, styles, _countof(styles), m_settings->style);
    FillCombo(IDC_FONT_FAMILY, fonts, _countof(fonts), m_settings->fontFamily);
    FillCombo(IDC_LINE_HEIGHT, defaults, _countof(defaults), m_settings->lineHeight == 0 ? 0 : (m_settings->lineHeight <= 120 ? 1 : 2));
    FillCombo(IDC_PAGE_MARGINS, margins, _countof(margins), m_settings->pageMargins);
    FillCombo(IDC_TEXT_ALIGNMENT, text, _countof(text), m_settings->textAlignment);
    FillCombo(IDC_HEADING_ALIGNMENT, headings, _countof(headings), m_settings->headingAlignment);
    SetDlgItemText(IDC_CUSTOM_FONT, m_settings->customFontFamily);
    SetDlgItemInt(IDC_FONT_SIZE, m_settings->fontSize, FALSE);
    SetDlgItemInt(IDC_CONTENT_WIDTH, m_settings->contentMaxWidth, FALSE);
    SetDlgItemText(IDC_CUSTOM_CSS, m_settings->customCss);
    UpdateEnabledState();
}

bool HtmlExportAppearancePage::SaveToSettings(HtmlExportSettings& candidate, CString& error)
{
    BOOL translated = FALSE;
    const int fontSize = static_cast<int>(GetDlgItemInt(IDC_FONT_SIZE, &translated, FALSE));
    if (!translated || fontSize < 0 || fontSize > 72) { ::SetFocus(GetDlgItem(IDC_FONT_SIZE)); error = LoadExportHtmlString(IDS_OPTIONS_ERROR_IMAGE_SIZE); return false; }
    const int width = static_cast<int>(GetDlgItemInt(IDC_CONTENT_WIDTH, &translated, FALSE));
    if (!translated || width < 0 || width > 10000) { ::SetFocus(GetDlgItem(IDC_CONTENT_WIDTH)); error = LoadExportHtmlString(IDS_OPTIONS_ERROR_IMAGE_SIZE); return false; }
    candidate.style = max(0, min(2, CComboBox(GetDlgItem(IDC_STYLE)).GetCurSel()));
    candidate.fontFamily = max(0, min(3, CComboBox(GetDlgItem(IDC_FONT_FAMILY)).GetCurSel()));
    const int lineHeightValues[] = { 0, 120, 150 };
    const int lineHeightSelection = CComboBox(GetDlgItem(IDC_LINE_HEIGHT)).GetCurSel();
    if (lineHeightSelection < 0 || lineHeightSelection >= static_cast<int>(_countof(lineHeightValues))) { ::SetFocus(GetDlgItem(IDC_LINE_HEIGHT)); error = LoadExportHtmlString(IDS_ERROR); return false; }
    candidate.lineHeight = lineHeightValues[lineHeightSelection];
    candidate.pageMargins = CComboBox(GetDlgItem(IDC_PAGE_MARGINS)).GetCurSel();
    candidate.textAlignment = CComboBox(GetDlgItem(IDC_TEXT_ALIGNMENT)).GetCurSel();
    candidate.headingAlignment = CComboBox(GetDlgItem(IDC_HEADING_ALIGNMENT)).GetCurSel();
    candidate.customFontFamily = U::GetWindowText(GetDlgItem(IDC_CUSTOM_FONT));
    candidate.fontSize = fontSize;
    candidate.contentMaxWidth = width;
    candidate.customCss = U::GetWindowText(GetDlgItem(IDC_CUSTOM_CSS));
    return true;
}

void HtmlExportAppearancePage::UpdateEnabledState()
{
    ::EnableWindow(GetDlgItem(IDC_CUSTOM_FONT), CComboBox(GetDlgItem(IDC_FONT_FAMILY)).GetCurSel() == 3);
}

LRESULT HtmlExportAppearancePage::OnClearCss(WORD, WORD, HWND, BOOL&)
{
    SetDlgItemText(IDC_CUSTOM_CSS, L"");
    return 0;
}

LRESULT HtmlExportAppearancePage::OnBrowseCss(WORD, WORD, HWND, BOOL&)
{
    std::vector<CString> labels, patterns;
    std::vector<COMDLG_FILTERSPEC> filters;
    BuildCssFileTypes(LoadExportHtmlString(IDS_OPEN_CSS_FILTER), labels, patterns, filters);
    ModernFileDialog::Request request;
    request.fileMustExist = true;
    request.pathMustExist = true;
    request.defaultExtension = L"css";
    request.filters = filters.data();
    request.filterCount = static_cast<UINT>(filters.size());
    const ModernFileDialog::Result result = ModernFileDialog::Show(m_hWnd, request);
    if (result.outcome == ModernFileDialog::Outcome::Accepted && !result.paths.empty())
        SetDlgItemText(IDC_CUSTOM_CSS, result.paths.front().c_str());
    return 0;
}

LRESULT HtmlExportAppearancePage::OnFontFamily(WORD, WORD, HWND, BOOL&)
{
    UpdateEnabledState();
    return 0;
}
