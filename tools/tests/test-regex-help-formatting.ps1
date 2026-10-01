[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexHelpDialog.cpp')
foreach ($token in @('enum class HelpLineKind { Title, Heading, Body, Syntax, Example, Note }', 'struct HelpBlock', 'BuildHelpBlocks', 'AddQuickReferenceSyntax', 'HelpLineKind::Syntax', 'HelpLineKind::Example', 'HelpLineKind::Note', 'CFM_FACE', 'Consolas', 'PFM_STARTINDENT', 'PFM_SPACEBEFORE')) {
    if ($source -notmatch [regex]::Escape($token)) { throw "Missing Regex Help formatting behavior: $token" }
}
if ($source -match 'ClassifyHelpLine|section ==') { throw 'Regex Help formatting must use explicit HelpBlock kinds, not section indexes.' }
Write-Host 'Regex Help formatting contract passed.'
