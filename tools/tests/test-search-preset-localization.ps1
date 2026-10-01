<# Ensures every built-in Search/Replace template and category can be rendered in every shipped UI language. #>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$catalog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json
$source = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'src\fbe\search\SearchPresetCatalog.cpp')
$ids = [regex]::Matches($source, 'PRESET\(([a-z0-9_]+),') | ForEach-Object { $_.Groups[1].Value } | Select-Object -Skip 1
$categories = @('whitespace','punctuation','typography','dashes_numbers','ocr','proofreading','names','xml_formatting','fb2_structure','links_notes','import_artifacts','diagnostics')
$keys = @('fbe.search_preset.review_only')
foreach($id in $ids) { $keys += "fbe.search_preset.$id.name", "fbe.search_preset.$id.description" }
foreach($category in $categories) { $keys += "fbe.search_preset.category.$category" }
foreach($key in $keys | Select-Object -Unique) {
    $entry = $catalog.strings.PSObject.Properties[$key]
    if($null -eq $entry) { throw "Missing preset localization key: $key" }
    foreach($language in $catalog.targetLanguages) {
        if([string]::IsNullOrWhiteSpace([string]$entry.Value.translations.$language)) { throw "Empty $language translation for $key" }
    }
}
Write-Host "Search preset localization contract passed ($($ids.Count) built-ins, $($keys.Count) strings)."