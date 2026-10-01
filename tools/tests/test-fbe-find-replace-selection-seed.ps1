<# Guards the consistent Design and Source policy for seeding a newly opened Find/Replace dialog. #>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$design = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\FBEview.cpp')
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\mainfrm.cpp')
$dialog = Get-Content -Raw -LiteralPath (Join-Path $root 'src\fbe\SearchReplace.h')
function Require([string]$text, [string]$pattern, [string]$description) { if($text -notmatch $pattern) { throw "Missing $description." } }
Require $design '(?s)m_fo\.pattern = !selection\.IsEmpty\(\).*?\? selection : CString\(\)' 'Design useful-selection policy that clears unsuitable input'
Require $design '(?s)const bool openingFind.*?if \(openingFind\) SeedSearchPatternFromSelection' 'Design Find only seeds a newly opened dialog'
Require $design '(?s)const bool openingReplace.*?if \(openingReplace\) SeedSearchPatternFromSelection' 'Design Replace only seeds a newly opened dialog'
Require $source 'static bool IsUsefulFindSelection' 'Source useful-selection helper'
foreach($dialogClass in @('CSciFindDlg', 'CSciReplaceDlg')) { Require $source ("class {0}[\s\S]*?IsUsefulFindSelection\(selection\) \? selection : CString\(\)" -f $dialogClass) "$dialogClass Source policy" }
if($source -match "while \(\*p && \*p!='\\r'") { throw 'Source selection must not silently truncate a multiline selection.' }
if($dialog -match 'first=str') { throw 'Search history must not auto-select its first item.' }
function ShouldSeed([string]$selection) { return -not [string]::IsNullOrEmpty($selection) -and $selection.Length -le 120 -and $selection.IndexOfAny([char[]]"`r`n") -lt 0 }
foreach($value in @('word', 'short phrase', ('x' * 120))) { if(-not (ShouldSeed $value)) { throw 'Useful selection must seed the query.' } }
foreach($value in @('', ('x' * 121), "first`r`nsecond")) { if(ShouldSeed $value) { throw 'Unsuitable selection must leave the query empty.' } }
Write-Host 'Find/Replace selection prefill policy passed.'