[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$catalog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json
$dialog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'src\fbe\search\ui\RegexHelpDialog.cpp')
$runtimeLocalization = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'src\fbe\RuntimeLocalization.cpp')
if ($runtimeLocalization -notmatch '\{ IDD_REGEX_HELP, IDC_REGEX_HELP_CLOSE, L"fbe\.dialog\.idd_regex_help\.close" \}') { throw 'Regex Help Close button is not bound to its resource ID.' }
if ($dialog -notmatch 'FbeApplyRuntimeDialogLocalization\(m_hWnd, IDD_REGEX_HELP\)') { throw 'Regex Help does not apply runtime dialog localization.' }
foreach ($key in @('fbe.regex_help.design.caption', 'fbe.regex_help.source.caption',
    'fbe.regex_help.heading.engine', 'fbe.regex_help.heading.unicode_ucp', 'fbe.regex_help.heading.characters',
    'fbe.regex_help.heading.classes', 'fbe.regex_help.heading.anchors', 'fbe.regex_help.heading.quantifiers',
    'fbe.regex_help.heading.groups', 'fbe.regex_help.heading.lookaround', 'fbe.regex_help.heading.advanced',
    'fbe.regex_help.heading.syntax', 'fbe.regex_help.heading.replacement', 'fbe.regex_help.heading.examples',
    'fbe.regex_help.heading.limitations', 'fbe.regex_help.example.source_digits', 'fbe.regex_help.example.source_spaces',
    'fbe.regex_help.example.source_capture', 'fbe.regex_help.example.source_empty_paragraph', 'fbe.regex_help.example.source_external_link',
    'fbe.regex_help.example.source_undefined_reference', 'fbe.regex_help.example.source_note_link', 'fbe.regex_help.example.source_html_artifact',
    'fbe.regex_help.example.design_spaces', 'fbe.regex_help.example.design_punctuation',
    'fbe.regex_help.example.design_mixed_alphabet', 'fbe.regex_help.example.design_repeated_word',
    'fbe.regex_help.example.design_numeric_range', 'fbe.regex_help.example.design_initials')) {    foreach ($language in $catalog.targetLanguages) {
        if ([string]::IsNullOrWhiteSpace([string]$catalog.strings.$key.translations.$language)) { throw "Missing $key translation for $language." }
    }
    if ($key -like 'fbe.regex_help.heading.*' -and $dialog.IndexOf($key, [System.StringComparison]::Ordinal) -lt 0) { throw "Localized heading $key is not used by Regex Help." }
}
$activeHelpKeys = [System.Collections.Generic.HashSet[string]]::new()
foreach ($match in [regex]::Matches($dialog, 'L"(fbe\.regex_help\.(?:heading|body|example)\.[^"]*)"')) {
    [void] $activeHelpKeys.Add($match.Groups[1].Value)
}
foreach ($key in $activeHelpKeys) {
    foreach ($language in $catalog.targetLanguages) {
        if ([string]::IsNullOrWhiteSpace([string]$catalog.strings.$key.translations.$language)) { throw "Missing active Help translation $key for $language." }
    }
}foreach ($token in @('struct HelpBlock', 'BuildHelpBlocks', 'AddQuickReferenceSyntax', 'Scintilla regular expressions in its documented C++11 mode. This is not PCRE2.', 'No UCP, Unicode property classes, lookbehind, \\K, \\G, branch reset, PCRE2 verbs', 'Unicode and UCP', 'Lookaround and inline options', 'Advanced PCRE2', 'Replacement in FBE')) {
    if ($dialog.IndexOf($token, [System.StringComparison]::Ordinal) -lt 0) { throw "Regex Help structure misses $token." }
}
foreach ($key in @('fbe.regex_help.example.source_empty_paragraph', 'fbe.regex_help.example.source_external_link', 'fbe.regex_help.example.source_undefined_reference', 'fbe.regex_help.example.source_note_link', 'fbe.regex_help.example.source_html_artifact', 'fbe.regex_help.example.design_mixed_alphabet', 'fbe.regex_help.example.design_repeated_word', 'fbe.regex_help.example.design_numeric_range', 'fbe.regex_help.example.design_initials')) {
    if ($dialog.IndexOf($key, [System.StringComparison]::Ordinal) -lt 0) { throw "Regex Help does not render practical example $key." }
}if ($dialog -match 'fbe\.regex_help\.example\.design_word') { throw 'Regex Help still contains the duplicate repeated-word example.' }
if ($dialog -match 'ClassifyHelpLine|section ==') { throw 'Regex Help still derives formatting from paragraph position.' }
$limitationsKey = 'fbe.regex_help.body.source.limitations'
if ($dialog.IndexOf($limitationsKey, [System.StringComparison]::Ordinal) -lt 0) { throw 'Regex Help does not render the Source line-boundary limitations.' }
foreach ($language in $catalog.targetLanguages) {
    $text = [string]$catalog.strings.$limitationsKey.translations.$language
    if ([string]::IsNullOrWhiteSpace($text)) { throw "Missing Source line-boundary limitation for $language." }
    if ($text.IndexOf('MatchOnLines', [System.StringComparison]::Ordinal) -lt 0) { throw "Source limitation for $language does not mention MatchOnLines." }
}
$englishLimitations = [string]$catalog.strings.$limitationsKey.translations.'en-US'
foreach ($token in @('line by line', 'across line boundaries', 'MatchOnLines')) {
    if ($englishLimitations.IndexOf($token, [System.StringComparison]::Ordinal) -lt 0) { throw "English Source limitation misses: $token" }
}
$expected = @{ 'en-US' = 'Close'; 'ru-RU' = 'Закрыть' }
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