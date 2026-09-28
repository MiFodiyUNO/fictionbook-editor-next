<# Проверяет regressions Phase 1 ExportHTML: notes, title и сохранность текста. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$plugin = Get-Content -Raw -LiteralPath (Join-Path $root 'src\export-html\ExportHTMLPlugin.cpp')
if ($plugin -match 'RemoveServiceMarkers|regex_replace|std::wregex') {
    throw 'ExportHTML не должен эвристически удалять фигурные скобки из текста книги.'
}

$sourceXsl = Join-Path $root 'src\export-html\html.xsl'
$runtimeXsl = Join-Path $root 'runtime\html.xsl'
if ((Get-FileHash -LiteralPath $sourceXsl -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $runtimeXsl -Algorithm SHA256).Hash) {
    throw 'runtime/html.xsl должен совпадать с исходным XSL экспортера.'
}
$xsl = New-Object -ComObject Msxml2.DOMDocument.6.0; $xsl.async = $false
if (-not $xsl.load($runtimeXsl)) { throw $xsl.parseError.reason }

function Invoke-ExportHtmlXsl([string]$xml) {
    $document = New-Object -ComObject Msxml2.DOMDocument.6.0; $document.async = $false
    if (-not $document.loadXML($xml)) { throw $document.parseError.reason }
    $template = New-Object -ComObject Msxml2.XSLTemplate.6.0; $template.stylesheet = $xsl
    $processor = $template.createProcessor(); $processor.input = $document
    [void]$processor.transform()
    return [string]$processor.output
}

$fb2 = @'
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink">
 <description><title-info><author><first-name>Аркадий</first-name><last-name>Стругацкий</last-name></author><author><first-name>Борис</first-name><last-name>Стругацкий</last-name></author><book-title>Пикник на обочине</book-title></title-info></description>
 <body><section><p><a type="note" l:href="#n1">a</a> <a type="note" l:href="#n1">b</a> <a type="note" l:href="#n2">c</a></p><p>Повест-{43}вуя; Набор {15}; Версия {2026}; f(x) = {15}; {-1}</p></section></body>
 <body name="notes"><section id="n1"><p>One</p></section><section id="n2"><p>Two</p></section></body>
</FictionBook>
'@
$html = Invoke-ExportHtmlXsl $fb2
if ($html -notmatch '<title>Аркадий Стругацкий, Борис Стругацкий — Пикник на обочине</title>') { throw 'Title author — book title is incorrect.' }
foreach ($text in @('Повест-{43}вуя', 'Набор {15}', 'Версия {2026}', 'f(x) = {15}', '{-1}')) {
    if ($html -notlike "*$text*") { throw "Legitimate source text was changed: $text" }
}
$refs = [regex]::Matches($html, 'class="note" id="(?<id>[^"]+)"')
if ($refs.Count -ne 3) { throw 'Expected three independent note reference anchors.' }
$backlinks = [regex]::Matches($html, 'class="note-back" href="#(?<id>[^"]+)"')
if ($backlinks.Count -ne 3) { throw 'Expected one backlink for every note reference.' }
$backlinkIds = @($backlinks | ForEach-Object { $_.Groups['id'].Value })
foreach ($reference in $refs) {
    if ($backlinkIds -notcontains $reference.Groups['id'].Value) { throw "Missing backlink for $($reference.Groups['id'].Value)." }
}
if ($html -notmatch '↩1' -or $html -notmatch '↩2') { throw 'Repeated note must show distinct backlinks.' }

foreach ($case in @(
    @{ Xml = '<description><title-info><book-title>Only title</book-title></title-info></description>'; Expected = '<title>Only title</title>' },
    @{ Xml = '<description><title-info><author><nickname>Anon</nickname></author></title-info></description>'; Expected = '<title>Anon</title>' },
    @{ Xml = '<description><title-info/></description>'; Expected = '<title>FictionBook</title>' }
)) {
    $html = Invoke-ExportHtmlXsl ("<FictionBook xmlns=`"http://www.gribuser.ru/xml/fictionbook/2.0`">{0}<body><section><p>x</p></section></body></FictionBook>" -f $case.Xml)
    if ($html -notlike "*$($case.Expected)*") { throw "Title fallback failed: $($case.Expected)" }
}
Write-Host 'ExportHTML correctness regression passed.'
