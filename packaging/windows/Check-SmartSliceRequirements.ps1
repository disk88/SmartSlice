param([switch]$Quiet)
$ErrorActionPreference = 'Stop'
function Info($s) { if (-not $Quiet) { Write-Host $s } }
$os = Get-CimInstance Win32_OperatingSystem
$ramGB = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1)
$freeGB = [math]::Round((Get-PSDrive -Name $env:SystemDrive.Substring(0,1)).Free / 1GB, 1)
$gpu = @(Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name)
$is64 = [Environment]::Is64BitOperatingSystem
$major = [Environment]::OSVersion.Version.Major
$ollama = Get-Command ollama -ErrorAction SilentlyContinue
$ollamaExe = if($ollama) { $ollama.Source } else { Join-Path $env:LOCALAPPDATA 'Programs\Ollama\ollama.exe' }
$hasOllama = Test-Path $ollamaExe
$api = $false
try { $null = Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/tags' -TimeoutSec 3; $api = $true } catch {}
$result = [ordered]@{
  Windows = $os.Caption; x64OS = $is64; RAM_GB = $ramGB
  FreeSpace_GB = $freeGB; GPU = $gpu
  BaseRequirementsOK = ($is64 -and $major -ge 10 -and $ramGB -ge 8 -and $freeGB -ge 5)
  LocalAIRecommended = ($is64 -and $major -ge 10 -and $ramGB -ge 16 -and $freeGB -ge 10)
  OllamaExecutableFound = $hasOllama; OllamaAPIResponds = $api
}
Info "SmartSlice AI — verifica requisiti"
foreach($key in $result.Keys) { Info ("{0}: {1}" -f $key, ($result[$key] -join ', ')) }
if (-not $Quiet) { Info "Il controllo non installa niente e non modifica il sistema." }
[pscustomobject]$result
