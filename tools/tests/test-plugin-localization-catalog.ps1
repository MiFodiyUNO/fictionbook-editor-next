<#
Проверяет Weblate-friendly каталог пользовательских строк плагинов.
Скрипт валидирует JSON, обязательные языки и наличие непустого перевода для
каждого ключа, чтобы каталог можно было безопасно использовать как основу для
последующей генерации Win32 resource-фрагментов.
#>
[CmdletBinding()]
param(
    [string]$CatalogPath
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if (-not $CatalogPath) {
    $CatalogPath = Join-Path $repoRoot "localization\plugin-ui\catalog.json"
}

if (-not (Test-Path -LiteralPath $CatalogPath)) {
    throw "Каталог локализации плагинов не найден: $CatalogPath"
}

$catalog = Get-Content -Raw -LiteralPath $CatalogPath | ConvertFrom-Json
if ($catalog.formatVersion -ne 1) {
    throw "Неожиданная версия формата каталога: $($catalog.formatVersion)"
}

$requiredLanguages = @(
    "en-US",
    "ru-RU",
    "uk-UA",
    "de-DE",
    "fr-FR",
    "es-ES",
    "it-IT",
    "pl-PL",
    "pt-PT",
    "nl-NL",
    "cs-CZ",
    "bg-BG"
)

$declaredLanguages = @($catalog.targetLanguages)
foreach ($language in $requiredLanguages) {
    if ($declaredLanguages -notcontains $language) {
        throw "В targetLanguages отсутствует обязательный язык: $language"
    }
}

if ((@($declaredLanguages | Sort-Object -Unique)).Count -ne $declaredLanguages.Count) {
    throw "В targetLanguages есть дублирующиеся языки."
}

$stringProperties = @($catalog.strings.PSObject.Properties)
if ($stringProperties.Count -eq 0) {
    throw "Каталог не содержит ни одной строки."
}

$seenKeys = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$translationIssues = [Collections.Generic.List[object]]::new()
foreach ($property in $stringProperties) {
    $key = [string]$property.Name
    if (-not $seenKeys.Add($key)) {
        throw "Дублирующийся ключ локализации: $key"
    }

    if ($key -notmatch '^[a-z0-9_]+(\.[a-z0-9_]+)+$') {
        throw "Ключ локализации имеет нестабильный формат: $key"
    }

    $entry = $property.Value
    if ([string]::IsNullOrWhiteSpace([string]$entry.source)) {
        throw "У ключа $key отсутствует source-строка."
    }

    foreach ($language in $requiredLanguages) {
        $translationProperty = if ($entry.translations) { $entry.translations.PSObject.Properties[$language] } else { $null }
        if (-not $translationProperty) {
            $translationIssues.Add([pscustomobject]@{
                    Key      = $key
                    Language = $language
                    State    = 'отсутствует'
                })
            continue
        }
        if ([string]::IsNullOrWhiteSpace([string]$translationProperty.Value)) {
            $translationIssues.Add([pscustomobject]@{
                    Key      = $key
                    Language = $language
                    State    = 'пустой'
                })
        }
    }
}

if ($translationIssues.Count -gt 0) {
    $issuesByKey = @($translationIssues | Group-Object Key | Sort-Object Name)
    Write-Host 'Каталог локализации плагинов не прошёл проверку.'
    Write-Host "  Проблемных ключей: $($issuesByKey.Count)"
    Write-Host "  Отсутствующих/пустых переводов: $($translationIssues.Count)"
    Write-Host '  По языкам:'
    foreach ($languageIssues in @($translationIssues | Group-Object Language | Sort-Object Name)) {
        Write-Host "    $($languageIssues.Name): $($languageIssues.Count)"
    }
    Write-Host '  Ключи с отсутствующими/пустыми переводами:'
    foreach ($keyIssues in $issuesByKey) {
        $languages = @($keyIssues.Group | Sort-Object Language | ForEach-Object { "$($_.Language) ($($_.State))" })
        Write-Host "    $($keyIssues.Name): $($languages -join ', ')"
    }
    exit 1
}

Write-Host "Каталог локализации плагинов прошёл проверку."
Write-Host "  Файл: $CatalogPath"
Write-Host "  Ключей: $($stringProperties.Count)"
Write-Host "  Языков: $($requiredLanguages.Count)"
