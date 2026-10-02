$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$grid = Join-Path $root 'src\fbe\table\TableGrid.cpp'
$editor = Join-Path $root 'src\fbe\table\TableStructuralEditor.cpp'
$view = Join-Path $root 'src\fbe\FBEview.cpp'

foreach ($path in @($grid, $editor)) {
    $text = Get-Content -Raw $path
    foreach ($forbidden in @('FBEview.h', 'CFBEView', 'mainfrm.h', 'CMainFrame', 'Settings')) {
        if ($text -match [regex]::Escape($forbidden)) { throw "$path must not depend on $forbidden" }
    }
}

$viewText = Get-Content -Raw $view
foreach ($legacy in @('struct LogicalTableCell', 'struct LogicalTableGrid', 'BuildLogicalTableGrid', 'CreateTableRowLike')) {
    if ($viewText -match [regex]::Escape($legacy)) { throw "FBEview.cpp still owns $legacy" }
}

foreach ($required in @('FbeTable::BuildGrid(table, grid)', 'FbeTable::InsertRow(Document(), grid, rowIndex, true, cell->tagName)')) {
    if ($viewText -notmatch [regex]::Escape($required)) { throw "Tab from the final table cell does not use the logical grid: $required" }
}

foreach ($required in @('grid.rows.size() <= 1', 'grid.columns <= 1')) {
    if ((Get-Content -Raw $editor) -notmatch [regex]::Escape($required)) { throw "Structural deletion guard is missing: $required" }
}
Write-Host 'PASS: table structure boundary'
