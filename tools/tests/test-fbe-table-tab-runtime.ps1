<# .SYNOPSIS Exercises real Tab / Shift+Tab table navigation, including spans. #>
[CmdletBinding()]
param([string]$FbeExe = (Join-Path $PSScriptRoot '..\..\out\Release\FBE.exe'), [int]$TimeoutSeconds = 30, [string]$FixtureId)
$ErrorActionPreference='Stop'; $FbeExe=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($FbeExe)
if(-not (Test-Path -LiteralPath $FbeExe)){throw "Не найден FBE: $FbeExe"}
function Invoke-Fbe([string[]]$Arguments,[string]$Name) { $p=Start-Process -FilePath $FbeExe -ArgumentList $Arguments -PassThru; if(-not $p.WaitForExit($TimeoutSeconds*1000)){Stop-Process $p -Force;throw "FBE не завершил $Name"};if($p.ExitCode){throw "FBE вернул $($p.ExitCode): $Name"} }
$viewPath = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..\..')) 'src\fbe\FBEview.cpp'
$view = Get-Content -Raw -LiteralPath $viewPath
$route = 'if \(oe && oe->keyCode == VK_TAB && MoveTableCell\(oe->shiftKey == VARIANT_TRUE\)\)\s*\{\s*oe->cancelBubble = VARIANT_TRUE;\s*oe->returnValue = VARIANT_FALSE;\s*return VARIANT_FALSE;\s*\}'
if ($view -notmatch $route) { throw 'OnKeyDown no longer routes VK_TAB / Shift+Tab through successful MoveTableCell navigation.' }
if ($view -notmatch 'return VARIANT_TRUE;\s*\}') { throw 'OnKeyDown no longer leaves unsuccessful Tab navigation to MSHTML.' }
$dir=Join-Path ([IO.Path]::GetTempPath()) ('fbe-table-tab-'+[guid]::NewGuid().ToString('N')); New-Item -ItemType Directory -Path $dir | Out-Null
try {
 $cases=@(
  @{id='plain'; table='<tr><td>A</td><td>B</td></tr><tr><td>C</td><td>D</td></tr>'; rows=2},
  @{id='colspan'; table='<tr><td colspan="2">A</td><td>B</td></tr><tr><td>C</td><td>D</td><td>E</td></tr>'; rows=2},
  @{id='rowspan'; table='<tr><td rowspan="2">A</td><td>B</td><td>C</td></tr><tr><td>D</td><td>E</td></tr>'; rows=2},
  @{id='combined'; table='<tr><th colspan="2">A</th><td>B</td></tr><tr><td rowspan="2">C</td><td>D</td><td>E</td></tr><tr><td>F</td><td>G</td></tr>'; rows=3}
 )
 if($FixtureId) { $cases=@($cases | Where-Object id -eq $FixtureId); if($cases.Count -ne 1){throw "Не найдена tab fixture: $FixtureId"} }
 foreach($case in $cases) {
  $fb2=Join-Path $dir ($case.id+'.fb2');$report=Join-Path $dir ($case.id+'.tsv')
  $text='<?xml version="1.0" encoding="utf-8"?><FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0"><description><title-info><genre>prose</genre><author><first-name>T</first-name><last-name>T</last-name></author><book-title>tab</book-title><lang>en</lang></title-info><document-info><program-used>test</program-used><id>tab-'+$case.id+'</id><version>1.0</version></document-info></description><body><section><table>'+$case.table+'</table></section></body></FictionBook>'
  Set-Content -LiteralPath $fb2 -Value $text -Encoding utf8
  $oldMode=$env:FBE_NEXT_TEST_MODE;$oldScenario=$env:FBE_NEXT_TEST_SCENARIO
  try{$env:FBE_NEXT_TEST_MODE='1';$env:FBE_NEXT_TEST_SCENARIO='table-tab';Invoke-Fbe @('-b',$report,$fb2) ('tab '+$case.id)}finally{if($null -eq $oldMode){Remove-Item Env:FBE_NEXT_TEST_MODE -ErrorAction SilentlyContinue}else{$env:FBE_NEXT_TEST_MODE=$oldMode};if($null -eq $oldScenario){Remove-Item Env:FBE_NEXT_TEST_SCENARIO -ErrorAction SilentlyContinue}else{$env:FBE_NEXT_TEST_SCENARIO=$oldScenario}}
  $rows=Import-Csv -LiteralPath $report -Delimiter "`t"; $forward=@($rows|? direction -eq 'forward');$reverse=@($rows|? direction -eq 'reverse')
  if($forward.Count-ne 1 -or $forward.ok -ne '1'){throw "Tab failed: $($case.id)"};if($reverse.Count-ne 1 -or $reverse.ok -ne '1'){throw "Shift+Tab failed: $($case.id)"}
  if($forward.before -eq $forward.after){throw "Tab from final cell did not add a row: $($case.id)"};if($forward.after -notmatch ('rows=' + ($case.rows+1) + '[;,]')){throw "Tab did not add exactly one row: $($case.id)"}
  if($forward.position -notmatch ('^' + $case.rows + ',0$')){throw "Tab did not select the first logical cell of new row: $($case.id) / $($forward.position)"}
  if($reverse.before -ne $reverse.after){throw "Shift+Tab from first cell changed table structure: $($case.id)"};if($reverse.position -ne '0,0'){throw "Shift+Tab did not remain at first cell: $($case.id) / $($reverse.position)"}
 }
 Write-Host 'Production Tab / Shift+Tab table navigation passed.'
} finally { Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue }