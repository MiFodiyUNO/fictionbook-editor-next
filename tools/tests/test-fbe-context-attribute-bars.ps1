<# Guards the UI-only boundary of contextual attribute bars. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$barsHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\ui\ContextAttributeBars.h')
$barsSource = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\ui\ContextAttributeBars.cpp')
$controlsHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\ui\ContextAttributeControls.h')
$mainHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\mainfrm.h')
$mainSource = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\mainfrm.cpp')
$project = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\FBE.vcxproj')

function Require([string]$text, [string]$pattern, [string]$description) {
    if($text -notmatch $pattern) { throw "Context attribute bars contract missing: $description" }
}

foreach($forbidden in @('mainfrm\.h', 'FBDoc', 'FB::Doc', 'MSHTML', 'DocumentSession', 'SearchReplace', 'Settings')) {
    if($barsHeader -match $forbidden -or $barsSource -match $forbidden -or $controlsHeader -match $forbidden) {
        throw "Context attribute UI must not depend on $forbidden."
    }
}

Require $barsHeader 'class\s+ContextAttributeBars' 'ContextAttributeBars owner'
Require $barsHeader 'struct\s+LinkAttributeState' 'link UI state'
Require $barsHeader 'struct\s+TableAttributeState' 'table UI state'
Require $barsHeader 'SetLinkState\s*\(' 'link state setter'
Require $barsHeader 'GetLinkState\s*\(' 'link state getter'
Require $barsHeader 'SetTableState\s*\(' 'table state setter'
Require $barsHeader 'GetTableState\s*\(' 'table state getter'
Require $barsHeader 'LinkAttributeAvailability' 'link availability state'
Require $barsHeader 'TableAttributeAvailability' 'table availability state'
Require $barsHeader 'SetLinkAvailability\s*\(' 'link availability setter'
Require $barsHeader 'SetTableAvailability\s*\(' 'table availability setter'
Require $barsHeader 'ApplySelectionState\s*\(' 'selection state application'
Require $barsHeader 'UpdateMetrics\s*\(' 'central metrics update'
Require $barsHeader 'UpdateLocalization\s*\(' 'central localization update'
Require $barsSource 'TB_DELETEBUTTON' 'context-owned caption-toolbar rebuild'
if($barsSource -match '1234567890') { throw 'Context attribute layout must not size fields with digit placeholders.' }
foreach($token in @('ContextAttributeFieldWidth::Short', 'ContextAttributeFieldWidth::Medium', 'ContextAttributeFieldWidth::Long', 'ContextAttributeFieldWidth::Dropdown', 'FieldWidth(', 'CaptionWidth(', 'GetTextExtentPoint32W', 'UiMetrics::ScaleForDpi', 'AddFixedWidthSlot', 'AddAttributePairSlots')) {
    if($barsSource -notmatch [regex]::Escape($token)) { throw "Context attribute sizing must use $token." }
}
foreach($required in @('m_colspanCaption.*ContextAttributeFieldWidth::Short', 'm_rowspanCaption.*ContextAttributeFieldWidth::Short', 'm_hrefCaption.*ContextAttributeFieldWidth::Long', 'm_imageTitleCaption.*ContextAttributeFieldWidth::Long')) {
    if($barsSource -notmatch $required) { throw "Context attribute field class is missing: $required" }
}
if($barsSource -notmatch 'index \* 3 \+ 1') { throw 'Context attribute toolbar pairs must retain a separate fixed DPI gap slot.' }
foreach($token in @('CaptionHeight(', 'SetContextRowHeight(', 'TB_SETBUTTONSIZE', 'TB_GETBUTTONSIZE', 'HIWORD(buttonSize)', 'UiMetrics::ScaleForDpi(4', 'NormalizeRebarBands', 'RBBIM_CHILDSIZE', 'info.cyChild = height', 'info.cyMinChild = height', 'info.cyMaxChild = height', 'info.cyIntegral = height')) {
    if($barsSource -notmatch [regex]::Escape($token) -and $barsHeader -notmatch [regex]::Escape($token)) { throw "Context row height contract is missing $token." }
}
Require $mainSource 'UpdateMetrics\(\);[\s\S]{0,220}NormalizeRebarBands\(m_rebar\);[\s\S]{0,120}m_rebar\.SendMessage\(WM_SIZE\);[\s\S]{0,120}UpdateLayout\(\)' 'context metric updates trigger a full main-frame relayout'
if($barsHeader -match 'ContextBarMode|\bSetMode\s*\(|\bMode\s*\(' -or $barsSource -match 'ContextBarMode|\bSetMode\s*\(') { throw 'ContextAttributeBars must not retain test-only visibility modes.' }
Require $controlsHeader 'class\s+CCustomEdit' 'custom edit moved from main frame'
Require $controlsHeader 'class\s+CCustomStatic' 'custom static moved from main frame'
Require $controlsHeader 'class\s+CTableToolbarsWindow' 'toolbar window moved from main frame'
Require $controlsHeader 'WM_PAINT' 'caption painting behavior'
Require $controlsHeader 'WM_SETFONT' 'caption font behavior'
Require $controlsHeader 'IDN_ED_RETURN' 'edit parent notification'
Require $mainHeader 'ContextAttributeBars\s+m_contextAttributeBars' 'single main-frame context UI owner'
if($mainHeader -match 'class\s+CCustomEdit\b|class\s+CCustomStatic\b|class\s+CTableToolbarsWindow\b') { throw 'mainfrm.h must not define context custom controls.' }
if($mainSource -match '(?m)^#define\s+m_(id|href|section|hWndLinksBar|hWndTableBar)') { throw 'mainfrm.cpp must not retain context-control compatibility macros.' }
foreach($accessor in @('IdBox', 'IdEdit', 'IdCaption', 'HrefEdit', 'HrefCaption', 'TableIdBox', 'TableStyleBox', 'CellIdBox', 'CellStyleBox', 'ColspanBox', 'RowspanBox', 'RowAlignBox', 'AlignBox', 'VAlignBox')) {
    if($barsHeader -match "\b$accessor\s*\(") { throw "ContextAttributeBars must not expose raw $accessor access." }
    if($mainSource -match "m_contextAttributeBars\.$accessor\s*\(") { throw "CMainFrame must not access raw $accessor." }
}
foreach($item in @('ui\ContextAttributeBars.cpp', 'ui\ContextAttributeControls.cpp', 'ui\ContextAttributeBars.h', 'ui\ContextAttributeControls.h')) {
    if(-not $project.Contains($item)) { throw "FBE project must include $item." }
}

Write-Host 'Context attribute bars boundary contract passed.'
