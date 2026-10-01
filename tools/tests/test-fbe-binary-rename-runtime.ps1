<# Exercises binary rename through the production MSHTML document and validates Save/Reopen persistence. #>
[CmdletBinding()]
param([string]$FbeExe = (Join-Path $PSScriptRoot '..\..\out\Release\FBE.exe'), [int]$TimeoutSeconds = 120)
$ErrorActionPreference = 'Stop'
$FbeExe = (Resolve-Path -LiteralPath $FbeExe).Path
$directory = Join-Path ([IO.Path]::GetTempPath()) ('fbe-binary-rename-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory | Out-Null
try {
    $fixture = Join-Path $directory 'rename.fb2'; $report = Join-Path $directory 'rename.txt'; $reopen = Join-Path $directory 'reopen.txt'
    $bytes = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAF/gL+MZ7M0QAAAABJRU5ErkJggg=='
    @"
<?xml version="1.0" encoding="utf-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink"><description><title-info><genre>prose</genre><author><first-name>T</first-name><last-name>T</last-name></author><book-title>rename</book-title><lang>en</lang><coverpage><image l:href="#cover-old.png"/></coverpage></title-info><src-title-info><genre>prose</genre><author><first-name>S</first-name><last-name>S</last-name></author><book-title>source rename</book-title><lang>en</lang><coverpage><image l:href="#cover-old.png"/></coverpage></src-title-info><document-info><id>rename-test</id><version>1.0</version></document-info></description><body><section><p><image l:href="#cover-old.png"/></p><p>Inline <image l:href="#cover-old.png"/> image</p></section></body><binary id="cover-old.png" content-type="image/png">$bytes</binary><binary id="other.png" content-type="image/png">$bytes</binary></FictionBook>
"@ | Set-Content -LiteralPath $fixture -Encoding utf8
    $oldMode,$oldScenario=$env:FBE_NEXT_TEST_MODE,$env:FBE_NEXT_TEST_SCENARIO
    try {
        $env:FBE_NEXT_TEST_MODE='1'; $env:FBE_NEXT_TEST_SCENARIO='binary-rename-runtime'
        $process=Start-Process -FilePath $FbeExe -ArgumentList @('-b',$report,$fixture) -PassThru
        $deadline=[Diagnostics.Stopwatch]::StartNew()
        while(((-not (Test-Path -LiteralPath $report)) -or (Get-Item -LiteralPath $report).Length -eq 0) -and -not $process.HasExited -and $deadline.Elapsed.TotalSeconds -lt $TimeoutSeconds){Start-Sleep -Milliseconds 100}
        if((-not (Test-Path -LiteralPath $report)) -or (Get-Item -LiteralPath $report).Length -eq 0){if(-not $process.HasExited){Stop-Process -Id $process.Id -Force};throw 'FBE binary rename runtime did not write its report.'}
        if(-not $process.HasExited){Stop-Process -Id $process.Id -Force; $process.WaitForExit()}    } finally {$env:FBE_NEXT_TEST_MODE=$oldMode;$env:FBE_NEXT_TEST_SCENARIO=$oldScenario}
    $rows=@{}; Get-Content -LiteralPath $report | ForEach-Object {$pair=$_ -split '=',2;if($pair.Count -eq 2){$rows[$pair[0]]=$pair[1]}}
    if($rows.dom_rename -ne '1' -or $rows.body_source_roundtrip -ne '1' -or $rows.saved -ne '1'){throw "Binary rename runtime report failed: $(Get-Content $report -Raw)"}
    [xml]$xml=Get-Content -LiteralPath $fixture -Raw; $ns=[Xml.XmlNamespaceManager]::new($xml.NameTable);$ns.AddNamespace('fb','http://www.gribuser.ru/xml/fictionbook/2.0')
    foreach($path in @('//fb:title-info/fb:coverpage/fb:image','//fb:src-title-info/fb:coverpage/fb:image','//fb:body//fb:image')){foreach($image in @($xml.SelectNodes($path,$ns))){if($image.GetAttribute('href','http://www.w3.org/1999/xlink') -ne '#cover-renamed.png'){throw "Saved reference was not renamed: $path"}}}
    if($null -eq $xml.SelectSingleNode('/fb:FictionBook/fb:binary[@id="cover-renamed.png"]',$ns)){throw 'Saved binary with new ID is missing.'}
    if($xml.OuterXml -match 'cover-old\.png'){throw 'Old binary ID survived the save.'}
    $process=Start-Process -FilePath $FbeExe -ArgumentList @('-b',$reopen,$fixture) -PassThru
    if(-not $process.WaitForExit($TimeoutSeconds*1000)){Stop-Process -Id $process.Id -Force;throw 'FBE binary rename reopen timed out.'}
    if($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $reopen)){throw 'FBE binary rename reopen failed.'}
    Write-Host 'FBE production binary rename -> Save -> reopen passed.'
} finally {Remove-Item -LiteralPath $directory -Recurse -Force -ErrorAction SilentlyContinue}