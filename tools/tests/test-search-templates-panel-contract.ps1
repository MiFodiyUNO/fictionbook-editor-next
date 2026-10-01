[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
$resources = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\FBE.rc')
$catalog = Get-Content -Raw -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json
function Require([string]$pattern, [string]$description) { if ($source -notmatch $pattern) { throw "Missing $description." } }
Require 'OnTogglePresets[\s\S]*?SetPresetPanelVisible\(!collapse\)[\s\S]*?collapse && _Settings\.SearchTemplatesPanelPinned\(\)[\s\S]*?SetSearchTemplatesPanelPinned\(false, true\)' 'manual collapse pin reset'
$pinStart = $source.IndexOf('LRESULT OnTogglePresetPin')
$pinEnd = $source.IndexOf('LRESULT OnApplyPreset', $pinStart)
$pinHandler = $source.Substring($pinStart, $pinEnd - $pinStart)
if ($pinHandler -notmatch 'SetSearchTemplatesPanelPinned\(pinned, true\)' -or $pinHandler -notmatch 'InvalidateRect') { throw 'Missing pin persistence and visual update.' }
if ($pinHandler -match 'SetPresetPanelVisible') { throw 'Unexpected pin visibility mutation.' }
$visibleStart = $source.IndexOf('void SetPresetPanelVisible')
$visibleEnd = $source.IndexOf('FbeSearchPresets::SearchPreset CurrentPreset', $visibleStart)
$visibilityHandler = $source.Substring($visibleStart, $visibleEnd - $visibleStart)
if ($visibilityHandler -match 'SetSearchTemplatesPanelPinned') { throw 'Unexpected implicit pin reset in visibility setter.' }
Require 'struct PresetPanelMetrics' 'shared preset panel metrics'
Require 'GetPresetPanelMetrics\(int availableHeight = 0\)' 'adaptive metrics function'
Require 'metrics\.treeHeight = \(std::max\)\(metrics\.lineHeight \* 4' 'four-row minimum tree'
Require 'm_presetPanelHeight = GetPresetPanelMetrics\(availablePanelHeight\)\.totalHeight' 'monitor constrained expanded height'
Require 'UiMetrics::ScaleForDpi\(22, UiMetrics::DpiForWindow\(dialog\)\)' 'DPI-aware compact pin size'
foreach ($control in @('IDC_FIND_PRESET_APPLY','IDC_FIND_PRESET_SAVE','IDC_FIND_PRESET_UPDATE','IDC_FIND_PRESET_RENAME','IDC_FIND_PRESET_DELETE')) { Require ("SetWindowPos\(GetDlgItem\(" + $control + '\)') "layout for $control" }
foreach ($asset in @('src\fbe\res\icons\lucide\pin.svg','src\fbe\res\icons\lucide\pin-off.svg','src\fbe\res\icons\lucide\pin.ico','src\fbe\res\icons\lucide\pin-off.ico','src\fbe\res\icons\lucide\LICENSE.txt')) { if (-not (Test-Path (Join-Path $root $asset))) { throw "Missing asset $asset" } }
if ($resources -notmatch 'IDI_FIND_PRESETS_PIN\s+ICON' -or $resources -notmatch 'IDI_FIND_PRESETS_PIN_OFF\s+ICON') { throw 'Native pin icon resources are missing.' }
if ($source -match '::Ellipse\(draw->hDC|::Rectangle\(draw->hDC|::LineTo\(draw->hDC, center') { throw 'Unexpected manual pin drawing.' }
Require 'LoadImage\([\s\S]*?IDI_FIND_PRESETS_PIN[\s\S]*?IMAGE_ICON' 'pin HICON loader'
Require 'DrawIconEx\(' 'DrawIconEx pin rendering'
Require 'UiMetrics::ScaleForDpi\(16, dpi\)' 'DPI-aware pin glyph rectangle'
if ($source -match 'DrawState|DSS_MONO') { throw 'Pin glyph must not use monochrome DrawState rendering.' }
$pin = $catalog.strings.'fbe.search_preset.pin'.translations
foreach ($language in $catalog.targetLanguages) { if ([string]::IsNullOrWhiteSpace([string]$pin.$language)) { throw "Missing pin tooltip for $language." } }
if ($pin.'en-US' -ne 'Always open the templates panel' -or $pin.'ru-RU' -ne 'Всегда открывать панель шаблонов') { throw 'Pin tooltip semantics are not canonical.' }
Write-Host 'Templates pin, icon, and adaptive layout contract passed.'