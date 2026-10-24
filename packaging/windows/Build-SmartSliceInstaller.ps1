param(
  [Parameter(Mandatory=$true)][string]$ApplicationFolder,
  [string]$Version = '0.1.0-preview',
  [string]$ISCC = 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
)
$ErrorActionPreference='Stop'
$payload=(Resolve-Path $ApplicationFolder).Path
if(-not (Test-Path (Join-Path $payload 'orca-slicer.exe'))) {
  throw "La cartella deve contenere orca-slicer.exe e le dipendenze della build installata."
}
if(-not(Test-Path $ISCC)) {throw "Inno Setup 6 non trovato. Installa Inno Setup 6 o passa -ISCC."}
$spec=Join-Path $PSScriptRoot 'SmartSlice.iss'
& $ISCC "/DAppSourceDir=$payload" "/DAppVersion=$Version" $spec
if($LASTEXITCODE -ne 0){throw "Generazione installer fallita: $LASTEXITCODE"}
Write-Host "Pacchetto generato nella cartella dist. Da testare prima di distribuire."
