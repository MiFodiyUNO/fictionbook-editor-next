$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportOptionsDialog.h')
$implementation = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportOptionsDialog.cpp')
$resource = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTML.rc')
$plugin = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTMLPlugin.cpp')
$project = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTML.vcxproj')
$filters = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTML.vcxproj.filters')
$pages = @('HtmlExportGeneralPage', 'HtmlExportAppearancePage', 'HtmlExportImagesPage', 'HtmlExportNotesMetadataPage')
$allSource = $source + $implementation + $resource + $plugin
foreach ($token in @('class CHtmlExportOptionsDialog : public CDialogImpl', 'IDD_HTML_EXPORT_OPTIONS', 'NOTIFY_HANDLER(IDC_OPTIONS_TABS, TCN_SELCHANGE', 'MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)', 'COMMAND_ID_HANDLER(IDOK, OnOk)', 'COMMAND_ID_HANDLER(IDCANCEL, OnCancel)', 'void Persist()')) { if (-not $source.Contains($token)) { throw "HTML options dialog is missing: $token" } }
foreach ($page in $pages) {
    $header = Get-Content -Raw (Join-Path $root "src\export-html\$page.h")
    $pageSource = Get-Content -Raw (Join-Path $root "src\export-html\$page.cpp")
    $allSource += $header + $pageSource
    foreach ($token in @('LoadFromSettings()', 'SaveToSettings(HtmlExportSettings& candidate, CString& error)', 'UpdateEnabledState()')) { if (-not $header.Contains($token)) { throw "V2 settings page $page is missing: $token" } }
    if ($pageSource -match '\b_Settings\.\b|Persist\(|ExportCore|IXSLProcessor|addParameter') { throw "Settings page $page leaks exporter or persistence dependencies." }
}
foreach ($token in @('IDD_HTML_EXPORT_OPTIONS DIALOGEX', 'IDD_HTML_EXPORT_GENERAL_PAGE DIALOGEX', 'IDD_HTML_EXPORT_APPEARANCE_PAGE DIALOGEX', 'IDD_HTML_EXPORT_IMAGES_PAGE DIALOGEX', 'IDD_HTML_EXPORT_NOTES_PAGE DIALOGEX', 'DS_CONTROL | WS_CHILD')) { if ($resource -notmatch [regex]::Escape($token)) { throw "HTML options resource is missing: $token" } }
foreach ($token in @('m_workingSettings=m_settings', 'HtmlExportSettings candidate = m_workingSettings', 'm_workingSettings = candidate', 'SelectPage(0)', 'SelectPage(1)', 'SelectPage(2)', 'SelectPage(3)', 'LayoutPages()', 'options.Persist()')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "V2 HTML transactional or lifecycle regression: $token" } }
foreach ($token in @('BuildHtmlModernFileTypes', 'IDS_SAVE_FILE_FILTER', 'IDS_OPEN_CSS_FILTER', 'Outcome::Failed', 'FbeDiagnostic::HResult')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "HTML modern dialog regression: $token" } }
foreach ($token in @('IDS_OPEN_TEMPLATE_FILTER', 'templatePath', 'usingCustomTemplate', 'ExportHtmlPathsEqual', 'ModernFileDialog', 'FD204')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "Custom XSL regression: $token" } }
foreach ($token in @('IDS_OPTIONS_ERROR_TEMPLATE', 'GetFileAttributes', 'FILE_ATTRIBUTE_DIRECTORY', 'includeToc && (!translated || depth < 1 || depth > 10)')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "Template or disabled TOC validation regression: $token" } }
$appearanceSource = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportAppearancePage.cpp')
foreach ($token in @('Outcome::Cancelled', 'Outcome::Failed', 'FD205', 'Browse HTML CSS file')) { if ($appearanceSource -notmatch [regex]::Escape($token)) { throw "CSS browse regression: $token" } }
if ($allSource.Contains('OutputDebugStringW')) { throw 'Modern file dialog errors must use persistent logging.' }
foreach ($caption in @('HTML with external images', 'XSL files (*.xsl)', 'CSS files (*.css)', 'All files (*.*)')) { if ($allSource.Contains($caption)) { throw "HTML modern dialog contains a hardcoded filter caption: $caption" } }
if ($plugin -notmatch 'Outcome::Cancelled[\s\S]*Outcome::Failed[\s\S]*options\.Persist\(\)') { throw 'HTML settings must persist only after an accepted save dialog.' }
if ($implementation -match 'OnInitDialog[^{]*\{[^}]*HtmlExportSettingsStore::Load') { throw 'HTML options OnInitDialog must not reload persistent settings.' }
if ($plugin -notmatch 'options\.LoadSettings\(\);[\s\S]*ModernFileDialog::Show') { throw 'HTML options must load settings once before opening Save dialog.' }
if ($implementation -notmatch 'OnCancel[\s\S]*EndDialog\(IDCANCEL\)') { throw 'Nested HTML options Cancel must leave the current object unchanged.' }
if (([regex]::Matches($project, '<ClInclude Include="HtmlExportOptionsDialog\.h"').Count) -ne 1) { throw 'ExportHTML.vcxproj must contain one final options-dialog header entry.' }
if (([regex]::Matches($filters, '<ClInclude Include="HtmlExportOptionsDialog\.h"').Count) -ne 1) { throw 'ExportHTML.vcxproj.filters must contain one final options-dialog header entry.' }
Write-Host 'HTML export options dialog contract passed.'
