$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportOptionsDialogV2.h')
$implementation = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportOptionsDialogV2.cpp')
$resource = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTML.rc')
$plugin = Get-Content -Raw (Join-Path $root 'src\export-html\ExportHTMLPlugin.cpp')
$pages = @('HtmlExportGeneralPage', 'HtmlExportAppearancePage', 'HtmlExportImagesPage', 'HtmlExportNotesMetadataPage')
$allSource = $source + $implementation + $resource + $plugin
foreach ($token in @('class CHtmlExportOptionsDialogV2 : public CDialogImpl', 'IDD_HTML_EXPORT_OPTIONS_V2', 'NOTIFY_HANDLER(IDC_OPTIONS_TABS, TCN_SELCHANGE', 'MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)', 'COMMAND_ID_HANDLER(IDOK, OnOk)', 'COMMAND_ID_HANDLER(IDCANCEL, OnCancel)', 'void Persist()')) { if (-not $source.Contains($token)) { throw "V2 HTML options dialog is missing: $token" } }
foreach ($page in $pages) {
    $header = Get-Content -Raw (Join-Path $root "src\export-html\$page.h")
    $pageSource = Get-Content -Raw (Join-Path $root "src\export-html\$page.cpp")
    $allSource += $header + $pageSource
    foreach ($token in @('LoadFromSettings()', 'SaveToSettings(HtmlExportSettings& candidate, CString& error)', 'UpdateEnabledState()')) { if (-not $header.Contains($token)) { throw "V2 settings page $page is missing: $token" } }
    if ($pageSource -match '\b_Settings\b|Persist\(|ExportHTMLPlugin|XSL') { throw "V2 settings page $page leaks exporter or persistence dependencies." }
}
foreach ($token in @('IDD_HTML_EXPORT_OPTIONS_V2 DIALOGEX', 'IDD_HTML_EXPORT_GENERAL_PAGE DIALOGEX', 'IDD_HTML_EXPORT_APPEARANCE_PAGE DIALOGEX', 'IDD_HTML_EXPORT_IMAGES_PAGE DIALOGEX', 'IDD_HTML_EXPORT_NOTES_PAGE DIALOGEX', 'DS_CONTROL | WS_CHILD')) { if ($resource -notmatch [regex]::Escape($token)) { throw "V2 HTML options resource is missing: $token" } }
foreach ($token in @('m_workingSettings=m_settings', 'HtmlExportSettings candidate = m_workingSettings', 'm_workingSettings = candidate', 'SelectPage(0)', 'SelectPage(1)', 'SelectPage(2)', 'SelectPage(3)', 'LayoutPages()', 'options.Persist()')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "V2 HTML transactional or lifecycle regression: $token" } }
foreach ($token in @('BuildHtmlModernFileTypes', 'IDS_SAVE_FILE_FILTER', 'IDS_OPEN_CSS_FILTER', 'Outcome::Failed', 'FbeDiagnostic::HResult')) { if ($allSource -notmatch [regex]::Escape($token)) { throw "HTML modern dialog regression: $token" } }
if ($allSource.Contains('OutputDebugStringW')) { throw 'Modern file dialog errors must use persistent logging.' }
foreach ($caption in @('HTML with external images', 'XSL files (*.xsl)', 'CSS files (*.css)', 'All files (*.*)')) { if ($allSource.Contains($caption)) { throw "HTML modern dialog contains a hardcoded filter caption: $caption" } }
if ($plugin -notmatch 'Outcome::Cancelled[\s\S]*Outcome::Failed[\s\S]*options\.Persist\(\)') { throw 'HTML settings must persist only after an accepted save dialog.' }
if ($implementation -match 'OnInitDialog[^{]*\{[^}]*HtmlExportSettingsStore::Load') { throw 'HTML options OnInitDialog must not reload persistent settings.' }
if ($plugin -notmatch 'options\.LoadSettings\(\);[\s\S]*ModernFileDialog::Show') { throw 'HTML options must load settings once before opening Save dialog.' }
if ($implementation -notmatch 'OnCancel[\s\S]*EndDialog\(IDCANCEL\)') { throw 'Nested HTML options Cancel must leave the current object unchanged.' }
Write-Host 'HTML export options dialog contract passed.'
