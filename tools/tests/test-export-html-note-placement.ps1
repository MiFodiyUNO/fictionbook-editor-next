<# Exercises the built-in ExportHTML note placement modes. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$sourceXsl = Join-Path $root 'src\export-html\html.xsl'
$runtimeXsl = Join-Path $root 'runtime\html.xsl'
if ((Get-FileHash -LiteralPath $sourceXsl -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $runtimeXsl -Algorithm SHA256).Hash) { throw 'Source and runtime HTML XSL files diverged.' }
$xsl = New-Object -ComObject Msxml2.DOMDocument.6.0; $xsl.async = $false
if (-not $xsl.load($runtimeXsl)) { throw $xsl.parseError.reason }
$xml = New-Object -ComObject Msxml2.DOMDocument.6.0; $xml.async = $false
$fixture = '<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink"><description><title-info><book-title>Notes</book-title></title-info></description><body><section id="chapter"><title><p>Chapter</p></title><p>Text <a l:href="#note-one" type="note">1</a> and <a l:href="#note-one" type="note">again</a>.</p><section id="nested"><title><p>Nested</p></title><p>Nested text <a l:href="#note-two" type="note">2</a>.</p><section id="note-two"><title><p>Note two</p></title><p>Second note text.</p></section></section><section id="note-one"><title><p>Note one</p></title><p>First note text.</p></section></section></body></FictionBook>'
if (-not $xml.loadXML($fixture)) { throw $xml.parseError.reason }
$template = New-Object -ComObject Msxml2.XSLTemplate.6.0; $template.stylesheet = $xsl
function Transform([int] $placement) { $processor = $template.createProcessor(); $processor.input = $xml; $processor.addParameter('noteplacement', $placement, ''); [void]$processor.transform(); return [string]$processor.output }
function Assert-Equal([int] $actual, [int] $expected, [string] $name) { if ($actual -ne $expected) { throw "${name}: expected $expected, actual $actual" } }
function Assert-UniqueAnchor([string] $html, [string] $id) { Assert-Equal ([regex]::Matches($html, 'id="' + [regex]::Escape($id) + '"').Count) 1 "unique anchor $id" }
$source = Transform 0
Assert-Equal ([regex]::Matches($source, 'class="note-entry"').Count) 0 'source must keep notes in place'
Assert-Equal ([regex]::Matches($source, 'First note text\.').Count) 1 'source note one duplication'
Assert-Equal ([regex]::Matches($source, 'Second note text\.').Count) 1 'source note two duplication'
foreach ($placement in @(1, 2)) {
    $html = Transform $placement
    Assert-Equal ([regex]::Matches($html, 'class="notes"').Count) 1 "notes container for placement $placement"
    Assert-Equal ([regex]::Matches($html, 'class="note-entry"').Count) 2 "re-emitted notes for placement $placement"
    Assert-Equal ([regex]::Matches($html, 'First note text\.').Count) 1 "note one duplication for placement $placement"
    Assert-Equal ([regex]::Matches($html, 'Second note text\.').Count) 1 "note two duplication for placement $placement"
    Assert-Equal ([regex]::Matches($html, 'class="note-back"').Count) 3 "backlinks for placement $placement"
    Assert-Equal ([regex]::Matches($html, 'href="#_note_ref_').Count) 3 "backlink targets for placement $placement"
    Assert-UniqueAnchor $html 'note-one'; Assert-UniqueAnchor $html 'note-two'
}
Write-Host 'ExportHTML note placement regression passed.'