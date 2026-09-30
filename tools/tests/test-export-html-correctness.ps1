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

function Invoke-ExportHtmlXsl([string]$xml, [hashtable]$parameters = @{}) {
    $document = New-Object -ComObject Msxml2.DOMDocument.6.0; $document.async = $false
    if (-not $document.loadXML($xml)) { throw $document.parseError.reason }
    $template = New-Object -ComObject Msxml2.XSLTemplate.6.0; $template.stylesheet = $xsl
    $processor = $template.createProcessor(); $processor.input = $document
    foreach ($parameter in $parameters.GetEnumerator()) { $processor.addParameter($parameter.Key, $parameter.Value, '') }
    [void]$processor.transform()
    return [string]$processor.output
}

$fb2 = @'
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink">
 <description><title-info><author><first-name>Аркадий</first-name><last-name>Стругацкий</last-name></author><author><first-name>Борис</first-name><last-name>Стругацкий</last-name></author><book-title>Пикник на обочине</book-title><lang>ru</lang><annotation><p>Аннотация с Unicode — NBSP&#160;и ©.</p></annotation><coverpage><image l:href="#cover.png"/></coverpage></title-info></description>
 <body><section id="chapter-one"><title><p>Глава один</p></title><p><a type="note" l:href="#n1">a</a> <a type="note" l:href="#n1">b</a> <a type="note" l:href="#n2">c</a> <a l:href="#chapter-two">Вперёд</a> <a l:href="https://example.test/export">External</a></p><p>Повест-{43}вуя; Набор {15}; Версия {2026}; f(x) = {15}; {-1}; NBSP&#160;© «Unicode».</p><epigraph><p>Эпиграф</p><text-author>Автор</text-author></epigraph><cite><p>Цитата</p></cite><poem><stanza><p>Строка один</p><p>Строка два</p></stanza><stanza><p>Строка три</p></stanza></poem><table><tr><th>Заголовок</th><td colspan="2">Ячейка</td></tr></table><image l:href="#inside-one.png"/><section id="nested"><title><p>Вложенная глава</p></title><p>Вложенный текст</p><image l:href="#inside-two.png"/></section><section id="n1"><title><p>Сноска один</p></title><p>One</p></section><section id="n2"><title><p>Сноска два</p></title><p>Two</p></section></section><section id="chapter-two"><title><p>Глава два</p></title><p>Вторая глава</p></section></body>
 <binary id="cover.png" content-type="image/png">iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAF/gL+MZ7M0QAAAABJRU5ErkJggg==</binary><binary id="inside-one.png" content-type="image/png">iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAF/gL+MZ7M0QAAAABJRU5ErkJggg==</binary><binary id="inside-two.png" content-type="image/png">iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAF/gL+MZ7M0QAAAABJRU5ErkJggg==</binary>
</FictionBook>
'@
$html = Invoke-ExportHtmlXsl $fb2 @{ saveimages = $true; embedimages = $true; customcss = 'body { color: rgb(1, 2, 3); }' }
if ($html -notmatch '<title>Аркадий Стругацкий, Борис Стругацкий — Пикник на обочине</title>') { throw 'Title author — book title is incorrect.' }
foreach ($expected in @('meta charset="utf-8"', 'name="author"', 'name="title"', 'name="description"', 'body { color: rgb(1, 2, 3); }', 'Table of contents', 'class="epigraph"', '<blockquote>', 'class="poem"', 'class="stanza"', 'class="fb2-table"', 'colspan="2"', 'href="#chapter-two"', 'href="https://example.test/export"')) {
    if ($html -notlike "*$expected*") { throw "Comprehensive fixture lost: $expected" }
}
if ([regex]::Matches($html, 'data:image/png;base64,').Count -ne 3) { throw 'Cover and inline images were not embedded from the comprehensive fixture.' }
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

foreach ($placement in @(0, 1, 2)) {
    $placed = Invoke-ExportHtmlXsl $fb2 @{ noteplacement = $placement }
    if ([regex]::Matches($placed, 'class="note-back"').Count -ne 3) { throw "Backlinks regressed for note placement $placement." }
    if ($placement -eq 0) {
        if ($placed -match 'class="note-entry"') { throw 'Source note placement must not re-emit note entries.' }
    } else {
        if ([regex]::Matches($placed, 'class="note-entry"').Count -ne 2 -or [regex]::Matches($placed, 'class="notes"').Count -ne 1) { throw "Placed notes regressed for mode $placement." }
    }
}

foreach ($case in @(
    @{ Xml = '<description><title-info><book-title>Only title</book-title></title-info></description>'; Expected = '<title>Only title</title>' },
    @{ Xml = '<description><title-info><author><nickname>Anon</nickname></author></title-info></description>'; Expected = '<title>Anon</title>' },
    @{ Xml = '<description><title-info/></description>'; Expected = '<title>FictionBook</title>' }
)) {
    $html = Invoke-ExportHtmlXsl ("<FictionBook xmlns=`"http://www.gribuser.ru/xml/fictionbook/2.0`">{0}<body><section><p>x</p></section></body></FictionBook>" -f $case.Xml)
    if ($html -notlike "*$($case.Expected)*") { throw "Title fallback failed: $($case.Expected)" }
}
Write-Host 'ExportHTML correctness regression passed.'
