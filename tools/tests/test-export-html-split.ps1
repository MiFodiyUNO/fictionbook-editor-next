<# Exercises deterministic split planning, filenames and cross-document links. #>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset v143
$out = Join-Path $root 'out\tests\export-html-split.exe'
$obj = Join-Path $root 'out\tests\export-html-split'
New-Item -ItemType Directory -Force -Path $obj | Out-Null
try {
    & cl.exe /nologo /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE "/I$(Join-Path $root 'src\export-html')" "/I$(Join-Path $root 'third_party')" "/Fo:$obj\\" (Join-Path $PSScriptRoot 'export-html-split-harness.cpp') (Join-Path $root 'src\export-html\HtmlSplitExport.cpp') /link "/OUT:$out"
    if ($LASTEXITCODE -ne 0) { throw 'ExportHTML split harness did not compile.' }
    & $out
    if ($LASTEXITCODE -ne 0) { throw "ExportHTML split harness failed: $LASTEXITCODE" }
}
finally {
    Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ($out -replace '\.exe$', '.pdb') -Force -ErrorAction SilentlyContinue
}
Write-Host 'ExportHTML split test passed.'