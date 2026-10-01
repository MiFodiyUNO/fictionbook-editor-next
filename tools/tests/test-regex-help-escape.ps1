[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexHelpDialog.cpp')
foreach ($token in @('COMMAND_ID_HANDLER(IDC_REGEX_HELP_CLOSE, OnClose)', 'COMMAND_ID_HANDLER(IDCANCEL, OnClose)', 'MESSAGE_HANDLER(WM_CLOSE, OnWindowClose)', 'LRESULT OnClose', 'LRESULT OnWindowClose', 'SaveSize(); EndDialog(IDC_REGEX_HELP_CLOSE);')) {
    if ($source.IndexOf($token, [System.StringComparison]::Ordinal) -lt 0) { throw "Regex Help close path misses $token." }
}
if ($source -match 'COMMAND_ID_HANDLER\(IDCANCEL, OnClose\)[\s\S]*?COMMAND_ID_HANDLER\(IDCANCEL, OnClose\)') { throw 'Duplicate Escape command handler.' }
Write-Host 'Regex Help Escape/close contract passed.'