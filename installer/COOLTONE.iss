#define MyAppName "COOLTONE"
#define MyAppVersion "0.2.0"
#define MyAppPublisher "YMY Records"
[Setup]
AppId={{A92C3F0D-5C11-4C0B-8B5C-0A1C00202026}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\COOLTONE
DefaultGroupName=COOLTONE
OutputDir=Output
OutputBaseFilename=COOLTONE-Setup
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
[Dirs]
Name: "{commoncf}\VST3\COOLTONE.vst3"
[Files]
Source: "..\dist\VST3\COOLTONE.vst3\*"; DestDir: "{commoncf}\VST3\COOLTONE.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\dist\Standalone\COOLTONE.exe"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\COOLTONE"; Filename: "{app}\COOLTONE.exe"
[Run]
Filename: "{app}\COOLTONE.exe"; Description: "Launch COOLTONE"; Flags: postinstall nowait skipifsilent
