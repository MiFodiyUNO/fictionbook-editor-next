<#
.SYNOPSIS
Guards the legacy Shift quick-customization path for FBE toolbars.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'src\fbe\mainfrm.cpp')
$header = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'src\fbe\mainfrm.h')

function MustContain([string] $text, [string] $value, [string] $description) {
    if ($text.IndexOf($value, [StringComparison]::Ordinal) -lt 0) {
        throw "${description}: '$value'"
    }
}

foreach ($required in @(
    'BeginToolbarQuickCustomize',
    'TrackToolbarQuickCustomize',
    'CompleteToolbarQuickCustomize',
    'ApplyToolbarQuickCustomizeItems',
    '(wParam & MK_SHIFT) != 0',
    'TB_HITTEST',
    'SM_CXDRAG',
    'SM_CYDRAG',
    'items.insert(items.begin() + source, separator)',
    'items.erase(items.begin() + source)',
    'if(sourceSeparator) return false',
    'if(!::PtInRect(&client, point)) return false',
    'UpdateCommandToolbarItems(items)',
    'UpdateScriptToolbarItems(runtime->definition.id, items)',
    'PortableToolbarStore::Save(layout)',
    'ToolbarLayoutAdapter::Apply(toolbar, previous, catalog)',
    'return ::DefSubclassProc(window, message, wParam, lParam);'
)) {
    MustContain $source $required 'Shift quick-customization implementation is incomplete'
}

foreach ($required in @(
    'HWND m_quickToolbarWindow;',
    'int m_quickToolbarSourceIndex;',
    'bool m_quickToolbarSourceSeparator;',
    'bool m_quickToolbarDragging;'
)) {
    MustContain $header $required 'Quick-customization state is incomplete'
}

if ($source -match 'DeleteButton\(.*outside|AddScriptToToolbar\(.*quick') {
    throw 'Shift quick-customization must not add scripts or delete buttons outside a toolbar.'
}

Write-Host 'Shift toolbar quick-customization contract passed.'