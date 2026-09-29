<# Verifies HtmlExportSettings-to-XSL behaviour without invoking the UI. #>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$bridge = Get-Content -Raw (Join-Path $root 'src\export-html\HtmlExportXslParameters.cpp')
foreach ($parameter in @('includetoc', 'includemetadata', 'style', 'fontfamily', 'customfontfamily', 'fontsize', 'lineheight', 'contentmaxwidth', 'pagemargins', 'textalignment', 'headingalignment', 'covermode', 'imagemaxwidth', 'imagemaxheight')) {
    if ($bridge -notmatch ('L"' + $parameter + '"')) { throw "XSL parameter is not wired: $parameter" }
}
$xsl = New-Object -ComObject Msxml2.DOMDocument.6.0; $xsl.async = $false
if (-not $xsl.load((Join-Path $root 'runtime\html.xsl'))) { throw $xsl.parseError.reason }
$xml = New-Object -ComObject Msxml2.DOMDocument.6.0; $xml.async = $false
if (-not $xml.loadXML('<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0"><description><title-info><author><first-name>A</first-name><last-name>B</last-name></author><book-title>T</book-title><annotation><p>Annotation</p></annotation></title-info></description><body><section><title><p>One</p></title><p>Text</p></section></body></FictionBook>')) { throw $xml.parseError.reason }
$template = New-Object -ComObject Msxml2.XSLTemplate.6.0; $template.stylesheet = $xsl
$processor = $template.createProcessor(); $processor.input = $xml
@{ includetoc = $false; includemetadata = $false; style = 1; fontfamily = 1; fontsize = 18; lineheight = 150; contentmaxwidth = 960; pagemargins = 2; textalignment = 0; headingalignment = 1; covermode = 1; imagemaxwidth = 640; imagemaxheight = 480; customcss = 'body { color: red; }' }.GetEnumerator() | ForEach-Object { $processor.addParameter($_.Key, $_.Value, '') }
[void]$processor.transform(); $html = [string]$processor.output
foreach ($unexpected in @('Table of contents', 'name="author"', 'name="title"', 'name="description"', 'class="props"')) { if ($html -match [regex]::Escape($unexpected)) { throw "Disabled setting was emitted: $unexpected" } }
foreach ($expected in @('font-family: sans-serif', 'font-size: 18px', 'line-height: 1.5', 'max-width: 960px', 'margin: 3em', 'text-align: left', 'body { color: red; }')) { if ($html -notmatch [regex]::Escape($expected)) { throw "Setting was not rendered: $expected" } }
Write-Host 'ExportHTML extended XSL settings regression passed.'
$sourceXsl = Get-Content -Raw (Join-Path $root 'src\export-html\html.xsl')
$runtimeXsl = Get-Content -Raw (Join-Path $root 'runtime\html.xsl')
if ($sourceXsl -ne $runtimeXsl) { throw 'Source and runtime HTML XSL files diverged.' }
if ($runtimeXsl -match '\$includedesc') { throw 'The v2 HTML XSL must not depend on legacy includedesc.' }
function Invoke-ExportHtmlXsl([hashtable]$parameters) {
    $run = $template.createProcessor(); $run.input = $xml
    foreach ($pair in $parameters.GetEnumerator()) { $run.addParameter($pair.Key, $pair.Value, '') }
    [void]$run.transform()
    return [string]$run.output
}
$metadataWithoutAuthors = Invoke-ExportHtmlXsl @{ includetoc = $true; includemetadata = $true; includeannotation = $true; includetitleinfo = $true; includedocumentinfo = $false; includepublishinfo = $false; includecustominfo = $false; includeauthors = $false; includetranslators = $false }
foreach ($unexpected in @('name="author"', '<td>Author', '>A B<')) { if ($metadataWithoutAuthors -match [regex]::Escape($unexpected)) { throw "Authors OFF was ignored: $unexpected" } }
if ($metadataWithoutAuthors -notmatch 'Title Info') { throw 'Enabled Title Info metadata was not emitted.' }
$metadataWithoutAnnotation = Invoke-ExportHtmlXsl @{ includetoc = $true; includemetadata = $true; includeannotation = $false; includetitleinfo = $true; includedocumentinfo = $false; includepublishinfo = $false; includecustominfo = $false; includeauthors = $true; includetranslators = $true }
foreach ($unexpected in @('href="#_fbh_annotation"', 'name="description"', 'class="annotation"')) { if ($metadataWithoutAnnotation -match [regex]::Escape($unexpected)) { throw "Annotation OFF was ignored: $unexpected" } }
if ($metadataWithoutAnnotation -notmatch 'href="#_fbh_description"') { throw 'Metadata TOC link was not emitted for enabled Title Info.' }
$metadataDisabled = Invoke-ExportHtmlXsl @{ includetoc = $true; includemetadata = $false; includeannotation = $true; includetitleinfo = $true; includedocumentinfo = $true; includepublishinfo = $true; includecustominfo = $true; includeauthors = $true; includetranslators = $true }
foreach ($unexpected in @('href="#_fbh_annotation"', 'href="#_fbh_description"', 'class="props"')) { if ($metadataDisabled -match [regex]::Escape($unexpected)) { throw "Metadata master flag was ignored: $unexpected" } }
Write-Host 'ExportHTML metadata master/child and XSL parity regression passed.'