<# Guards the native quick-reference popup integration and its modal-help escape hatch. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$dialog = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
$popup = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexQuickReferencePopup.cpp')
foreach($token in @('RegexQuickReferencePopup', 'GetDlgItem(IDC_FIND_REGEX_HELP)', 'RegexQuickReferenceMode::Replacement', 'm_lastRegexTarget', 'm_view->SyncSearchOptionsToOpenDialogs(this)', 'InvalidateSearchSelectionState()', 'ShowRegexHelpDialog')) {
    if($dialog -notmatch [regex]::Escape($token)) { throw "Missing quick-reference dialog behavior: $token" }
}
foreach($token in @('WS_POPUP | WS_BORDER', 'WS_EX_TOOLWINDOW', 'MonitorFromWindow', 'UiMetrics::DpiForWindow', 'UiMetrics::ScaleForDpi', 'VK_ESCAPE', 'VK_RETURN', 'VK_F1', 'VK_LEFT', 'VK_RIGHT', 'm_left', 'm_right', 'AddRows', 'fbe.regex_quick.full_help', 'OnKillFocus')) {
    if($popup -notmatch [regex]::Escape($token)) { throw "Missing quick-reference popup behavior: $token" }
}
Write-Host 'Regex quick-reference popup contract passed.'
