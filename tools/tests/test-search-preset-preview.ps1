<# Guards the localized, Unicode-safe Find/Replace preview contract. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
$catalog = Get-Content -Raw -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json

foreach($token in @('MakePresetPreviewValue', 'L"\\t"', 'L"\\r"', 'L"\\n"', 'L"\x2026"', 'fbe.search_preset.preview.find', 'fbe.search_preset.preview.replace', 'fbe.search_preset.preview.empty')) {
    if($source -notmatch [regex]::Escape($token)) { throw "Missing preview behavior: $token" }
}
foreach($key in @('fbe.search_preset.preview.find', 'fbe.search_preset.preview.replace', 'fbe.search_preset.preview.empty')) {
    $entry = $catalog.strings.$key
    if($null -eq $entry -or [string]::IsNullOrWhiteSpace($entry.translations.'en-US') -or [string]::IsNullOrWhiteSpace($entry.translations.'ru-RU')) { throw "Missing en-US/ru-RU preview localization: $key" }
}
foreach($token in @('PresetPreviewText', 'PreviewHeightForCurrentSelection', 'DrawTextW', 'DT_CALCRECT | DT_WORDBREAK', 'lineHeight * 4', 'ResizePresetPanelForCurrentSelection')) { if($source -notmatch [regex]::Escape($token)) { throw "Missing dynamic preview behavior: $token" } }
if($source -notmatch 'if\s*\(preset->hasReplacement\)') { throw 'A Find dialog must retain the Replace part of a stored preset preview.' }
if($source -notmatch 'preset->builtIn && preset->safety == FbeSearchPresets::SearchPresetSafety::ReviewOnly') { throw 'Only built-in ReviewOnly presets may show the editorial review warning.' }
$apply = [regex]::Match($source, '(?s)void ApplySelectedPreset\(\).*?(?=\n\s*LRESULT OnApplyPreset)').Value
if($apply -notmatch 'if \(!preset\) return;' -or $apply -notmatch 'm_view->m_fo\.pattern = preset->findText;') { throw 'A selected ReviewOnly Find preset must still apply its search expression.' }
Write-Host 'Search preset preview contract passed.'
