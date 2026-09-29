<# Runs the production ExportHTML split path through FBE's headless host. #>
[CmdletBinding()]
param([string]$FbeExe = (Join-Path $PSScriptRoot '..\..\out\Release\FBE.exe'), [int]$TimeoutSeconds = 90)
$ErrorActionPreference = 'Stop'
$FbeExe = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($FbeExe)
if (-not (Test-Path -LiteralPath $FbeExe)) { throw "Не найден FBE: $FbeExe" }
$directory = Join-Path ([IO.Path]::GetTempPath()) ('fbe-export-html-split-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory | Out-Null
try {
    $png = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScL1kQAAAABJRU5ErkJggg=='
    $fixture = Join-Path $directory 'split.fb2'
    @"
<?xml version="1.0" encoding="utf-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink"><description><title-info><genre>prose</genre><book-title>Split</book-title><lang>en</lang></title-info></description><body><section id="first"><title><p>Глава / one</p></title><p><a l:href="#second">Next</a> <a type="note" l:href="#note">Note</a></p><image l:href="#picture.png"/></section><section id="second"><title><p>Глава / one</p></title><p><a l:href="#first">Back</a></p></section><section id="note"><p>Note text</p></section></body><binary id="picture.png" content-type="image/png">$png</binary></FictionBook>
"@ | Set-Content -LiteralPath $fixture -Encoding utf8
    $selected = Join-Path $directory 'chosen-name.html'
    $old = @($env:FBE_NEXT_TEST_MODE, $env:FBE_NEXT_TEST_SCENARIO, $env:FBE_NEXT_TEST_EXPORT_HTML_PATH, $env:FBE_NEXT_TEST_EXPORT_HTML_MODE, $env:FBE_NEXT_TEST_EXPORT_HTML_SPLIT)
    try {
        $env:FBE_NEXT_TEST_MODE='1'; $env:FBE_NEXT_TEST_SCENARIO='export-html'; $env:FBE_NEXT_TEST_EXPORT_HTML_PATH=$selected; $env:FBE_NEXT_TEST_EXPORT_HTML_MODE='1'; $env:FBE_NEXT_TEST_EXPORT_HTML_SPLIT='1'
        $report = Join-Path $directory 'split.tsv'; $process = Start-Process -FilePath $FbeExe -ArgumentList @('-b', $report, $fixture) -PassThru
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $process.Id -Force; throw 'FBE did not finish split export.' }
        if ($process.ExitCode -ne 0) { throw "FBE split export failed: $($process.ExitCode)" }

    } finally { $env:FBE_NEXT_TEST_MODE,$env:FBE_NEXT_TEST_SCENARIO,$env:FBE_NEXT_TEST_EXPORT_HTML_PATH,$env:FBE_NEXT_TEST_EXPORT_HTML_MODE,$env:FBE_NEXT_TEST_EXPORT_HTML_SPLIT = $old }
    $index = Join-Path $directory 'index.html'; if (-not (Test-Path -LiteralPath $index)) { throw 'Split export did not create index.html.' }
    $pages = @((Get-ChildItem -LiteralPath $directory -Filter 'section-*.html').FullName); if ($pages.Count -ne 3) { throw "Expected three top-level section files, got $($pages.Count)." }
    if ((@($pages | Select-Object -Unique).Count -ne 3) -or $pages | Where-Object { $_ -match '\.\.' -or $_ -match '[\\/](?:\\|/)' }) { throw 'Split filenames are not unique and safe.' }
    $indexText = Get-Content -Raw -LiteralPath $index; if ($indexText -notmatch 'section-1-.*#_toc_' -or $indexText -match '<h2>Глава') { throw 'Index TOC or section removal regressed.' }
    $pageText = Get-Content -Raw -LiteralPath $pages[0]; if ($pageText -notmatch 'section-2-.*#second' -or $pageText -notmatch 'section-3-.*#note') { throw 'Cross-section or note links were not rewritten.' }
    $noteText = Get-Content -Raw -LiteralPath $pages[2]; if ($noteText -notmatch 'section-1-.*#_note_ref_') { throw 'Split note backlink was not rewritten.' }
    $images = Join-Path $directory 'index_files'; if (-not (Test-Path -LiteralPath (Join-Path $images 'picture.png'))) { throw 'Split export did not use one shared external image directory.' }
    if ($pageText -notmatch 'index_files/picture.png') { throw 'Split page does not retain the shared image prefix.' }
    Write-Host 'ExportHTML split production E2E passed.'
} finally { Remove-Item -LiteralPath $directory -Recurse -Force -ErrorAction SilentlyContinue }