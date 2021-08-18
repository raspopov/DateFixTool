#if Platform == "x64"
  #define MyBitness		  "64-bit"
#else
  #define MyBitness		  "32-bit"
#endif

#define MyAppExe		    ExtractFileName( TargetPath )
#define MyAppSource		  TargetPath
#define MyAppId         GetStringFileInfo( MyAppSource, INTERNAL_NAME )
#define MyAppName			  GetStringFileInfo( MyAppSource, PRODUCT_NAME )
#define MyAppVersion	  GetFileProductVersion( MyAppSource )
#define MyAppCopyright	GetFileCopyright( MyAppSource )
#define MyAppPublisher	GetFileCompany( MyAppSource )
#define MyAppURL		    GetStringFileInfo( MyAppSource, "Comments" )
#define MyOutputDir		  ExtractFileDir( TargetPath )
#define MyOutput		    StringChange( MyAppId + " " + MyAppVersion + " " + MyBitness, " ", "_" )

[Setup]
AppId={#MyAppId}
AppName={#MyAppName} {#MyBitness}
AppVersion={#MyAppVersion}
VersionInfoVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright={#MyAppCopyright}
DefaultDirName={pf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyOutput}
Compression=lzma2/ultra64
SolidCompression=yes
InternalCompressLevel=ultra64
LZMAUseSeparateProcess=yes
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExe},0
DirExistsWarning=no
WizardImageFile=compiler:WizModernImage-IS.bmp
WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp
; AppMutex=Global\{#MyAppId}
SetupMutex=Global\Setup_{#MyAppId}
OutputManifestFile=Setup-Manifest.txt
#if Platform == "x64"
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
#endif

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
InstallService=Install and start '%1' service
UnInstallService=Stop and uninstall '%1' service
ru.InstallService=Установить и запустить службу '%1'
ru.UnInstallService=Остановить и удалить службу '%1'

[Files]
Source: "{#MyAppSource}"; DestDir: "{app}"; Flags: replacesameversion uninsrestartdelete
Source: "{#MyOutputDir}\README.html"; DestName: "ReadMe.html"; DestDir: "{app}"; Flags: replacesameversion uninsrestartdelete
Source: "..\LICENSE"; DestName: "License.txt"; DestDir: "{app}"; Flags: replacesameversion uninsrestartdelete

[Icons]
Name: "{group}\{cm:InstallService,{#MyAppName}}"; Filename: "{app}\{#MyAppExe}"; Parameters: "install"
Name: "{group}\{cm:UnInstallService,{#MyAppName}}"; Filename: "{app}\{#MyAppExe}"; Parameters: "uninstall"
Name: "{group}\ReadMe.html"; Filename: "{app}\ReadMe.html"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}";	Filename: "{uninstallexe}"

[Run]
; Install service
Filename: "{app}\{#MyAppExe}"; Parameters: "install"; StatusMsg: "{cm:InstallService,{#MyAppName}}..."; Flags: runascurrentuser

[UninstallRun]
; Stop service
Filename: "net.exe"; Parameters: "stop {#MyAppId}"; Flags: runascurrentuser
; Remove service
Filename: "{app}\{#MyAppExe}"; Parameters: "uninstall"; Flags: runascurrentuser

[UninstallDelete]
Name: "{app}"; Type: dirifempty
Name: "{pf}\{#MyAppPublisher}"; Type: dirifempty

; Microsoft Visual C++ Redistributable
[Files]
#if Platform == "x64"
Source: "VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall 64bit; Check: IsWin64; AfterInstall: ExecTemp( 'VC_redist.x64.exe', '/passive /norestart' );
#else
Source: "VC_redist.x86.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall 32bit; AfterInstall: ExecTemp( 'VC_redist.x86.exe', '/passive /norestart' );
#endif

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
	nCode: Integer;
begin
  // Looking for service mutex
  If CheckForMutexes( 'Global\{#MyAppId}' ) then begin
    // Stop service
    Exec( 'net.exe', 'stop {#MyAppId}', '', SW_SHOW, ewWaitUntilTerminated, nCode );
  end;
  Result := '';
end;

procedure ExecTemp(File, Params : String);
var
	nCode: Integer;
begin
	Exec( ExpandConstant( '{tmp}' ) + '\' + File, Params, ExpandConstant( '{tmp}' ), SW_SHOW, ewWaitUntilTerminated, nCode );
end;
