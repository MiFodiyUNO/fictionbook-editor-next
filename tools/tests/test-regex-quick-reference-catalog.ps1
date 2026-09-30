<# Guards the engine-specific quick-reference catalogue and insertion metadata. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\RegexQuickReference.h')
$strings = (Get-Content -Raw -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json).strings
foreach($required in @('enum class RegexQuickReferenceMode', 'RegexQuickReferenceEntry', 'InsertRegexQuickReference', 'SearchUiContext::Design', 'SearchUiContext::Source', 'RegexQuickReferenceMode::Replacement', 'L"\\p{L}"', 'L"\\p{N}"', 'L"\\xNN"', 'L"(?=...)"', 'L"(?<=...)"', 'L"{n,m}?"', 'L"{n,m}+"', 'L"(?-i)"', 'L"\\U"', 'L"\\S"', 'L"\\1"', 'L"\\B"')) {
    if($source -notmatch [regex]::Escape($required)) { throw "Missing quick-reference element: $required" }
}
foreach($forbidden in @('SearchUiContext::Source, RegexQuickReferenceMode::Search },\r?\n        { L"\\p{L}"', 'SearchUiContext::Source, RegexQuickReferenceMode::Replacement },\r?\n        { L"\\U"')) {
    if($source -match $forbidden) { throw 'Source catalogue must not advertise Design-only syntax.' }
}
if($source -notmatch 'result\.text = text\.Left\(start\) \+ entry\.insertionText \+ text\.Mid\(end\)') { throw 'Insertion must replace the current selection.' }
if($source -notmatch 'selectionStart = start \+ entry\.selectionStart') { throw 'Insertion must preserve metadata-driven selection.' }
foreach($key in [regex]::Matches($source, 'L"(fbe\.regex_quick\.[^"]+)"') | ForEach-Object { $_.Groups[1].Value } | Select-Object -Unique) {
    $entry = $strings.$key
    if($null -eq $entry -or [string]::IsNullOrWhiteSpace($entry.translations.'en-US') -or [string]::IsNullOrWhiteSpace($entry.translations.'ru-RU')) { throw "Quick-reference key is not localized: $key" }
}
foreach($forbidden in @('L"\\p{L}"', 'L"(?<=...)"', 'L"\\U"')) {
    $sourceLines = $source -split "`n" | Where-Object { $_ -match 'SearchUiContext::Source' }
    if(($sourceLines -join "`n") -match [regex]::Escape($forbidden)) { throw "Source catalogue advertises unsupported PCRE2 syntax: $forbidden" }
}
Write-Host 'Regex quick-reference catalogue contract passed.'
