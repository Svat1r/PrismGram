; PrismGram installer for Windows x64 (Inno Setup 6).
; Based on Telegram/build/setup.iss from Telegram Desktop / AyuGram.
;
; Build:  ISCC.exe /DMyAppVersion=0.1.0 /DReleasePath=C:\TBuild\tdesktop\out\Release nexo_setup.iss
;
; Differences from the upstream script:
; - own AppId, names and exe;
; - the program goes to %LOCALAPPDATA%\Programs\PrismGram, user data stays in
;   %APPDATA%\PrismGram, so reinstalling or updating never touches it;
; - uninstalling asks before deleting user data (account login, settings,
;   plugins) and keeps it by default.

#define MyAppShortName "PrismGram"
#define MyAppName "PrismGram"
#define MyAppPublisher "PrismGram"
#define MyAppExeName "PrismGram.exe"
; Must match AppId in Telegram/SourceFiles/core/version.h.
#define MyAppId "B72EFDDC-98AA-48C4-A7A6-E8BEFBE62C02"
#define CurrentYear GetDateTimeString('yyyy','','')

#ifndef MyAppVersion
  #define MyAppVersion "0.2.1"
#endif
#ifndef ReleasePath
  #define ReleasePath "..\..\out\Release"
#endif

[Setup]
AppId={{{#MyAppId}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppCopyright={#MyAppPublisher} {#CurrentYear}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir={#ReleasePath}\..\installer
OutputBaseFilename=PrismGram-Setup-x64-{#MyAppVersion}
SetupIconFile={#SourcePath}..\Resources\art\icon256.ico
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/max
SolidCompression=yes
DisableStartupPrompt=yes
PrivilegesRequired=lowest
VersionInfoVersion={#MyAppVersion}.0
CloseApplications=force
DisableDirPage=no
; Same AppId as Nexo Desk (the old name), but a new folder.
UsePreviousAppDir=no
DisableProgramGroupPage=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
ru.DeleteUserData=Удалить также данные PrismGram: вход в аккаунт, настройки, темы и плагины?%n%nЕсли вы собираетесь переустановить программу, нажмите «Нет».
english.DeleteUserData=Also delete PrismGram data: account login, settings, themes and plugins?%n%nIf you are going to reinstall, choose No.

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#ReleasePath}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleasePath}\modules\x64\d3d\d3dcompiler_47.dll"; DestDir: "{app}\modules\x64\d3d"; Flags: ignoreversion
; Present only in builds with auto-update on.
Source: "{#ReleasePath}\Updater.exe"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppShortName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppShortName}}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#MyAppShortName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppShortName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[InstallDelete]
; Program files and shortcuts of Nexo Desk, the old name. User data in
; %APPDATA%\Nexo Desk is moved to %APPDATA%\PrismGram by the app itself.
Type: filesandordirs; Name: "{localappdata}\Programs\Nexo Desk"
Type: files; Name: "{userdesktop}\Nexo Desk.lnk"
Type: files; Name: "{userprograms}\Nexo Desk.lnk"
Type: filesandordirs; Name: "{userprograms}\Nexo Desk"

[UninstallDelete]
; Only program files. User data is handled in [Code] below.
Type: filesandordirs; Name: "{app}\modules"
Type: dirifempty; Name: "{app}"

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var ResultCode: Integer;
    DataDir: String;
begin
  if CurUninstallStep = usUninstall then
  begin
    // Removes autostart and similar system integration.
    Exec(ExpandConstant('{app}\{#MyAppExeName}'), '-cleanup', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
  if CurUninstallStep = usPostUninstall then
  begin
    DataDir := ExpandConstant('{userappdata}\{#MyAppName}');
    if DirExists(DataDir) and not UninstallSilent() then
    begin
      if MsgBox(CustomMessage('DeleteUserData'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
        DelTree(DataDir, True, True, True);
    end;
  end;
end;
