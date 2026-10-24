# SmartSlice AI v0.1 — Installer Windows (sorgenti)

## Stato

**Creati nel branch** `dev/smartslice-v0.1`: configurazione installer Inno Setup, controllo hardware, assistente opzionale Ollama e gestione AI locale nell'app. **Non esiste ancora un installer compilato e verificato**: serve prima una build Windows riuscita dell'app e la compilazione dello script Inno Setup. Nessun download/modello è incluso nel pacchetto.

## Flusso utente previsto

1. L'installer permette di installare lo slicer anche senza AI.
2. Casella non preselezionata: **Configura IA locale con Ollama dopo l'installazione**.
3. Se selezionata, dopo la copia dei file compare l'assistente: controlla architettura Windows, RAM, spazio sul disco di sistema, presenza Ollama e API `localhost:11434`. Il controllo non installa nulla.
4. Se Ollama risponde, SmartSlice lo riutilizza. Se è installato ma non avviato, l'utente lo avvia. Se manca, viene chiesta conferma specifica prima di usare WinGet (`Ollama.Ollama`); la procedura può essere rinviata.
5. Il download del modello `qwen3:4b` avviene **solo dall'interfaccia SmartSlice** tramite il pulsante e una seconda conferma.
6. Nel menu **SmartSlice AI → Test local AI...** puoi inserire endpoint locale, rilevare modelli preinstallati, scegliere il modello salvato e provare una domanda. Le impostazioni AI vengono salvate in `%APPDATA%\SmartSliceAI\ai.ini` (in una posizione dipendente dalla configurazione wxWidgets).
7. Se manca Ollama oppure il sistema non è adatto, lo slicer resta utilizzabile senza AI.

## Sorgenti

- `packaging/windows/SmartSlice.iss`: wizard Inno Setup.
- `packaging/windows/Check-SmartSliceRequirements.ps1`: controlli diagnostici non invasivi.
- `packaging/windows/AI-Setup.ps1`: configurazione facoltativa con conferme.
- `packaging/windows/Build-SmartSliceInstaller.ps1`: impacchetta una cartella Windows già compilata.
- `src/slic3r/GUI/SmartSliceAIDialog.cpp`: gestione di Ollama locale.

## Preparazione pacchetto Windows

1. Compila il fork con la toolchain Orca (Release x64) e genera la cartella di installazione completa, con `orca-slicer.exe` e tutte le dipendenze.
2. Installa **Inno Setup 6** sul PC di compilazione.
3. Da PowerShell nel repository:

```powershell
.\packaging\windows\Build-SmartSliceInstaller.ps1 -ApplicationFolder "C:\percorso\build\OrcaSlicer"
```

Lo script produce un EXE nella cartella `dist`, se la build e il packaging riescono.

**Attenzione:** il contenuto binario usa ancora il nome `orca-slicer.exe` e parte del branding/configurazione Orca; l'isolamento definitivo di risorse, aggiornamenti, identità e profili è una fase successiva. Non distribuire la preview come release ufficiale prima di testare la compatibilità, la disinstallazione e la coesistenza con Orca.

## Verifiche necessarie prima della distribuzione

- Compilazione Release x64 su Windows pulito.
- Smoke test Ollama: nessuna installazione, modello già presente, modello assente, download e chiusura dialog durante richiesta.
- Verifica WinGet, errori di rete e assenza di elevazione non richiesta.
- Verifica dei percorsi e della RAM sul disco che ospita realmente i modelli (il controllo corrente usa il disco di sistema e soglie indicative, **non garantisce** sufficienza per ogni modello).
- Verifica Windows 10 **22H2** o successivo e driver NVIDIA adeguati: il check attuale è preliminare, non completo.
- Verifica Inno Setup/AppId e cartelle di configurazione, eventuale conflitto con installazioni Orca, licenze AGPL-3.0 e file di terze parti.
- Firma del binario, se pubblicato; l'installer di sviluppo non è firmato.
- Conferma che il selettore modelli e le chiamate HTTP non abbiano regressioni o race condition.

## Compatibilità AI

La v0.1 rileva provider **Ollama compatibili sull'interfaccia localhost**. Non rileva automaticamente ogni AI già installata (LM Studio e provider con altri protocolli richiederanno adapter specifici). Il backend non modifica G-code, supporti o profili.
