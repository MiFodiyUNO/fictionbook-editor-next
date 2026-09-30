<#
.SYNOPSIS
Проверяет generated-строки ExportHTML.

.DESCRIPTION
Скрипт страхует переход ExportHTML на JSON→generated `.rc2`: проверяет, что
`ExportHTML.rc` подключает generated-файл, ручная `STRINGTABLE` с runtime-
строками не вернулась, а `ExportHTMLStrings.generated.rc2` синхронизирован с
`localization/plugin-ui/catalog.json` и содержит строки runtime/tooltip.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$rcPath = Join-Path $repoRoot "src\export-html\ExportHTML.rc"
$generatedRcPath = Join-Path $repoRoot "src\export-html\ExportHTMLStrings.generated.rc2"
$dialogPath = Join-Path $repoRoot "src\export-html\HtmlExportOptionsDialog.h"

$pluginCatalog = Get-Content -Raw -Encoding UTF8 (Join-Path $repoRoot 'localization\plugin-ui\catalog.json') | ConvertFrom-Json
$appCatalog = Get-Content -Raw -Encoding UTF8 (Join-Path $repoRoot 'localization\app-ui\catalog.json') | ConvertFrom-Json
if ((@($pluginCatalog.targetLanguages) -join '|') -ne (@($appCatalog.targetLanguages) -join '|')) { throw 'plugin-ui.targetLanguages must match app-ui.targetLanguages.' }
$newExportHtmlResourceIds = @('IDS_SAVE_BUTTON', 'IDS_OPTIONS_CSS_CLEAR', 'IDS_OPTIONS_IMAGES_FOLDER_NAME', 'IDS_OPTIONS_WARNING_MIB', 'IDS_TOOLTIP_INCLUDE_TOC', 'IDS_TOOLTIP_STYLE', 'IDS_TOOLTIP_FONT', 'IDS_TOOLTIP_FONT_SIZE', 'IDS_TOOLTIP_LINE_HEIGHT', 'IDS_TOOLTIP_CONTENT_WIDTH', 'IDS_TOOLTIP_MARGINS', 'IDS_TOOLTIP_TEXT_ALIGNMENT', 'IDS_TOOLTIP_HEADING_ALIGNMENT', 'IDS_TOOLTIP_CSS_CLEAR', 'IDS_TOOLTIP_COVER_MODE', 'IDS_TOOLTIP_IMAGES_FOLDER', 'IDS_TOOLTIP_IMAGES_FOLDER_NAME', 'IDS_TOOLTIP_WARNING_MIB', 'IDS_TOOLTIP_NOTE_PLACEMENT', 'IDS_TOOLTIP_METADATA', 'IDS_TOOLTIP_METADATA_CHILD', 'IDS_OPTIONS_DOCUMENT_STRUCTURE', 'IDS_OPTIONS_VALUE_SINGLE_HTML', 'IDS_OPTIONS_VALUE_SPLIT_SECTIONS', 'IDS_TOOLTIP_DOCUMENT_STRUCTURE')
$identicalTechnicalTermAllowlist = @{
    # Only resource IDs and locales explicitly listed here may intentionally match English.
}
$expectedGeneratedLanguageCount = 12
if ($pluginCatalog.targetLanguages.Count -ne $expectedGeneratedLanguageCount) { throw "ExportHTML must declare exactly $expectedGeneratedLanguageCount target languages." }
foreach ($resourceId in $newExportHtmlResourceIds) {
    $entries = @($pluginCatalog.strings.PSObject.Properties | Where-Object { $_.Value.resourceId -eq $resourceId -and $_.Value.component -like 'export-html.*' })
    if ($entries.Count -ne 1) { throw "ExportHTML catalog entry is missing or duplicated: $resourceId" }
    $entry = $entries[0].Value
    $source = [string]$entry.source
    $englishProperty = $entry.translations.PSObject.Properties['en-US']
    if ($null -eq $englishProperty -or [string]::IsNullOrWhiteSpace([string]$englishProperty.Value)) { throw "ExportHTML English translation is missing or empty: $resourceId" }
    $english = [string]$englishProperty.Value
    foreach ($language in $pluginCatalog.targetLanguages) {
        $translationProperty = $entry.translations.PSObject.Properties[$language]
        if ($null -eq $translationProperty) { throw "ExportHTML translation is missing: $resourceId / $language" }
        $translation = [string]$translationProperty.Value
        if ([string]::IsNullOrWhiteSpace($translation)) { throw "ExportHTML translation is empty: $resourceId / $language" }
        if ($language -eq 'en-US') { continue }
        $allowIdenticalTechnicalTerm = $identicalTechnicalTermAllowlist.ContainsKey($resourceId) -and $identicalTechnicalTermAllowlist[$resourceId] -contains $language
        if (-not $allowIdenticalTechnicalTerm -and ($translation -eq $source -or $translation -eq $english)) {
            throw "English fallback is forbidden for ExportHTML UI/prose: $resourceId / $language"
        }
    }
}
if ((Get-Content -Raw (Join-Path $repoRoot 'src\export-html\resource.h')) -match 'IDS_TOOLTIP_OPTION_VALUE') { throw 'Unused IDS_TOOLTIP_OPTION_VALUE remained in resource.h.' }
if ((Get-Content -Raw (Join-Path $repoRoot 'src\export-html\RuntimeLocalization.cpp')) -match 'IDS_TOOLTIP_OPTION_VALUE') { throw 'Unused IDS_TOOLTIP_OPTION_VALUE remained in runtime bindings.' }
$rc = Get-Content -Raw -LiteralPath $rcPath
if (-not (Test-Path -LiteralPath $generatedRcPath)) {
    throw "Сгенерированный файл строк ExportHTML не найден: $generatedRcPath"
}
$generatedRc = Get-Content -Raw -LiteralPath $generatedRcPath
if ($pluginCatalog.strings.PSObject.Properties.Name -contains 'export_html.tooltip.option_value') { throw 'Unused generic tooltip catalog entry remained.' }
if ($generatedRc -match 'Configure this HTML export setting') { throw 'Template tooltip prose remained in generated resources.' }
if ($generatedRc -match 'IDS_TOOLTIP_OPTION_VALUE') { throw 'Unused IDS_TOOLTIP_OPTION_VALUE remained in generated resources.' }
foreach ($resourceId in $newExportHtmlResourceIds) {
    $occurrences = [regex]::Matches($generatedRc, "\b$([regex]::Escape($resourceId))\b").Count
    if ($occurrences -ne $expectedGeneratedLanguageCount) {
        throw "ExportHTML generated resource must occur exactly $expectedGeneratedLanguageCount times: $resourceId (actual: $occurrences)"
    }
}
$dialog = Get-Content -Raw -LiteralPath $dialogPath
$generalPage = Get-Content -Raw -LiteralPath (Join-Path $repoRoot "src\export-html\HtmlExportGeneralPage.cpp")
$appearancePage = Get-Content -Raw -LiteralPath (Join-Path $repoRoot "src\export-html\HtmlExportAppearancePage.cpp")
$localizedSources = $dialog + $generalPage + $appearancePage

