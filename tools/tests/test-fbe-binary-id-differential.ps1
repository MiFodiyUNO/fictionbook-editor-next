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
        @{ Id = ''; Expected = $false }, @{ Id = '_'; Expected = $true }, @{ Id = 'A'; Expected = $true }, @{ Id = 'a'; Expected = $true },
        @{ Id = '1start'; Expected = $false }, @{ Id = '-start'; Expected = $false }, @{ Id = '.start'; Expected = $false },
        @{ Id = 'name-1.2'; Expected = $true }, @{ Id = 'name space'; Expected = $false }, @{ Id = 'name!symbol'; Expected = $false },
        @{ Id = [string][char]0x00d6; Expected = $true }, @{ Id = [string][char]0x00d7; Expected = $false },
        @{ Id = [string][char]0x00d8; Expected = $true }, @{ Id = [string][char]0x00f6; Expected = $true },
        @{ Id = [string][char]0x00f7; Expected = $false }, @{ Id = [string][char]0x00f8; Expected = $true },
        @{ Id = [string][char]0x037d; Expected = $true }, @{ Id = [string][char]0x037e; Expected = $false },
        @{ Id = [string][char]0x037f; Expected = $true }, @{ Id = [string][char]0x200c; Expected = $true },
        @{ Id = [string][char]0x200d; Expected = $true }, @{ Id = [string][char]0x200e; Expected = $false },
        @{ Id = [string][char]0x02ff; Expected = $true }, @{ Id = [string][char]0x0300; Expected = $false },
        @{ Id = ('A' + [char]0x00b7); Expected = $true }, @{ Id = ('A' + [char]0x0300); Expected = $true },
        @{ Id = ('A' + [char]0x036f); Expected = $true }, @{ Id = ('A' + [char]0x203f); Expected = $true },
        @{ Id = ('A' + [char]0x2040); Expected = $true }, @{ Id = ('A' + [char]0x2041); Expected = $false },
        @{ Id = [string][char]0x3000; Expected = $false }, @{ Id = [string][char]0x3001; Expected = $true },
        @{ Id = [string][char]0xd7ff; Expected = $true }, @{ Id = [string][char]0xd800; Expected = $false },
        @{ Id = [string][char]0xf8ff; Expected = $false }, @{ Id = [string][char]0xf900; Expected = $true },
        @{ Id = [string][char]0xfdcf; Expected = $true }, @{ Id = [string][char]0xfdd0; Expected = $false },
        @{ Id = [string][char]0xfdf0; Expected = $true }, @{ Id = [string][char]0xfffd; Expected = $true },
        @{ Id = [string][char]0xfffe; Expected = $false }, @{ Id = ([char]0xd800 + [char]0xdc00); Expected = $true },
        @{ Id = ([char]0xdb7f + [char]0xdfff); Expected = $true }, @{ Id = ([char]0xdb80 + [char]0xdc00); Expected = $false },
        @{ Id = 'Обложка_1-2.jpg'; Expected = $true }, @{ Id = 'Àrbre-1'; Expected = $true },
        @{ Id = ([char]0x00d7 + 'symbol'); Expected = $false }, @{ Id = ('a' + [char]0x00d7); Expected = $false }
    )
    foreach ($case in $cases) {
        $id = [string]$case.Id
        $expected = if ($case.Expected) { '1' } else { '0' }
        $nativeResult = ([string](& $native $id)).Trim()
        if ($LASTEXITCODE -ne 0) { throw "Native binary-ID validator failed for case '$id'." }
        $jsResult = ([string](& cscript.exe //nologo $runner $id)).Trim()
        if ($LASTEXITCODE -ne 0) { throw "JScript binary-ID validator failed for case '$id'." }
        if ($nativeResult -notin @('0', '1') -or $jsResult -notin @('0', '1')) { throw "Invalid validator output for case '$id': native=$nativeResult, js=$jsResult" }
        $codePoints = if ($id.Length -eq 0) { '(empty)' } else { [string]::Join('-', ($id.ToCharArray() | ForEach-Object { '{0:X4}' -f [int][char]$_ })) }
        if ($nativeResult -ne $expected) { throw "C++ XML NCName result mismatch for U+${codePoints}: expected=$expected, actual=$nativeResult" }
        if ($jsResult -ne $expected) { throw "JScript XML NCName result mismatch for U+${codePoints}: expected=$expected, actual=$jsResult" }
    }} finally {
    Remove-Item -LiteralPath $runner -Force -ErrorAction SilentlyContinue
}
Write-Host "Binary-ID C++/JScript differential contract passed ($($cases.Count) boundary cases)."