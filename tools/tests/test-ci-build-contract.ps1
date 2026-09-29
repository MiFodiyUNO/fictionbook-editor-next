<# CI contract: production shell build scripts, UTF-8 console setup and output locations. #>
[CmdletBinding()]
param(
    [string]$Configuration = 'Release',
    [switch]$RequireArtifacts
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$workflowPath = Join-Path $root '.github\workflows\build.yml'
$workflow = Get-Content -Raw -LiteralPath $workflowPath

if ($workflow.Contains('experimental-property-handler') -or $workflow -match '(?i)experimental') {
    throw 'CI workflow must not use obsolete experimental shell-integration naming.'
}
foreach ($required in @(
    'Build Win32 shell integration',
    'Build x64 shell integration',
    './tools/build/build-shell-integration.ps1 -Configuration Release -Platform Win32 -PlatformToolset v143',
    './tools/build/build-shell-integration.ps1 -Configuration Release -Platform x64 -PlatformToolset v143',
    './tools/build/Initialize-CiUtf8.ps1',
    './tools/tests/test-first-party-msbuild-policy.ps1',
    './tools/tests/test-plugin-localization-catalog.ps1',
    './tools/tests/test-ci-build-contract.ps1 -RequireArtifacts')) {
    if (-not $workflow.Contains($required)) { throw "CI workflow is missing '$required'." }
}
$localizationStep = $workflow.IndexOf('- name: Validate plugin localization catalog')
$validateFixtures = $workflow.IndexOf('- name: Initialize MSBuild policy fixtures')
if ($localizationStep -lt 0 -or $localizationStep -gt $validateFixtures) {
    throw 'Plugin localization catalog must run before validate initializes submodules and later checks.'
}

if ($workflow -notmatch '(?m)^\s*build:\s*\r?\n\s*needs:\s*validate\s*$' -or
    $workflow -notmatch '(?m)^\s*package:\s*\r?\n\s*if:.*\r?\n\s*needs:\s*\[validate, build\]\s*$' -or
    $workflow -notmatch '(?m)^\s*publish:\s*\r?\n\s*if:.*\r?\n\s*needs:\s*\[validate, package\]\s*$') {
    throw 'Build, package and publish must depend on successful validate.'
}

foreach ($artifact in @(
        @{ Name = 'editor background runtime diagnostics'; Output = 'editor_background_has_files'; Path = 'out/tests/editor-background-runtime-failure/**' },
        @{ Name = 'failed runtime diagnostics'; Output = 'runtime_has_files'; Path = 'out/tests/runtime-failure/**' }
    )) {
    $pattern = "(?s)- name: Upload $([regex]::Escape($artifact.Name)).*?if: failure\(\) && steps\.runtime-diagnostics\.outputs\.$($artifact.Output) == 'True'.*?path: $([regex]::Escape($artifact.Path)).*?if-no-files-found: error"
    if ($workflow -notmatch $pattern) {
        throw "CI must upload $($artifact.Name) only when its diagnostic files exist."
    }
}

$validateCheckout = [regex]::Match($workflow, '(?s)validate:.*?actions/checkout@v7\s*\r?\n\s*with:(?<options>.*?)\r?\n\s*# validate reads.*?- name: Initialize MSBuild policy fixtures\s*\r?\n\s*shell: pwsh\s*\r?\n\s*run: (?<command>.*?)\r?\n\s*- name: Check generated')
if(-not $validateCheckout.Success -or $validateCheckout.Groups['options'].Value -match '(?m)^\s*submodules:\s*recursive\s*$' -or
    $validateCheckout.Groups['command'].Value -notmatch 'git submodule update --init --depth=1 third_party/lexilla third_party/hunspell') {
    throw 'Validate must initialize only the vendored MSBuild policy fixtures.'
}

foreach ($match in [regex]::Matches($workflow, '(?m)(?:\./|\.\\)(tools[\\/][A-Za-z0-9_.\\/-]+\.ps1)')) {
    $relativePath = $match.Groups[1].Value -replace '/', '\\'
    if (-not (Test-Path -LiteralPath (Join-Path $root $relativePath) -PathType Leaf)) {
        throw "CI workflow references missing script: $relativePath"
    }
}

foreach ($scriptName in @('build-libde265.ps1', 'build-aom.ps1', 'build-libheif.ps1')) {
    $scriptText = Get-Content -Raw -LiteralPath (Join-Path $root "tools\build\$scriptName")
    if (-not $scriptText.Contains("'-DCMAKE_SYSTEM_VERSION=6.1'") -or
        $scriptText -match '(?<![\x27\x22])-DCMAKE_SYSTEM_VERSION=6\.1') {
        throw "$scriptName must pass CMAKE_SYSTEM_VERSION=6.1 as one quoted native argument."
    }
}

$buildScript = Get-Content -Raw -LiteralPath (Join-Path $root 'tools\build\build.ps1')
if ([regex]::Matches($buildScript, '(?i)\$msbuild[^\r\n]*?/nr:false').Count -lt 2) {
    throw 'The public build entry point must disable MSBuild node reuse for both solution and required-project builds.'
}
foreach ($required in @('Get-FirstPartyToolchainFingerprint', 'Test-FirstPartyToolchainFingerprint', 'first-party-{0}-{1}.json', '/t:Clean')) {
    if (-not $buildScript.Contains($required)) {
        throw "The public build entry point is missing the first-party toolchain fingerprint contract: $required"
    }
}

$utf8Bootstrap = Join-Path $root 'tools\build\Initialize-CiUtf8.ps1'
& $utf8Bootstrap
$expected = 'Проверка UTF-8: Ёж'
$nativeOutput = (& cmd.exe /d /c "echo $expected").Trim()
if ($nativeOutput -ne $expected -or $nativeOutput -match ('[?' + [char]0xFFFD + ']')) {
    throw "UTF-8 console regression: expected '$expected', got '$nativeOutput'."
}

if ($RequireArtifacts) {
    foreach ($platform in @('Win32', 'x64')) {
        $artifact = Join-Path $root "out\package\shell-build\$platform\$Configuration\FBShell.dll"
        if (-not (Test-Path -LiteralPath $artifact -PathType Leaf) -or (Get-Item -LiteralPath $artifact).Length -eq 0) {
            throw "Shell integration artifact is missing or empty: $artifact"
        }
    }
}

Write-Host 'CI shell integration and UTF-8 contract passed.'
