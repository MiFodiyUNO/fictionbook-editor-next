[CmdletBinding()]
param(
    [string]$PlatformToolset = "v143"
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$implementation = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'src/fbe/search/SearchPresetStore.cpp')
if ($implementation -notmatch 'DeploymentContext::SettingsDirectory\(\)') { throw 'SearchPresetStore must use DeploymentContext::SettingsDirectory().' }
if ($implementation -match 'Registry|CRegKey') { throw 'SearchPresetStore must not use the Registry.' }
& (Join-Path $repoRoot 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset $PlatformToolset
$testDir = Join-Path $repoRoot 'out\tests\search-preset-store'
New-Item -ItemType Directory -Force -Path $testDir | Out-Null
$exe = Join-Path $testDir 'search-preset-store-smoke.exe'
& cl.exe /nologo /EHsc /std:c++17 /MT /DUNICODE /D_UNICODE /utf-8 /DFBE_SEARCH_PRESET_STORE_TESTING `
    "/I$(Join-Path $repoRoot 'third_party\wtl')" `
    "/I$(Join-Path $repoRoot 'src\fbe')" `
    "/Fo$testDir\\" `
    (Join-Path $PSScriptRoot 'search-preset-store-smoke.cpp') `
    (Join-Path $repoRoot 'src\fbe\search\SearchPresetStore.cpp') `
    "/link" /SUBSYSTEM:CONSOLE ole32.lib oleaut32.lib shell32.lib "/OUT:$exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $exe
if ($LASTEXITCODE -ne 0) { throw "SearchPresetStore smoke failed with exit code $LASTEXITCODE." }
Write-Host 'SearchPresetStore contract passed.'