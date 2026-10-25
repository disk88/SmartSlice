$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName PresentationFramework
function Ask($message,$title='SmartSlice AI') {
  return [System.Windows.MessageBox]::Show($message,$title,'YesNo','Question') -eq 'Yes'
}
function Note($message) { [void][System.Windows.MessageBox]::Show($message,'SmartSlice AI','OK','Information') }
try {
  $check = & "$PSScriptRoot\Check-SmartSliceRequirements.ps1" -Quiet
  if (-not $check.BaseRequirementsOK) {
    Note "Il sistema non soddisfa i requisiti minimi preliminari per il modello locale (Windows 64-bit, almeno 8 GB RAM, almeno 5 GB disponibili). SmartSlice si può usare senza AI."
    return
  }
  if (-not $check.LocalAIRecommended) {
    if (-not (Ask "Sono consigliati almeno 16 GB RAM e 10 GB liberi per un modello locale. Vuoi comunque procedere?")) { return }
  }
  if ($check.OllamaAPIResponds) {
    Note "Ollama è già attivo. Apri SmartSlice AI > Test local AI per scegliere un modello o scaricarne uno."
    return
  }
  $exe = (Get-Command ollama -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source)
  if (-not $exe) { $candidate = Join-Path $env:LOCALAPPDATA "Programs\Ollama\ollama.exe"; if(Test-Path $candidate){$exe=$candidate} }
  if ($exe) {
    Note "Ollama è già installato, ma l'API locale non risponde. Avvialo dal menu Start, poi apri SmartSlice AI > Test local AI."
    return
  }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Note "WinGet non è disponibile. Scarica Ollama dal sito ufficiale https://ollama.com/download/windows, poi scegli il modello in SmartSlice."
    Start-Process 'https://ollama.com/download/windows'
    return
  }
  if (-not (Ask "Scaricare e installare Ollama da WinGet (pacchetto ufficiale Ollama.Ollama)? Il modello AI sarà scaricato separatamente dall'app, solo dopo tua conferma.")) { return }
  $arguments = @('install','--id','Ollama.Ollama','--exact','--source','winget','--accept-source-agreements','--accept-package-agreements')
  $p = Start-Process -FilePath (Get-Command winget).Source -ArgumentList $arguments -Wait -PassThru
  if ($p.ExitCode -ne 0) {
    Note "Installazione Ollama non completata (codice $($p.ExitCode)). SmartSlice resta installato. Puoi configurare l'AI in seguito."
    return
  }
  Note "Installazione Ollama completata. Apri Ollama e poi SmartSlice AI > Test local AI: potrai scaricare e selezionare un modello."
} catch {
  Note "Configurazione AI non completata: $($_.Exception.Message). SmartSlice continua a funzionare senza AI."
}
