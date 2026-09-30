<# Runs the startup-validation switch in an isolated portable profile. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$FbeExe, [ValidateRange(30, 180)][int]$TimeoutSeconds = 90)

$ErrorActionPreference = 'Stop'
$FbeExe = (Resolve-Path -LiteralPath $FbeExe).Path
$exeDirectory = Split-Path -Parent $FbeExe
$portableIni = Join-Path $exeDirectory 'portable.ini'
$hadIni = Test-Path -LiteralPath $portableIni
$oldIni = if($hadIni) { Get-Content -LiteralPath $portableIni -Raw } else { $null }
$dataDirectory = [IO.Path]::GetFullPath((Join-Path $exeDirectory 'ScriptStartupValidationRuntime'))
if((Split-Path -Parent $dataDirectory) -ne [IO.Path]::GetFullPath($exeDirectory)) { throw 'Unsafe script startup validation test directory.' }

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class FbeScriptValidationDialog {
    public delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr window, StringBuilder name, int length);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    public static bool CloseVisibleDialog(int targetProcessId) {
        bool found = false;
        EnumWindows((window, parameter) => {
            uint processId; GetWindowThreadProcessId(window, out processId);
            var className = new StringBuilder(32); GetClassName(window, className, className.Capacity);
            if(processId == (uint)targetProcessId && IsWindowVisible(window) && className.ToString() == "#32770") {
                found = true; PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
'@

function Invoke-Scenario([string]$Scenario, [switch]$ExpectDialog) {
    $savedMode = $env:FBE_NEXT_TEST_MODE; $savedScenario = $env:FBE_NEXT_TEST_SCENARIO
    try {
        $env:FBE_NEXT_TEST_MODE = '1'; $env:FBE_NEXT_TEST_SCENARIO = $Scenario
        $process = Start-Process -FilePath $FbeExe -WorkingDirectory $exeDirectory -ArgumentList '--portable' -PassThru
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds); $dialogSeen = $false
        do {
            if($ExpectDialog -and [FbeScriptValidationDialog]::CloseVisibleDialog($process.Id)) { $dialogSeen = $true }
            $process.Refresh(); Start-Sleep -Milliseconds 100
        } while(-not $process.HasExited -and (Get-Date) -lt $deadline)
        if(-not $process.HasExited) { Stop-Process -Id $process.Id -Force; throw "$Scenario timed out." }
        if($ExpectDialog -and -not $dialogSeen) { throw "$Scenario did not show the interactive script error dialog." }
        $report = Join-Path $dataDirectory 'Diagnostics\portable-state-report.txt'
        $text = if(Test-Path -LiteralPath $report) { Get-Content -LiteralPath $report -Raw } else { '' }
        if($process.ExitCode -ne 0 -or $text -notmatch "(?m)^phase=$Scenario$" -or $text -notmatch '(?m)^result=pass$') { throw "$Scenario failed:`n$text" }
        return $text
    } finally { $env:FBE_NEXT_TEST_MODE = $savedMode; $env:FBE_NEXT_TEST_SCENARIO = $savedScenario }
}

try {
    [IO.File]::WriteAllText($portableIni, "[Portable]`r`nDataPath=ScriptStartupValidationRuntime`r`n", [Text.UTF8Encoding]::new($false))
    if(Test-Path -LiteralPath $dataDirectory) { Remove-Item -LiteralPath $dataDirectory -Recurse -Force }
    $scripts = Join-Path $dataDirectory 'Scripts'; New-Item -ItemType Directory -Path $scripts -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $scripts 'broken.js'), 'function Run( {', [Text.UTF8Encoding]::new($false))
    $on = Invoke-Scenario 'script-startup-validation-on'
    if($on -notmatch '(?m)^validation-on=1$' -or $on -notmatch '(?m)^broken-in-catalog=0$') { throw "ON startup validation result is incorrect:`n$on" }
    $offWrite = Invoke-Scenario 'script-startup-validation-off-write'
    if($offWrite -notmatch '(?m)^validation-off=1$') { throw "OFF persistence write result is incorrect:`n$offWrite" }
    $offRead = Invoke-Scenario 'script-startup-validation-off-read' -ExpectDialog
    if($offRead -notmatch '(?m)^validation-off=1$' -or $offRead -notmatch '(?m)^broken-in-catalog=1$' -or $offRead -notmatch '(?m)^interactive-run=1$' -or $offRead -notmatch '(?m)^dialogs-suppressed=0$' -or $offRead -notmatch '(?m)^interactive-diagnostic=1$') { throw "OFF restart result is incorrect:`n$offRead" }
    Write-Host 'Startup script validation runtime smoke passed.'
} finally {
    if($hadIni) { [IO.File]::WriteAllText($portableIni, $oldIni, [Text.UTF8Encoding]::new($false)) } else { Remove-Item -LiteralPath $portableIni -Force -ErrorAction SilentlyContinue }
    if(Test-Path -LiteralPath $dataDirectory) { Remove-Item -LiteralPath $dataDirectory -Recurse -Force -ErrorAction SilentlyContinue }
}
