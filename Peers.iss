[Setup]
AppName=Peers
AppVerName=Peers r@BUILD.NUMBER@
AppPublisher=Inetra
AppPublisherURL=http://www.cn.ru/peers/
AppSupportURL=http://www.cn.ru/peers/
AppUpdatesURL=http://www.cn.ru/peers/
DefaultDirName={pf}\Peers
DefaultGroupName=Peers
;InfoBeforeFile="src\setup\readme.rtf"
LicenseFile="setup\licence.txt"
OutputDir=Output\@BUILD.NUMBER@
OutputBaseFilename=PeersSetup@BUILD.NUMBER@
SetupIconFile="src\res\peers\peers-app.ico"
Compression=lzma/ultra
SolidCompression=yes
WizardImageFile=setup\setup-1.bmp
WizardSmallImageFile=src\res\peers\app-logo.bmp
;release instance
AppMutex={{DOMODC-AEE8350A-B49A-4753-AB4B-E55479A48351}

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "startupicon"; Description: "Создать значок в группе Автозагрузка"; GroupDescription: "{cm:AdditionalIcons}";
Name: "downloadsicon"; Description: "Добавить ярлык для папки загрузок на рабочий стол"; GroupDescription: "{cm:AdditionalIcons}";
Name: "siteIcon"; Description: "Добавить ярлык для сайта CN.RU на рабочий стол"; GroupDescription: "{cm:AdditionalIcons}";

;[Components]
;Name: "program"; Description: "Program Files"; Types: full compact custom; Flags: fixed
;Name: "Lang"; Description: "Язык"; Types: full
;Name: "Lang\ru"; Description: "Русский"; Flags: exclusive
;Name: "Lang\en"; Description: "Английский"; Flags: exclusive

[Dirs]
Name: "{%USERPROFILE}\Загрузки Peers"; Tasks: downloadsicon; MinVersion: "0, 6.0"
Name: "{userdocs}\Загрузки Peers"; Tasks: downloadsicon; OnlyBelowVersion: "0, 6.0"

[Files]
Source: "setup\Peers.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "setup\Peers.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "setup\dcppboot.xml"; DestDir: "{app}"; Flags: onlyifdoesntexist ignoreversion
Source: "src\compiled\dbghelp.dll"; DestDir: "{app}"
Source: "setup\crshhndl.dll"; DestDir: "{app}"

Source: "src\compiled\Russian.xml"; DestDir: "{app}";  Flags: ignoreversion
;Source: "src\compiled\Settings\lang_eng\Russian.xml"; DestDir: "{app}";  Components: Lang\en; Flags: ignoreversion

;Source: "src\setup\readme.rtf"; DestDir: "{app}"

Source: "src\compiled\Sounds\*.wav"; DestDir: "{app}\Settings\Sounds"; Flags: overwritereadonly ignoreversion
Source: "src\compiled\Themes\*.dctheme"; DestDir: "{app}\Themes"; Flags: ignoreversion
Source: "src\compiled\EmoPacks\*.*"; DestDir: "{app}\EmoPacks"; Flags: createallsubdirs overwritereadonly ignoreversion recursesubdirs
Source: "src\compiled\VLC\*.*"; DestDir: "{app}\VLC"; Excludes: "Source,.svn"; Flags: createallsubdirs overwritereadonly ignoreversion recursesubdirs

Source: "setup\Favorites.xml"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion
;Source: "setup\IPTrust.ini"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion
Source: "src\compiled\Settings\common\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion

[Registry]
Root: HKCU; Subkey: "Software\Novotelecom\Peers"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Novotelecom\Peers"; ValueName: "DownloadDirectoryShortcut"; ValueType: dword; ValueData: 1; Tasks: downloadsicon
Root: HKCU; Subkey: "Software\Novotelecom\Peers"; ValueName: "DownloadDirectoryShortcut"; ValueType: dword; ValueData: 0; Tasks: not downloadsicon
Root: HKCU; Subkey: "Software\Novotelecom\Peers"; ValueName: "SiteShortcut"; ValueType: dword; ValueData: 1; Tasks: siteIcon
Root: HKCU; Subkey: "Software\Novotelecom\Peers"; ValueName: "SiteShortcut"; ValueType: dword; ValueData: 0; Tasks: not siteIcon

[Icons]
Name: "{group}\Peers"; Filename: "{app}\Peers.exe"; WorkingDir: "{app}"
;Name: "{group}\О программе"; Filename: "{app}\Readme.rtf"
Name: "{group}\{cm:ProgramOnTheWeb,Peers}"; Filename: "http://www.cn.ru/peers/"
Name: "{group}\{cm:UninstallProgram,Peers}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Peers"; Filename: "{app}\Peers.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Peers"; Filename: "{app}\Peers.exe"; Tasks: quicklaunchicon
Name: "{commonstartup}\Peers"; Filename: "{app}\Peers.exe"; Parameters: "/startup"; Tasks: startupicon
Name: "{userdesktop}\Загрузки Peers"; Filename: "{%USERPROFILE}\Загрузки Peers"; Tasks: downloadsicon; MinVersion: "0, 6.0"
Name: "{userdesktop}\Загрузки Peers"; Filename: "{userdocs}\Загрузки Peers"; Tasks: downloadsicon; OnlyBelowVersion: "0, 6.0"
Name: "{userdesktop}\CN.ru"; Filename: "http://www.cn.ru/"; Tasks: siteIcon; IconFilename: "{app}\Peers.exe"; IconIndex: 52

[Run]
Filename: "netsh.exe"; Description: "{cm:LaunchProgram,Peers}"; Parameters: "firewall add allowedprogram ""{app}\Peers.exe"" ""Peers"" ENABLE ALL"; StatusMsg: "Configuring firewall..."; Flags: skipifsilent runhidden
Filename: "{app}\Peers.exe"; Description: "{cm:LaunchProgram,Peers}"; Flags: nowait postinstall skipifsilent
