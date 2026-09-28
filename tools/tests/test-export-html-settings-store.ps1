<# Covers settings defaults, old-key migration, clamping and Unicode persistence. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset v143
$out = Join-Path $root 'out\tests\export-html-settings-harness.exe'
$obj = Join-Path $root 'out\tests\export-html-settings-harness'
New-Item -ItemType Directory -Force -Path $obj | Out-Null
try {
    & cl.exe /nologo /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE "/I$(Join-Path $root 'src\export-html')" "/I$(Join-Path $root 'third_party')" "/Fo:$obj\\" (Join-Path $PSScriptRoot 'export-html-settings-harness.cpp') (Join-Path $root 'src\export-html\HtmlExportSettings.cpp') (Join-Path $root 'src\export-html\RuntimeLocalization.cpp') (Join-Path $root 'src\export-html\Utils.cpp') /link ole32.lib oleaut32.lib shell32.lib comdlg32.lib comctl32.lib "/OUT:$out"
    if ($LASTEXITCODE -ne 0) { throw 'ExportHTML settings harness did not compile.' }
    & $out
    if ($LASTEXITCODE -ne 0) { throw "ExportHTML settings harness failed: $LASTEXITCODE" }
}
finally {
    Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ($out -replace '\.exe$', '.pdb') -Force -ErrorAction SilentlyContinue
}
Write-Host 'ExportHTML settings store regression passed.'
