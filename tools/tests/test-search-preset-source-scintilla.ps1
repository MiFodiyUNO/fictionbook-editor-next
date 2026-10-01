[CmdletBinding()]
param([string]$FbeExe = (Join-Path $PSScriptRoot '..\..\out\Release\FBE.exe'), [string]$PlatformToolset = 'v143')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset $PlatformToolset
$testDir = Join-Path ([IO.Path]::GetTempPath()) ('fbe-search-preset-source-scintilla-' + [guid]::NewGuid().ToString('N')); New-Item -ItemType Directory -Force -Path $testDir | Out-Null
$exe = Join-Path $testDir 'search-preset-source-scintilla-smoke.exe'
& cl.exe /nologo /EHsc /std:c++17 /MT /DUNICODE /D_UNICODE /utf-8 "/I$root\third_party\wtl" "/I$root\src\fbe" "/I$root\third_party\scintilla\include" "/I$root\third_party\lexilla\include" "/Fo$testDir\\" (Join-Path $PSScriptRoot 'search-preset-source-scintilla-smoke.cpp') (Join-Path $root 'src\fbe\search\SearchPresetCatalog.cpp') /link /SUBSYSTEM:CONSOLE "/OUT:$exe"
if($LASTEXITCODE -ne 0){ exit $LASTEXITCODE }
$runtime = Split-Path -Parent (Resolve-Path $FbeExe); Copy-Item -LiteralPath (Join-Path $runtime 'Scintilla.dll') -Destination (Join-Path $testDir 'Scintilla.dll') -Force; Copy-Item -LiteralPath (Join-Path $runtime 'Lexilla.dll') -Destination (Join-Path $testDir 'Lexilla.dll') -Force
& $exe
if($LASTEXITCODE -ne 0){ throw "Source preset Scintilla smoke failed with exit code $LASTEXITCODE." }
Write-Host 'All built-in Source presets passed real Scintilla fixtures.'