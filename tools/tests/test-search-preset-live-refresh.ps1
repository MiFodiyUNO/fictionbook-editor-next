<# Ensures every preset mutation refreshes the current panel, then notifies all dialog contexts. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
foreach($handler in @('OnSavePreset', 'OnUpdatePreset', 'OnRenamePreset', 'OnDeletePreset')) {
    $match = [regex]::Match($source, "LRESULT\s+$handler[\s\S]*?(?=\n\s*LRESULT|\n\s*BEGIN_MSG_MAP)")
    if(-not $match.Success -or $match.Value -notmatch 'RefreshPresetPanel\(' -or $match.Value -notmatch 'NotifyOpenPresetPanels\(') { throw "$handler must immediately refresh itself and notify every open preset panel." }
}
foreach($token in @('OpenPresetPanels', 'OnDestroyPresetPanel', 'WM_DESTROY', 'TreeView_SelectItem', 'TreeView_EnsureVisible', 'nextId, nextId.IsEmpty')) {
    if($source -notmatch [regex]::Escape($token)) { throw "Missing selection-preserving refresh behavior: $token" }
}
Write-Host 'Search preset live-refresh contract passed.'
