<# Builds and executes direct unit coverage for standalone-resource auditing. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset v143
$out = Join-Path $root 'out\tests\export-html-resource-audit.exe'
$obj = Join-Path $root 'out\tests\export-html-resource-audit'
New-Item -ItemType Directory -Force -Path $obj | Out-Null
try {
    & cl.exe /nologo /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE "/I$(Join-Path $root 'src\export-html')" "/I$(Join-Path $root 'third_party')" "/Fo:$obj\\" (Join-Path $PSScriptRoot 'export-html-resource-audit-harness.cpp') (Join-Path $root 'src\export-html\HtmlExportResourceAudit.cpp') /link "/OUT:$out"
    if ($LASTEXITCODE -ne 0) { throw 'ExportHTML resource audit harness did not compile.' }
    & $out
    if ($LASTEXITCODE -ne 0) { throw "ExportHTML resource audit harness failed: $LASTEXITCODE" }
}
finally {
    Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ($out -replace '\.exe$', '.pdb') -Force -ErrorAction SilentlyContinue
}
Write-Host 'ExportHTML standalone resource audit passed.'
