<# Verifies the persisted startup validation switch without changing script identity or command allocation. #>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
function Read([string]$path) { Get-Content -Raw -LiteralPath (Join-Path $root $path) }
function Require([string]$text, [string]$pattern, [string]$name) {
    if ($text -notmatch $pattern) { throw "Отсутствует контракт: $name" }
}

$defaults = Read 'src\fbe\settings\SettingsDefaults.cpp'
$settingsHeader = Read 'src\fbe\Settings.h'
$settings = Read 'src\fbe\Settings.cpp'
$serialization = Read 'src\fbe\settings\SettingsSerialization.cpp'
$advanced = Read 'src\fbe\settings\ui\SettingsAdvancedPage.cpp'
$frame = Read 'src\fbe\mainfrm.cpp'
$diagnostics = Read 'src\fbe\ScriptDiagnostics.h'
$runtime = Read 'runtime\main.js'
$catalog = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root 'localization\app-ui\fbe-small-dialogs.json') | ConvertFrom-Json

Require $defaults 'm_check_scripts_on_startup\s*=\s*true' 'default ON preserves existing startup validation'
Require $settingsHeader 'bool CheckScriptsOnStartup\(\) const' 'persisted setting getter'
Require $settingsHeader 'SetCheckScriptsOnStartup\(bool value, bool apply = false\)' 'persisted setting setter'
Require $serialization 'CheckScriptsOnStartup' 'serialized setting key'
Require $serialization 'GetStringedProperty\(&m_check_scripts_on_startup, KEY_BOOL\)' 'setting is saved as bool'
Require $serialization 'm_check_scripts_on_startup = StrToBool' 'setting is restored as bool'
Require $advanced 'IDC_CHECK_SCRIPTS_ON_STARTUP' 'advanced settings checkbox'
Require $advanced 'SetCheckScriptsOnStartup\(checkScriptsOnStartup, true\)' 'checkbox commits immediately to persistent settings'

Require $frame 'if\(!_Settings\.CheckScriptsOnStartup\(\)\) return true;' 'OFF performs discovery without parsing scripts'
Require $frame 'SUCCEEDED\(ScriptLoad\(path\)\) && ScriptFindFunc\(L"Run"\)' 'ON retains syntax and Run validation'
Require $frame 'ScopedDialogSuppression suppressDialogs' 'startup diagnostics suppress modal dialogs'
Require $diagnostics 'class ScopedDialogSuppression' 'scoped diagnostic suppression'
Require $diagnostics 'if \(DialogsSuppressed\(\)\)' 'suppressed startup errors stay diagnostic-only'
Require (Read 'src\fbe\script.cpp') 'if \(!FbeScriptDiagnostics::DialogsSuppressed\(\)\)' 'fallback parser error is also noninteractive at startup'
Require $runtime 'function apiRunCmd\(path\)' 'actual invocation remains the deferred validation path'

$entry = $catalog.strings.'fbe.dialog.idd_setting_other.check_scripts_on_startup'
if ($null -eq $entry -or $entry.resource -ne 'IDD_SETTINGS_ADVANCED' -or $entry.targetId -ne 'IDC_CHECK_SCRIPTS_ON_STARTUP') { throw 'Локализация checkbox не привязана к Advanced.' }
foreach ($language in @($catalog.targetLanguages)) { if ([string]::IsNullOrWhiteSpace($entry.translations.$language)) { throw "Нет перевода checkbox: $language" } }
$tooltip = $catalog.strings.'fbe.settings.tooltip.advanced.check_scripts_on_startup'
foreach ($language in @($catalog.targetLanguages)) { if ($null -eq $tooltip -or [string]::IsNullOrWhiteSpace($tooltip.translations.$language)) { throw "Нет перевода tooltip: $language" } }

Write-Host 'Startup script validation setting contract passed.'