if ($rc -notmatch '#include\s+"ExportHTMLStrings\.generated\.rc2"') {
    throw "ExportHTML.rc не подключает ExportHTMLStrings.generated.rc2."
}

if ($rc -match 'IDS_SAVE_FILE_FILTER\s+"') {
    throw "В ExportHTML.rc вернулась ручная STRINGTABLE-строка; runtime-строки должны генерироваться из JSON."
}

foreach ($controlId in @("IDC_TEMPLATE_LABEL", "IDC_TOC_DEPTH_LABEL")) {
    if ($rc -notmatch ("LTEXT\s+[^\r\n]*" + [regex]::Escape($controlId))) {
        throw "ExportHTML.rc должен использовать отдельный control ID $controlId вместо IDC_STATIC."
    }
}

foreach ($expectedCall in @(
    "SetDlgItemText(IDC_TEMPLATE_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_TEMPLATE_LABEL))",
    "SetDlgItemText(IDC_TOC_DEPTH_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_TOC_DEPTH))",
	"SetDlgItemText(IDC_CUSTOM_CSS_LABEL, LoadExportHtmlString(IDS_CUSTOM_SAVE_CUSTOM_CSS))",
    "LoadExportHtmlString(IDS_OPEN_TEMPLATE_FILTER)",
    "LoadExportHtmlString(IDS_OPEN_CSS_FILTER)"
)) {
    if ($localizedSources -notmatch [regex]::Escape($expectedCall)) {
        throw "HTML options UI не применяет локализованную строку: $expectedCall"
    }
}

