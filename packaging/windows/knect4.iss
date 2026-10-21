; The Windows installer of a staged release (packaging/stage.py windows), compiled by packaging/windows/make_installer.ps1
; with Inno Setup 6:
;   ISCC /DKnect4Version=<MAJOR.MINOR.BUILD> /DStageDir=<dist> /DOutputDir=<out> packaging\windows\knect4.iss
; The game, its licence and the third-party notices, a Start-menu entry and an uninstaller. A per-user install by
; default (no administrator asked, under the user's Programs folder: Reckless Technology\Knect 4 there); the dialog
; offers a machine-wide one, under Program Files\Reckless Technology\Knect 4, beside the organisation's other products.
; The uninstaller takes away what the installer put there and nothing else: the person's settings in their own folder
; (reckless-technology\knect4 under %APPDATA%) are theirs, and the other products' folders under Reckless Technology,
; and the Start menu's Reckless Technology group they share, keep everything of theirs.
;
; The installer's and the uninstaller's icon is the game's own, assets\icon\icon.ico; the uninstaller's entry in
; Settings shows the installed executable's.
;
; /DTestRoot=<dir> makes a test build (packaging/windows/test_installer.ps1): every part under that folder, an id and a
; name of its own so it never meets a real install, and no previous folder reused.

#ifndef Knect4Version
  #error Pass the version: /DKnect4Version=MAJOR.MINOR.BUILD (packaging\windows\make_installer.ps1 does)
#endif
#ifndef StageDir
  #error Pass the staged release: /DStageDir=<dist>
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif
#define IconFile AddBackslash(SourcePath) + "..\..\assets\icon\icon.ico"
#define AppExe "Knect4.exe"
#ifdef TestRoot
  #define AppName "Knect 4 (installer test)"
#else
  #define AppName "Knect 4"
#endif

[Setup]
#ifdef TestRoot
AppId={{8151F7D0-5852-4C6B-9755-1C1E1579D46E}
DefaultDirName={#TestRoot}\Knect 4
UsePreviousAppDir=no
OutputBaseFilename=knect4-{#Knect4Version}-win64-setup-test
#else
; One id for every version, so an install replaces the last and one uninstaller serves.
AppId={{6131B80F-85A8-4C05-884C-707EE7A4F1C4}
DefaultDirName={autopf}\Reckless Technology\Knect 4
OutputBaseFilename=knect4-{#Knect4Version}-win64-setup
#endif
AppName={#AppName}
AppVersion={#Knect4Version}
AppVerName={#AppName} {#Knect4Version}
AppPublisher=Reckless Technology
AppPublisherURL=https://recklesstechnology.com
AppCopyright=Copyright (C) 2014-2026 Jonny Reckless
DefaultGroupName=Reckless Technology
DisableProgramGroupPage=yes
LicenseFile={#StageDir}\LICENSE.txt
OutputDir={#OutputDir}
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile={#IconFile}
VersionInfoVersion={#Knect4Version}
VersionInfoCompany=Reckless Technology
VersionInfoDescription={#AppName} installer
; Signed, where the script is told how (make_installer.ps1 -SignWithESigner): the installer and the uninstaller it
; writes, each through the SignTool of that name, so neither is flagged by SmartScreen as unsigned.
#ifdef SignToolName
SignTool={#SignToolName}
SignedUninstaller=yes
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#StageDir}\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
