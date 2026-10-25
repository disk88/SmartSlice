#define AppName "SmartSlice AI"
#ifndef AppVersion
  #define AppVersion "0.1.0-preview"
#endif
#ifndef AppSourceDir
  #error AppSourceDir must point to a compiled OrcaSlicer installation folder
#endif

[Setup]
AppId={{D364195B-6025-4B2D-99AA-BF0F775D8E31}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\SmartSliceAI
DefaultGroupName=SmartSlice AI
OutputDir=..\..\dist
OutputBaseFilename=SmartSliceAI-Windows-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
DisableProgramGroupPage=yes
UninstallDisplayName=SmartSlice AI (Preview)
LicenseFile=..\..\LICENSE.txt

[Files]
Source: "{#AppSourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "AI-Setup.ps1"; DestDir: "{app}\SmartSlice"; Flags: ignoreversion
Source: "Check-SmartSliceRequirements.ps1"; DestDir: "{app}\SmartSlice"; Flags: ignoreversion

[Tasks]
Name: "desktopicon"; Description: "Crea un collegamento sul desktop"; Flags: unchecked
Name: "aihelper"; Description: "Configura IA locale con Ollama dopo l'installazione (facoltativo)"; Flags: unchecked

[Icons]
Name: "{autoprograms}\SmartSlice AI"; Filename: "{app}\orca-slicer.exe"
Name: "{autodesktop}\SmartSlice AI"; Filename: "{app}\orca-slicer.exe"; Tasks: desktopicon

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\SmartSlice\AI-Setup.ps1"""; Description: "Configura Ollama AI (opzionale)"; Flags: postinstall skipifsilent shellexec; Tasks: aihelper
Filename: "{app}\orca-slicer.exe"; Description: "Avvia SmartSlice AI"; Flags: nowait postinstall skipifsilent

[Code]
function InitializeSetup(): Boolean;
begin
  Result := IsWin64;
  if not Result then
    MsgBox('SmartSlice AI richiede Windows a 64 bit.', mbError, MB_OK);
end;