$requiredResourceIds = @(
    "IDR_EXPORTHTML",
    "IDS_SAVE_BUTTON",
    "IDS_ERROR_OPEN_FILE",
    "IDS_ERROR_CREATE_DIRECTORY",
    "IDS_ERROR_WRITE_FILE",
    "IDS_ERROR_WRITE_FILE2",
	"IDS_ERROR_EMBEDDED_IMAGES_TEMPLATE",
	"IDS_WARNING_EXTERNAL_RESOURCES",
    "IDS_WARNING_FILE_ALREADY_EXISTS",
    "IDS_SAVE_FILE_FILTER",
    "IDS_XML_PARSE_ERROR",
    "IDS_AT_LINE_COLUMN",
    "IDS_AT_S_S",
    "IDS_ERROR",
    "IDS_COM_ERROR",
    "IDS_TOOLTIP_TEMPLATE",
    "IDS_TOOLTIP_BROWSE_TEMPLATE",
    "IDS_TOOLTIP_DOCINFO",
    "IDS_TOOLTIP_TOC_DEPTH",
    "IDS_TOOLTIP_CUSTOM_CSS",
    "IDS_TOOLTIP_BROWSE_CSS",
    "IDS_TOOLTIP_IMAGE_MAX_WIDTH",
    "IDS_TOOLTIP_IMAGE_MAX_HEIGHT",
    "IDS_CUSTOM_SAVE_TEMPLATE_LABEL",
    "IDS_CUSTOM_SAVE_INCLUDE_DESC",
    "IDS_CUSTOM_SAVE_TOC_DEPTH",
    "IDS_OPEN_TEMPLATE_FILTER",
	"IDS_OPEN_CSS_FILTER",
    "IDS_UNKNOWN_ERROR",
	"IDS_CUSTOM_SAVE_CUSTOM_CSS",
	"IDS_CUSTOM_SAVE_IMAGE_MAX_WIDTH",
	"IDS_CUSTOM_SAVE_IMAGE_MAX_HEIGHT",
    "IDS_OPTIONS_IMAGES_FOLDER_NAME",
    "IDS_OPTIONS_WARNING_MIB",
    "IDS_TOOLTIP_INCLUDE_TOC",
    "IDS_TOOLTIP_STYLE",
    "IDS_TOOLTIP_FONT",
    "IDS_TOOLTIP_FONT_SIZE",
    "IDS_TOOLTIP_LINE_HEIGHT",
    "IDS_TOOLTIP_CONTENT_WIDTH",
    "IDS_TOOLTIP_MARGINS",
    "IDS_TOOLTIP_TEXT_ALIGNMENT",
    "IDS_TOOLTIP_HEADING_ALIGNMENT",
    "IDS_TOOLTIP_CSS_CLEAR",
    "IDS_TOOLTIP_COVER_MODE",
    "IDS_TOOLTIP_IMAGES_FOLDER",
    "IDS_TOOLTIP_IMAGES_FOLDER_NAME",
    "IDS_TOOLTIP_WARNING_MIB",
    "IDS_TOOLTIP_NOTE_PLACEMENT",
    "IDS_TOOLTIP_METADATA",
    "IDS_TOOLTIP_METADATA_CHILD"
)


$requiredLanguageBlocks = @(
    "LANG_RUSSIAN",
    "LANG_UKRAINIAN",
    "LANG_GERMAN",
    "LANG_FRENCH",
    "LANG_SPANISH",
    "LANG_ITALIAN",
    "LANG_POLISH",
    "LANG_CZECH",
    "LANG_BULGARIAN",
    "LANG_PORTUGUESE",
    "LANG_DUTCH",
    "LANG_ENGLISH"
)

foreach ($language in $requiredLanguageBlocks) {
    $languagePattern = "LANGUAGE\s+$([regex]::Escape($language))\b"
    $languageMatch = [regex]::Match($generatedRc, $languagePattern)
    if (-not $languageMatch.Success) {
        throw "В ExportHTMLStrings.generated.rc2 отсутствует языковой блок: $language"
    }

    $nextLanguageMatch = [regex]::Match($generatedRc.Substring($languageMatch.Index + $languageMatch.Length), "LANGUAGE\s+LANG_[A-Z_]+")
    if ($nextLanguageMatch.Success) {
        $block = $generatedRc.Substring($languageMatch.Index, $languageMatch.Length + $nextLanguageMatch.Index)
    } else {
        $block = $generatedRc.Substring($languageMatch.Index)
    }

    foreach ($id in $requiredResourceIds) {
        if ($block -notmatch "\b$([regex]::Escape($id))\b") {
            throw "В языковом блоке $language отсутствует обязательная строка ExportHTML: $id"
        }
    }
}

$tempDirectory = Join-Path ([IO.Path]::GetTempPath()) "fbe-export-html-generated-strings-$PID"
try {
    New-Item -ItemType Directory -Force -Path $tempDirectory | Out-Null
    $tempGeneratedPath = Join-Path $tempDirectory "ExportHTMLStrings.generated.rc2"
    & (Join-Path $repoRoot "tools\localization\update-export-html-resource-strings.ps1") -OutputPath $tempGeneratedPath | Out-Host

    $expected = [IO.File]::ReadAllBytes($tempGeneratedPath)
    $actual = [IO.File]::ReadAllBytes($generatedRcPath)
    if ($expected.Length -ne $actual.Length) {
        throw "ExportHTMLStrings.generated.rc2 не синхронизирован с localization/plugin-ui/catalog.json."
    }
    for ($i = 0; $i -lt $expected.Length; $i++) {
        if ($expected[$i] -ne $actual[$i]) {
            throw "ExportHTMLStrings.generated.rc2 не синхронизирован с localization/plugin-ui/catalog.json."
        }
    }
}
finally {
    Remove-Item -LiteralPath $tempDirectory -Recurse -Force -ErrorAction SilentlyContinue
}

    if ($generatedRc -cmatch (([char]0xFFFD) + '|Ð.|Ñ.|Ã.|Â.')) {
    throw "В ExportHTMLStrings.generated.rc2 обнаружены признаки mojibake."
}

Write-Host "Проверка локализации runtime/tooltip-строк ExportHTML прошла успешно."
Write-Host "  Языковых блоков: $($requiredLanguageBlocks.Count)"
Write-Host "  Строк на язык: $($requiredResourceIds.Count)"
