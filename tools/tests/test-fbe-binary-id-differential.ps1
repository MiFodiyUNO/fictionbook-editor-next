[CmdletBinding()]
param([string]$PlatformToolset = 'v143')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $root 'tools\build\Import-VsDevEnvironment.ps1') -Arch x86 -HostArch x64 -PlatformToolset $PlatformToolset
$testDirectory = Join-Path $root 'out\tests\binary-id-differential'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
$native = Join-Path $testDirectory 'binary-id-differential-smoke.exe'
& cl.exe /nologo /EHsc /std:c++17 /MT /DUNICODE /D_UNICODE /utf-8 "/I$root\third_party\wtl" "/I$root\src\fbe" `
    (Join-Path $PSScriptRoot 'binary-id-differential-smoke.cpp') /link /SUBSYSTEM:CONSOLE "/OUT:$native"
if ($LASTEXITCODE -ne 0) { throw "Could not build binary-ID native validator: $LASTEXITCODE" }

$source = Get-Content -Raw -LiteralPath (Join-Path $root 'runtime\main.js')
$validator = [regex]::Match($source, '(?s)function IsBinaryXmlLetterCodeUnit\(code\).*?(?=function IsBinaryReferenceValue)').Value
if ([string]::IsNullOrWhiteSpace($validator)) { throw 'Could not extract production binary-ID validator from runtime/main.js.' }
$runner = Join-Path $env:TEMP ('fbe-binary-id-validator-' + [guid]::NewGuid().ToString('N') + '.js')
try {
    [IO.File]::WriteAllText($runner, $validator + "`r`nvar argument = WScript.Arguments.length ? WScript.Arguments.Item(0) : ''; WScript.Echo(IsBinaryXmlId(argument) ? '1' : '0');`r`n", [Text.Encoding]::ASCII)
    $cases = @(
        '', '_', 'A', 'a', '1start', '-start', '.start', 'name-1.2', 'name space', 'name!symbol',
        [string][char]0x00bf, [string][char]0x00c0, [string][char]0x02ff, [string][char]0x0300,
        [string][char]0x036f, [string][char]0x0370, [string][char]0x1fff, [string][char]0x2000,
        [string][char]0x206f, [string][char]0x2070, [string][char]0x218f, [string][char]0x2190,
        [string][char]0x2bff, [string][char]0x2c00, [string][char]0x2fef, [string][char]0x2ff0,
        [string][char]0x3000, [string][char]0x3001, [string][char]0xd7ff, [string][char]0xd800,
        [string][char]0xf8ff, [string][char]0xf900, [string][char]0xfdcf, [string][char]0xfdd0,
        [string][char]0xfdf0, [string][char]0xfffd, [string][char]0xfffe,
        'Обложка_1-2.jpg', 'Àrbre-1', ([char]0x00d7 + 'symbol'), ('a' + [char]0x00d7)
    )
    foreach ($case in $cases) {
        $nativeResult = ([string](& $native $case)).Trim()
        if ($LASTEXITCODE -ne 0) { throw "Native binary-ID validator failed for case '$case'." }
        $jsResult = ([string](& cscript.exe //nologo $runner $case)).Trim()
        if ($LASTEXITCODE -ne 0) { throw "JScript binary-ID validator failed for case '$case'." }
        if ($nativeResult -notin @('0', '1') -or $jsResult -notin @('0', '1')) { throw "Invalid validator output for case '$case': native=$nativeResult, js=$jsResult" }
        if ($nativeResult -ne $jsResult) { throw "Binary-ID validator mismatch for U+$([string]::Join('-', ($case.ToCharArray() | ForEach-Object { '{0:X4}' -f [int][char]$_ }))): C++=$nativeResult, JS=$jsResult" }
    }
} finally {
    Remove-Item -LiteralPath $runner -Force -ErrorAction SilentlyContinue
}
Write-Host "Binary-ID C++/JScript differential contract passed ($($cases.Count) boundary cases)."