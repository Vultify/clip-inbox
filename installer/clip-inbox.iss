; Clip Inbox installer for Windows.
; Built by build-installer.ps1, which passes in AppVersion, PluginDir, OutputDir, and OutputName.

#ifndef AppVersion
  #error AppVersion must be defined
#endif

[Setup]
AppId={{5B0E5C2A-6F1E-4C57-9A0B-1E3C9D2F7A41}
AppName=Clip Inbox
AppVersion={#AppVersion}
AppVerName=Clip Inbox {#AppVersion}
AppPublisher=Caleb Castro
AppPublisherURL=https://github.com/Vultify/clip-inbox
AppSupportURL=https://github.com/Vultify/clip-inbox/issues
AppUpdatesURL=https://github.com/Vultify/clip-inbox/releases
UninstallDisplayName=Clip Inbox for OBS Studio

; OBS loads plugins for every user from ProgramData, so that's the only place this installs to.
DefaultDirName={commonappdata}\obs-studio\plugins\clip-inbox
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

; OBS keeps the plugin loaded while it runs, so an update offers to close OBS first.
CloseApplications=yes
RestartApplications=no

OutputDir={#OutputDir}
OutputBaseFilename={#OutputName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
; OBS 33 and newer load the plugin from its own folder. OBS 31 and 32 only look in bin\64bit, and OBS 33 skips that
; copy once it has loaded the first one, so installing both covers every supported version.
Source: "{#PluginDir}\clip-inbox.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PluginDir}\bin\64bit\clip-inbox.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "{#PluginDir}\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs

[Messages]
FinishedLabel=Clip Inbox is installed.%n%nOpen OBS Studio, then turn on Clip Inbox from the Docks menu.

[Code]
// Installing without OBS works, but it's almost always a mistake, so ask first.
function InitializeSetup(): Boolean;
begin
  Result := True;
  if not RegKeyExists(HKLM, 'SOFTWARE\OBS Studio') then
    Result := MsgBox('OBS Studio doesn''t seem to be installed on this computer. Clip Inbox only works inside OBS Studio.' + #13#10#13#10 + 'Install anyway?', mbConfirmation, MB_YESNO) = IDYES;
end;
