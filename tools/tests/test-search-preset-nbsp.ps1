<# Keeps built-in NBSP template input independent from the user's display glyph. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$catalog = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\SearchPresetCatalog.cpp')
$normalizer = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\FBEview.cpp')
$settingsPage = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\settings\ui\SettingsEditorPage.cpp')

if($catalog -notmatch 'design\.nbsp-to-space[\s\S]{0,260}L"\\u00A0"') { throw 'The NBSP preset must retain literal U+00A0, not a configured display glyph.' }
if($normalizer -notmatch 'NormalizeSearchPatternNbsp[\s\S]{0,220}pattern\.Replace\(L"\\u00A0", _Settings\.GetNBSPChar\(\)\)') { throw 'Search must normalize literal U+00A0 to the configured NBSP character.' }
if($settingsPage -notmatch 'L"\\u25AB"') { throw 'The custom NBSP glyph U+25AB must remain selectable.' }
Write-Host 'Search preset NBSP normalization contract passed.'
