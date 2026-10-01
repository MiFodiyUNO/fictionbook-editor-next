[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexHelpDialog.cpp')
foreach ($token in @('enum class HelpLineKind { Title, Heading, Body, Syntax, Example, Note }', 'HelpLineKind ClassifyHelpLine', 'HelpLineKind::Syntax', 'HelpLineKind::Example', 'HelpLineKind::Note', 'CFM_FACE', 'Consolas', 'PFM_STARTINDENT', 'PFM_SPACEBEFORE')) {
    if ($source -notmatch [regex]::Escape($token)) { throw "Missing Regex Help formatting behavior: $token" }
}
if ($source.Contains('line.Left(')) { throw 'Regex Help formatting must not guess line kind from regex punctuation.' }
Write-Host 'Regex Help formatting contract passed.'
