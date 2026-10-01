[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$catalog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json
$dialog = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexHelpDialog.cpp')
$runtimeLocalization = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\RuntimeLocalization.cpp')
if ($runtimeLocalization -notmatch '\{ IDD_REGEX_HELP, IDC_REGEX_HELP_CLOSE, L"fbe\.dialog\.idd_regex_help\.close" \}') { throw 'Regex Help Close button is not bound to its resource ID.' }
if ($dialog -notmatch 'FbeApplyRuntimeDialogLocalization\(m_hWnd, IDD_REGEX_HELP\)') { throw 'Regex Help does not apply runtime dialog localization.' }
$expected = @{ 'en-US' = 'Close'; 'ru-RU' = 'Закрыть' }
foreach ($key in @('fbe.regex_help.design.advanced', 'fbe.regex_help.source.advanced')) {
    foreach ($language in $catalog.targetLanguages) {
        if ([string]::IsNullOrWhiteSpace([string]$catalog.strings.$key.translations.$language)) { throw "Missing $key translation for $language." }
    }
}
foreach ($syntax in @('\K', '\G', '(?<name>...)', '\k<name>', '(?|...)', '(?(1)yes|no)', '(?1)', '(?&name)', '(*SKIP)(*FAIL)')) {
    if ($catalog.strings.'fbe.regex_help.design.advanced'.translations.'en-US'.IndexOf($syntax, [System.StringComparison]::Ordinal) -lt 0) { throw "Design Help omits compile-tested PCRE2 syntax $syntax." }
}
if ($catalog.strings.'fbe.regex_help.source.advanced'.translations.'en-US' -notmatch 'SCFIND_REGEXP \| SCFIND_CXX11REGEX') { throw 'Source Help must name its confirmed Scintilla C++11 mode.' }
foreach ($property in $catalog.strings.psobject.Properties | Where-Object { $_.Name -like 'fbe.regex_help.*' }) {
    foreach ($language in $catalog.targetLanguages) {
        if (([string]$property.Value.translations.$language).Contains('\\r\\n')) { throw "Regex Help $($property.Name)/$language contains literal \\r\\n." }
    }
}
$source = $catalog.strings.'fbe.regex_help.source.text.detail'.translations.'en-US'
foreach ($section in @('Engine', 'Supported syntax', 'Classes', 'Anchors', 'Quantifiers', 'Groups and alternatives', 'Back-references', 'Replacement', 'Examples', 'Limitations')) {
    if ($source.IndexOf($section, [System.StringComparison]::Ordinal) -lt 0) { throw "Source Help omits section $section." }
}
$design = $catalog.strings.'fbe.regex_help.design.advanced'.translations.'en-US'
foreach ($section in @('Escaping and classes', 'Unicode and UCP', 'Anchors', 'Quantifiers', 'Groups, named groups and alternatives', 'Lookaround', 'Inline options', 'Advanced PCRE2', 'Replacement in FBE', 'Examples', 'Limitations')) {
    if ($design.IndexOf($section, [System.StringComparison]::Ordinal) -lt 0) { throw "Design Help omits section $section." }
}
if ($dialog -match 'detail \+= L"\\r\\n\\r\\n" \+ advanced') { throw 'Design Help still concatenates the duplicate advanced mini-manual.' }
foreach ($language in $expected.Keys) {
    if ($catalog.strings.'fbe.dialog.idd_regex_help.close'.translations.$language -ne $expected[$language]) { throw "Incorrect catalog close caption for $language." }
}
$output = Join-Path ([IO.Path]::GetTempPath()) ('fbe-regex-help-lang-' + $PID)
try {
    & (Join-Path $root 'tools\localization\export-runtime-lang.ps1') -RepositoryRoot $root -OutputDirectory $output -Clean | Out-Host
    foreach ($language in $expected.Keys) {
        $runtime = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $output $language 'fbe.json') | ConvertFrom-Json -AsHashtable
        if ($runtime.strings['fbe.dialog.idd_regex_help.close'] -ne $expected[$language]) { throw "Runtime close caption was not exported for $language." }
    }
}
finally { if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Recurse -Force } }
Write-Host 'Regex Help runtime localization contract passed.'