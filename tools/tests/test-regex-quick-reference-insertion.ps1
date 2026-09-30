<# Guards metadata-driven quick-reference insertion and its expected templates. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\RegexQuickReference.h')
foreach($token in @('L"[]", 1, 1, 0', 'L"()", 1, 1, 0', 'L"{1,3}", 2, 1, 3', 'result.text = text.Left(start) + entry.insertionText + text.Mid(end)', 'result.caret = start + entry.caretOffset')) {
    if($source -notmatch [regex]::Escape($token)) { throw "Missing insertion template or metadata: $token" }
}
if($source -notmatch '\(std::max\)\(0, \(std::min\)\(selectionStart, text.GetLength\(\)\)\)') { throw 'Insertion must clamp selections before replacing Unicode text.' }
Write-Host 'Regex quick-reference insertion contract passed.'
