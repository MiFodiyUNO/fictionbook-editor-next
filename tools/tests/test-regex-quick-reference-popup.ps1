<# Guards the native quick-reference popup integration and its modal-help escape hatch. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$dialog = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
$popup = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexQuickReferencePopup.cpp')
$popupHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexQuickReferencePopup.h')
foreach($token in @('RegexQuickReferencePopup', 'GetDlgItem(IDC_FIND_REGEX_HELP)', 'RegexQuickReferenceMode::Replacement', 'm_lastRegexTarget', 'm_view->SyncSearchOptionsToOpenDialogs(this)', 'InvalidateSearchSelectionState()', 'ShowRegexHelpDialog')) {
    if($dialog -notmatch [regex]::Escape($token)) { throw "Missing quick-reference dialog behavior: $token" }
}
foreach($token in @('WS_POPUP | WS_BORDER', 'WS_EX_TOOLWINDOW', 'MonitorFromWindow', 'UiMetrics::DpiForWindow', 'UiMetrics::ScaleForDpi', 'VK_ESCAPE', 'VK_RETURN', 'VK_F1', 'VK_LEFT', 'VK_RIGHT', 'm_left', 'm_right', 'AddRows', 'fbe.regex_quick.full_help', 'OnKillFocus', 'AddMessageFilter(this)', 'RemoveMessageFilter(this)', 'WM_LBUTTONDOWN', 'WM_RBUTTONDOWN', 'WM_MBUTTONDOWN', 'WM_NCLBUTTONDOWN', 'UpdateWindow()', 'OnNcDestroy')) {
    if($popup -notmatch [regex]::Escape($token)) { throw "Missing quick-reference popup behavior: $token" }
}
if($popupHeader -notmatch 'public CMessageFilter') { throw 'Popup must be registered as a message filter.' }
if($popup -match 'ShowWindow\(SW_SHOW\)\s*!=\s*FALSE') { throw 'ShowWindow return value must not control popup ownership.' }
if($popup -notmatch 'HWND hwnd = Create\(' -or $popup -notmatch 'if\(hwnd == NULL\) return false;' -or $popup -notmatch 'ShowWindow\(SW_SHOW\);\s*UpdateWindow\(\);\s*return true;') { throw 'Show must transfer ownership only after a valid HWND was created.' }
if($popup -notmatch 'const std::function<void\(\)> callback = m_openFullHelp;\s*DestroyWindow\(\);\s*if\(callback\) callback\(\);') { throw 'Full Help callback must be copied before self-destruction.' }
foreach($token in @('ScaleForDpi(560, dpi)', 'ScaleForDpi(360, dpi)', 'workMargin', 'info.rcWork.right - info.rcWork.left', 'info.rcWork.bottom - info.rcWork.top')) {
    if($popup -notmatch [regex]::Escape($token)) { throw "Popup must use the available monitor work area for its expanded geometry: $token" }
}
Write-Host 'Regex quick-reference popup contract passed.'
