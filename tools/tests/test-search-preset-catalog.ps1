[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$PlatformToolset = "v143",
    [switch]$UsePreparedPcre2
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $repoRoot 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset $PlatformToolset
$installDir = Join-Path $repoRoot "build\pcre2\install\$Configuration"
if (-not $UsePreparedPcre2) {
    & (Join-Path $repoRoot 'tools\build\build-pcre2.ps1') -Configuration $Configuration -PlatformToolset $PlatformToolset -Quiet
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($path in @((Join-Path $installDir 'include\pcre2.h'), (Join-Path $installDir 'lib\pcre2-16-static.lib'))) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Не найдена PCRE2-зависимость: $path" }
}
$testDir = Join-Path $repoRoot 'out\tests\search-preset-catalog'
New-Item -ItemType Directory -Force -Path $testDir | Out-Null
$exe = Join-Path $testDir 'search-preset-catalog-smoke.exe'
& cl.exe /nologo /EHsc /std:c++17 /MT /DUNICODE /D_UNICODE /utf-8 `
    "/I$(Join-Path $repoRoot 'third_party\wtl')" `
    "/I$(Join-Path $repoRoot 'src\fbe')" `
    "/I$(Join-Path $installDir 'include')" `
    "/Fo$testDir\\" `
    (Join-Path $PSScriptRoot 'search-preset-catalog-smoke.cpp') `
    (Join-Path $repoRoot 'src\fbe\search\SearchPresetCatalog.cpp') `
    '/link' '/SUBSYSTEM:CONSOLE' "/LIBPATH:$(Join-Path $installDir 'lib')" 'pcre2-16-static.lib' "/OUT:$exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Search preset catalog smoke failed with exit code $LASTEXITCODE." }
Write-Host 'Search preset catalog regex contract passed.'
