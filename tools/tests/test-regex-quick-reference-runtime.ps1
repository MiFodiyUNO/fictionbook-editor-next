<# Compiles a native fixture that exercises the real ComboBox and popup HWND lifecycle. #>
[CmdletBinding()]
param(
    [string]$PlatformToolset = 'v143'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset $PlatformToolset
$out = Join-Path $root 'out\tests\regex-quick-reference-runtime'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$exe = Join-Path $out 'regex-quick-reference-runtime-smoke.exe'
& cl.exe /nologo /EHsc /std:c++17 /MD /utf-8 /DUNICODE /D_UNICODE `
    "/I$(Join-Path $root 'third_party\wtl')" `
    "/I$(Join-Path $root 'src\fbe')" `
    "/Fo$out\\" `
    (Join-Path $PSScriptRoot 'regex-quick-reference-runtime-smoke.cpp') `
    (Join-Path $root 'src\fbe\search\ui\RegexQuickReferencePopup.cpp') `
    (Join-Path $root 'src\fbe\UiMetrics.cpp') `
    /link /SUBSYSTEM:CONSOLE comctl32.lib user32.lib gdi32.lib ole32.lib oleaut32.lib comsuppw.lib "/OUT:$exe"
if ($LASTEXITCODE -ne 0) { throw 'Regex quick-reference native runtime fixture did not compile.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Regex quick-reference native runtime smoke failed with exit code $LASTEXITCODE." }
Write-Host 'Regex quick-reference native runtime smoke passed.'
