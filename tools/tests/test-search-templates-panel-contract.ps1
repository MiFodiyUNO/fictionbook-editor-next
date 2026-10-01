[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
$resources = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\FBE.rc')
$catalog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json
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
Require 'metrics\.treeHeight = \(std::max\)\(metrics\.lineHeight \* 6' 'dense six-row default tree'
Require 'PreviewHeightForCurrentSelection' 'preview height follows selected preset content'
Require 'DT_CALCRECT \| DT_WORDBREAK' 'preview uses wrapped text measurement'
Require 'lineHeight \* 4' 'preview is bounded to four lines'
Require 'LocalizedButtonWidth' 'Apply button measures the localized caption'
Require 'GetTextExtentPoint32W' 'Apply caption measurement uses the button font'
Require 'applyMinimum' 'Apply width has a DPI-aware minimum'
Require 'applyMaximum' 'Apply width has a bounded maximum'
Require 'row2Width = \(std::max\)\(0, \(contentWidth - margin \* 2\) / 3\)' 'equal second-row actions'
Require 'm_presetPanelHeight = GetPresetPanelMetrics\(availablePanelHeight\)\.totalHeight' 'monitor constrained expanded height'
Require 'UiMetrics::ScaleForDpi\(18, UiMetrics::DpiForWindow\(dialog\)\)' 'DPI-aware compact pin size'
foreach ($control in @('IDC_FIND_PRESET_APPLY','IDC_FIND_PRESET_SAVE','IDC_FIND_PRESET_UPDATE','IDC_FIND_PRESET_RENAME','IDC_FIND_PRESET_DELETE')) { Require ("SetWindowPos\(GetDlgItem\(" + $control + '\)') "layout for $control" }
foreach ($asset in @('src\fbe\res\icons\lucide\pin.svg','src\fbe\res\icons\lucide\pin-mask-16.bmp','src\fbe\res\icons\lucide\pin-mask-20.bmp','src\fbe\res\icons\lucide\pin-mask-24.bmp','src\fbe\res\icons\lucide\pin-mask-32.bmp','src\fbe\res\icons\lucide\LICENSE.txt')) { if (-not (Test-Path (Join-Path $root $asset))) { throw "Missing asset $asset" } }
foreach($resource in @('IDB_FIND_PRESETS_PIN_16 BITMAP','IDB_FIND_PRESETS_PIN_20 BITMAP','IDB_FIND_PRESETS_PIN_24 BITMAP','IDB_FIND_PRESETS_PIN_32 BITMAP')) { if($resources -notlike "*$resource*") { throw "Missing pin mask resource $resource." } }
if ($source -match 'DrawIconEx\(|GetIconInfo\(|IDI_FIND_PRESETS_PIN|IMAGE_ICON|maskPixels\\[index\\]') { throw 'Pin renderer must not recover alpha from an ICO.' }
Require 'PresetPinMaskResource' 'pin bitmap-mask resource selection'
Require 'IMAGE_BITMAP' 'pin bitmap-mask loader'
Require 'GetDIBits' 'pin reads the authored bitmap mask'
Require 'maskInfo.bmiHeader.biBitCount = 1' 'pin reads monochrome mask coverage'
Require 'maskBits\[' 'pin tints authored mask coverage'
Require 'ThemeManager::AccentColor\(\)' 'pinned accent glyph'
Require 'ThemeManager::SecondaryTextColor\(\)' 'unpinned secondary glyph'
if ($source -match 'IDI_FIND_PRESETS_PIN_OFF') { throw 'Pin-off icon remains a production dependency.' }
Require 'UiMetrics::ScaleForDpi\(16, dpi\)' 'DPI-aware pin glyph rectangle'
if ($source -match 'DrawState|DSS_MONO') { throw 'Pin glyph must not use monochrome DrawState rendering.' }
$pin = $catalog.strings.'fbe.search_preset.pin'.translations
foreach ($language in $catalog.targetLanguages) { if ([string]::IsNullOrWhiteSpace([string]$pin.$language)) { throw "Missing pin tooltip for $language." } }
if ($pin.'en-US' -ne 'Always open the templates panel' -or $pin.'ru-RU' -ne 'Всегда открывать панель шаблонов') { throw 'Pin tooltip semantics are not canonical.' }
Write-Host 'Templates pin, icon, and adaptive layout contract passed.'