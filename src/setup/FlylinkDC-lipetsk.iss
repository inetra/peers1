; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=FlylinkDC++
AppVerName=FlylinkDC++ r(300)
AppPublisher=FlylinkDC++ Team
AppPublisherURL=http://www.flylinkdc.ru
AppSupportURL=http://svn.flylinkdc.ru/trac
AppUpdatesURL=http://www.flylinkdc.ru
DefaultDirName={pf}\FlylinkDC++
DefaultGroupName=FlylinkDC++
InfoBeforeFile="readme.rtf"
OutputBaseFilename=SetupFlylinkDC-r300-lipetsk
SetupIconFile="..\res\StrongDC.ico"
Compression=lzma/ultra
SolidCompression=yes
WizardImageFile=setup-1.bmp
WizardSmallImageFile=setup-2.bmp


[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Components]
Name: "program"; Description: "Program Files"; Types: full compact custom; Flags: fixed
Name: "Lang"; Description: "����"; Types: full
Name: "Lang\ru"; Description: "�������"; Flags: exclusive
Name: "Lang\preved"; Description: "���������"; Flags: exclusive
Name: "Lang\en"; Description: "����������"; Flags: exclusive
Name: "Lang\ukr"; Description: "����������"; Flags: exclusive

Name: "Chatbot"; Description: "���-���";
Name: "DCPlusPlus"; Description: "����-��������� �� ���� (������� ���� ����� + ����)"; Types: full

Name: "DCPlusPlus\Lipetsk_domolink"; Description: "�.������ + ������� (���� ��������)"; Flags: exclusive
Name: "DCPlusPlus\Lipetsk_voice"; Description: "�.������ (���� �����)"; Flags: exclusive
Name: "DCPlusPlus\Lipetsk_lis"; Description: "�.������ (�������� �������������� ����)"; Flags: exclusive
Name: "DCPlusPlus\Lipetsk_lks"; Description: "�.������ (�������� ��������� ����)"; Flags: exclusive
Name: "DCPlusPlus\Lipetsk_media"; Description: "�.������ (���������)"; Flags: exclusive


Name: "Scripts"; Description: "PHP-c������";
Name: "Sounds"; Description: "�����";


[Files]
Source: "..\compiled\FlylinkDC.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\compiled\FlylinkDC.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\compiled\AVIPreview.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EN_Example.xml"; DestDir: "{app}"; Flags: ignoreversion

Source: "..\compiled\Russian.xml"; DestDir: "{app}";  Components: Lang\ru; Flags: ignoreversion
Source: "..\compiled\Settings\lang_preved\Russian.xml"; DestDir: "{app}";  Components: Lang\preved; Flags: ignoreversion
Source: "..\compiled\Settings\lang_eng\Russian.xml"; DestDir: "{app}";  Components: Lang\en; Flags: ignoreversion
Source: "..\compiled\Settings\lang_ukr\Russian.xml"; DestDir: "{app}";  Components: Lang\ukr; Flags: ignoreversion
Source: "..\compiled\BackUp\BackupProfile.bat"; DestDir: "{app}\BackUp"; Flags: ignoreversion

Source: "Readme.rtf"; DestDir: "{app}"; Flags: isreadme

Source: "..\compiled\Sounds\*.wav"; DestDir: "{app}\Settings\Sounds"; Components: Sounds; Flags: overwritereadonly ignoreversion
Source: "..\compiled\Themes\*.dctheme"; DestDir: "{app}\Themes"; Flags: ignoreversion
Source: "..\compiled\EmoPacks\*.*"; Excludes: "\.svn\"; DestDir: "{app}\EmoPacks"; Flags: createallsubdirs overwritereadonly ignoreversion recursesubdirs

Source: "..\compiled\chatbot\*.*"; Excludes: "\.svn\"; DestDir: "{app}\chatbot"; Components: Chatbot; Flags: createallsubdirs overwritereadonly ignoreversion recursesubdirs

Source: "..\compiled\Settings\Lipetsk\Favorites.xml"; DestDir: "{app}\Settings";Components: DCPlusPlus\Lipetsk_domolink; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_domolink; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk\IPTrust.ini"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_domolink; Flags: onlyifdoesntexist ignoreversion

Source: "..\compiled\Settings\Lipetsk_voice\Favorites.xml"; DestDir: "{app}\Settings";Components: DCPlusPlus\Lipetsk_voice; Flags: onlyifdoesntexist ignoreversion

Source: "..\compiled\Settings\Lipetsk_lis\Favorites.xml"; DestDir: "{app}\Settings";Components: DCPlusPlus\Lipetsk_lis; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_lis\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_lis; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_lis\CustomLocations.ini"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_lis; Flags:  onlyifdoesntexist ignoreversion

Source: "..\compiled\Settings\Lipetsk_media\Favorites.xml"; DestDir: "{app}\Settings";Components: DCPlusPlus\Lipetsk_media; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_media\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_media; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_media\CustomLocations.ini"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_media; Flags:  onlyifdoesntexist ignoreversion

Source: "..\compiled\Settings\Lipetsk_lks\Favorites.xml"; DestDir: "{app}\Settings";Components: DCPlusPlus\Lipetsk_lks; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_lks\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_lks; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\Lipetsk_lks\CustomLocations.ini"; DestDir: "{app}\Settings"; Components: DCPlusPlus\Lipetsk_lks; Flags:  onlyifdoesntexist ignoreversion

Source: "..\server\test.php"; DestDir: "{app}\Scripts"; Components: Scripts; Flags: ignoreversion
Source: "..\server\getip.php"; DestDir: "{app}\Scripts"; Components: Scripts; Flags: ignoreversion

Source: "..\compiled\Settings\CustomLocations.bmp"; DestDir: "{app}\Settings"; Flags:  onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\CustomLocations.ini"; DestDir: "{app}\Settings"; Flags:  onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\IPTrust.ini"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\common\DCPlusPlus.xml"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion




; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\FlylinkDC++"; Filename: "{app}\FlylinkDC.exe"; WorkingDir: "{app}"
Name: "{group}\� ���������"; Filename: "{app}\Readme.rtf"
Name: "{group}\{cm:ProgramOnTheWeb,FlylinkDC++}"; Filename: "http://www.flylinkdc.ru"
Name: "{group}\{cm:UninstallProgram,FlylinkDC++}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\FlylinkDC++"; Filename: "{app}\FlylinkDC.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\FlylinkDC++"; Filename: "{app}\FlylinkDC.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\FlylinkDC.exe"; Description: "{cm:LaunchProgram,FlylinkDC++}"; Flags: nowait postinstall skipifsilent


