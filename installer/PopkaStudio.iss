; (UTF-8 with a BOM: Inno Setup reads the Russian lines right only so)
; PopkaStudio-Setup.exe - the classic Windows installer (Inno Setup 6.3+). Built by .github/workflows/windows-installer.yml:
;   iscc /DAppVersion=2.8 installer\PopkaStudio.iss      -> installer\Output\PopkaStudio-Setup-2.8.exe
; The .exe holds no PopkaStudio: bootstrap.ps1 makes the same git clone install.ps1 makes (git: the one on the PATH, or a
; portable MinGit in .tools\git), so the studio's own Update works for this copy as for any other; then install.ps1
; -Setup. Run again, it updates (git pull). For this user only, no administrator: %LOCALAPPDATA%\PopkaStudio (a short
; path: the ESP32 build doesn't like long ones). The installer's own files go to .tools\setup (.tools: not in git).

#ifndef AppVersion
  #define AppVersion "0.0"
#endif

[Setup]
AppId={{6C1F3B0E-2F5B-4C1E-9C55-9F0A2B7D4E61}
AppName=PopkaStudio
AppVersion={#AppVersion}
AppVerName=PopkaStudio
AppPublisher=PopkaStudio
AppPublisherURL=https://github.com/PopaSpinka/PopkaStudio
DefaultDirName={localappdata}\PopkaStudio
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=Output
OutputBaseFilename=PopkaStudio-Setup-{#AppVersion}
SetupIconFile=popkastudio.ico
UninstallFilesDir={app}\.tools\setup
UninstallDisplayIcon={app}\.tools\setup\popkastudio.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
en.Tools=Downloading PopkaStudio and its tools (1.5-3 GB, 5-20 minutes the first time)...
ru.Tools=Загрузка PopkaStudio и её инструментов (1,5-3 ГБ, первый раз 5-20 минут)...
en.ToolsFailed=Setting PopkaStudio up failed (exit %1). Check the internet and run the installer again: it goes on from where it stopped. The logs: %2
ru.ToolsFailed=Не удалось установить PopkaStudio (код %1). Проверьте интернет и запустите установщик ещё раз: он продолжит с того же места. Логи: %2
en.StepGit=git (to get PopkaStudio and its updates)
ru.StepGit=git (чтобы скачать PopkaStudio и её обновления)
en.StepCode=PopkaStudio
ru.StepCode=PopkaStudio
en.StepUv=Python manager (uv)
ru.StepUv=Менеджер Python (uv)
en.StepPython=Python and its libraries
ru.StepPython=Python и библиотеки
en.StepCli=arduino-cli
ru.StepCli=arduino-cli
en.StepCore=ESP32 tools (1.5-3 GB, the longest step)
ru.StepCore=Инструменты ESP32 (1,5-3 ГБ, самый долгий шаг)
en.StepFonts=Fonts
ru.StepFonts=Шрифты
en.DelProjects=Delete your PopkaStudio projects too?%n%nThey are in %1, with the saved Wi-Fi passwords and the copies of boards' firmware. No - they stay, and a new install finds them.
ru.DelProjects=Удалить и ваши проекты PopkaStudio?%n%nОни лежат в %1 вместе с сохранёнными паролями Wi-Fi и копиями прошивок плат. «Нет» — они останутся, и новая установка их найдёт.
en.DelTools=Delete the ESP32 tools too (%1, 1.5-3 GB)?%n%nThey are arduino-cli's common folder: say No if you use the Arduino IDE or arduino-cli for other things.
ru.DelTools=Удалить и инструменты ESP32 (%1, 1,5-3 ГБ)?%n%nЭто общая папка arduino-cli: ответьте «Нет», если пользуетесь Arduino IDE или arduino-cli для чего-то ещё.
en.Desktop=Create a desktop shortcut
ru.Desktop=Создать ярлык на рабочем столе

[Tasks]
Name: "desktop"; Description: "{cm:Desktop}"; Flags: unchecked

[Files]
Source: "bootstrap.ps1"; Flags: dontcopy
Source: "popkastudio.ico"; DestDir: "{app}\.tools\setup"; Flags: ignoreversion

[Icons]
Name: "{userprograms}\PopkaStudio"; Filename: "{app}\bin\popkastudio.cmd"; WorkingDir: "{app}"; \
  IconFilename: "{app}\.tools\setup\popkastudio.ico"; Flags: runminimized
Name: "{userdesktop}\PopkaStudio"; Filename: "{app}\bin\popkastudio.cmd"; WorkingDir: "{app}"; \
  IconFilename: "{app}\.tools\setup\popkastudio.ico"; Flags: runminimized; Tasks: desktop

[Run]
Filename: "{app}\bin\popkastudio.cmd"; WorkingDir: "{app}"; Description: "{cm:LaunchProgram,PopkaStudio}"; \
  Flags: postinstall nowait skipifsilent runminimized

[UninstallRun]
; (a running PopkaStudio holds its Python: stop it first, by its port's owner)
Filename: "powershell.exe"; Parameters: "-NoProfile -Command ""Get-NetTCPConnection -LocalPort 8466 -State Listen -ErrorAction SilentlyContinue | ForEach-Object {{ Stop-Process -Id $_.OwningProcess -Force }"""; \
  Flags: runhidden; RunOnceId: "StopStudio"

[Code]
// After the files: bootstrap.ps1 (git, the clone, then install.ps1 -Setup), hidden, its output read line by line
// (ExecAndLogOutput). Its "@@ <step>" lines move the list of steps under the progress bar; other lines are the detail.
const
  StepCount = 7;
var
  StepKeys: array[0..6] of String;
  StepNames: array[0..6] of String;
  StepPct: array[0..7] of Integer;
  StepLabels: array[0..6] of TNewStaticText;
  CurrentStep: Integer;

procedure ShowSteps(Current: Integer);
var
  I: Integer;
begin
  for I := 0 to StepCount - 1 do begin
    if I < Current then begin
      StepLabels[I].Caption := '✓   ' + StepNames[I];
      StepLabels[I].Font.Color := clGreen;
      StepLabels[I].Font.Style := [];
    end else if I = Current then begin
      StepLabels[I].Caption := '›   ' + StepNames[I];
      StepLabels[I].Font.Color := clWindowText;
      StepLabels[I].Font.Style := [fsBold];
    end else begin
      StepLabels[I].Caption := '○   ' + StepNames[I];
      StepLabels[I].Font.Color := clGrayText;
      StepLabels[I].Font.Style := [];
    end;
  end;
  WizardForm.ProgressGauge.Position := StepPct[Current];
end;

procedure AddStep(I: Integer; Key, Msg: String; Pct: Integer);
begin
  StepKeys[I] := Key;
  StepNames[I] := CustomMessage(Msg);
  StepPct[I] := Pct;
  StepLabels[I] := TNewStaticText.Create(WizardForm);
  StepLabels[I].Parent := WizardForm.InstallingPage;
  StepLabels[I].Left := WizardForm.ProgressGauge.Left;
  StepLabels[I].Top := WizardForm.ProgressGauge.Top + WizardForm.ProgressGauge.Height + ScaleY(14) + I * ScaleY(20);
  StepLabels[I].AutoSize := True;
  StepLabels[I].Visible := False;
end;

procedure InitializeWizard;
begin
  AddStep(0, 'git', 'StepGit', 3);
  AddStep(1, 'code', 'StepCode', 8);
  AddStep(2, 'uv', 'StepUv', 15);
  AddStep(3, 'python', 'StepPython', 20);
  AddStep(4, 'cli', 'StepCli', 30);
  AddStep(5, 'core', 'StepCore', 35);
  AddStep(6, 'fonts', 'StepFonts', 95);
  StepPct[7] := 100;
end;

procedure OnSetupLine(const S: String; const Error, FirstLine: Boolean);
var
  I: Integer;
  Line: String;
begin
  Line := Trim(S);
  if Copy(Line, 1, 8) = '@@ info ' then begin  // (what is downloading, how much, how fast - if install.ps1 says)
    WizardForm.FilenameLabel.Caption := Copy(Line, 9, 140);
    Exit;
  end;
  if Copy(Line, 1, 3) = '@@ ' then begin
    Line := Trim(Copy(Line, 4, 40));
    if Line = 'done' then
      CurrentStep := StepCount
    else
      for I := 0 to StepCount - 1 do
        if StepKeys[I] = Line then
          CurrentStep := I;
    ShowSteps(CurrentStep);
    WizardForm.FilenameLabel.Caption := '';
  end else if (Line <> '') and (Copy(Line, 1, 1) <> '*') then
    WizardForm.FilenameLabel.Caption := Copy(Line, 1, 110);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code, I: Integer;
  App: String;
begin
  if CurStep <> ssPostInstall then
    Exit;
  App := ExpandConstant('{app}');
  ExtractTemporaryFile('bootstrap.ps1');
  WizardForm.StatusLabel.Caption := CustomMessage('Tools');
  WizardForm.FilenameLabel.Caption := '';
  WizardForm.ProgressGauge.Style := npbstNormal;
  WizardForm.ProgressGauge.Min := 0;
  WizardForm.ProgressGauge.Max := 100;
  for I := 0 to StepCount - 1 do
    StepLabels[I].Visible := True;
  CurrentStep := 0;
  ShowSteps(0);
  if not ExecAndLogOutput(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),
              '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\bootstrap.ps1') + '" -Dir "' + App + '"',
              App, SW_HIDE, ewWaitUntilTerminated, Code, @OnSetupLine) then
    Code := -1;
  if Code = 0 then
    ShowSteps(StepCount)
  else
    SuppressibleMsgBox(FmtMessage(CustomMessage('ToolsFailed'), [IntToStr(Code), App + '\setup.log, ' + App + '\install.log']),
                       mbError, MB_OK, IDOK);
end;

// The uninstall: PopkaStudio's folder is a git clone the installer didn't unpack - it goes here, all of it, but what is
// the user's: the projects (with the Wi-Fi passwords and the saved firmware) and the ESP32 tools (arduino-cli's common
// folder) go only when they say yes. Silent uninstall: both stay.
procedure DelAllBut(Dir: String; Keep: String);  // (Keep: '|a|b|' - names in Dir that stay)
var
  F: TFindRec;
begin
  if FindFirst(Dir + '\*', F) then
    try
      repeat
        if (F.Name <> '.') and (F.Name <> '..') and (Pos('|' + Lowercase(F.Name) + '|', Keep) = 0) then begin
          if F.Attributes and FILE_ATTRIBUTE_DIRECTORY <> 0 then
            DelTree(Dir + '\' + F.Name, True, True, True)
          else
            DeleteFile(Dir + '\' + F.Name);
        end;
      until not FindNext(F);
    finally
      FindClose(F);
    end;
end;

// The studio's own Update runs install.ps1 without -Setup, which puts {app}\bin on the user's PATH: take it off again.
procedure RemoveFromUserPath(Dir: String);
var
  Path, Rest, Part, Kept: String;
  I: Integer;
begin
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', Path) then
    Exit;
  Rest := Path + ';';
  Kept := '';
  repeat
    I := Pos(';', Rest);
    Part := Copy(Rest, 1, I - 1);
    Rest := Copy(Rest, I + 1, Length(Rest));
    if (Part <> '') and (CompareText(RemoveBackslashUnlessRoot(Part), RemoveBackslashUnlessRoot(Dir)) <> 0) then begin
      if Kept <> '' then
        Kept := Kept + ';';
      Kept := Kept + Part;
    end;
  until Rest = '';
  if Kept <> Path then
    RegWriteExpandStringValue(HKCU, 'Environment', 'Path', Kept);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  App, Arduino: String;
begin
  if CurUninstallStep <> usPostUninstall then
    Exit;
  App := ExpandConstant('{app}');
  RemoveFromUserPath(App + '\bin');
  Arduino := ExpandConstant('{localappdata}\Arduino15');
  if DirExists(Arduino) and
     (SuppressibleMsgBox(FmtMessage(CustomMessage('DelTools'), [Arduino]), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES) then begin
    DelTree(Arduino, True, True, True);
    DelTree(ExpandConstant('{localappdata}\arduino'), True, True, True);  // (arduino-cli's build cache)
  end;
  if DirExists(App + '\projects') and
     (SuppressibleMsgBox(FmtMessage(CustomMessage('DelProjects'), [App + '\projects']), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDNO) then begin
    DelAllBut(App, '|projects|studio|');
    DelAllBut(App + '\studio', '|secrets|backups|videos|');
    RemoveDir(App + '\studio');
  end else
    DelTree(App, True, True, True);
end;
