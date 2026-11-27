# PopkaStudio installer for Windows 10 and 11 (install.sh does the same on macOS and Linux). Everything goes into
# PopkaStudio's own folder (.venv, .tools) - nothing system-wide, no administrator rights.
#
#   powershell -ExecutionPolicy Bypass -c "irm https://raw.githubusercontent.com/PopaSpinka/PopkaStudio/main/install.ps1 | iex"
#   or, in a copy you cloned:  powershell -ExecutionPolicy Bypass -File .\install.ps1     (run it again any time to update)
#
# It installs: uv (Python and its libraries numpy, Pillow, pyserial), arduino-cli and the ESP32 core 3.3.11 (to build
# and upload the firmware), the fonts, the command popkastudio (bin\popkastudio.cmd) and a Start menu shortcut.
# Needs git (winget install --id Git.Git -e). The board's USB (ESP32-S3/P4 native USB) needs no driver on Windows 10+.
#
# -Setup: for the classic installer (installer\PopkaStudio.iss clones, then runs this; the studio's Update too) -
# no git, no shortcut and no PATH (the installer makes its own), the log in install.log next to it.
# POPKASTUDIO_SKIP_CORE=1: everything but the ESP32 core (a test run: the editor works, Load doesn't).
# (This file is plain ASCII: Windows PowerShell 5.1 reads a script without a BOM in the system's code page.)
# (Continue: in Windows PowerShell 5.1 'Stop' turns any line a native tool writes to stderr into a failure - each
# native step's exit code is checked instead, the downloads stop on their own errors)
param([switch]$Setup)
$ErrorActionPreference = 'Continue'
# Windows PowerShell started from PowerShell 7 (a terminal, the installer, CI) inherits 7's module path and then can't
# load its own modules ("The member AuditToString is already present"): only Windows PowerShell's own folders
if ($PSVersionTable.PSEdition -eq 'Desktop') {
  $env:PSModulePath = @("$HOME\Documents\WindowsPowerShell\Modules", "$env:ProgramFiles\WindowsPowerShell\Modules",
                        "$env:SystemRoot\system32\WindowsPowerShell\v1.0\Modules") -join ';'
}
$ProgressPreference = 'SilentlyContinue'  # (Invoke-WebRequest's progress bar makes big downloads many times slower)
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

$Repo = if ($env:POPKASTUDIO_REPO) { $env:POPKASTUDIO_REPO } else { 'https://github.com/PopaSpinka/PopkaStudio.git' }
$Core = 'esp32:esp32@3.3.11'
$EspUrl = 'https://espressif.github.io/arduino-esp32/package_esp32_index.json'
$UvZip = 'https://github.com/astral-sh/uv/releases/latest/download/uv-x86_64-pc-windows-msvc.zip'
$CliZip = 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip'

function Mark($k) { if ($Setup) { Write-Host "@@ $k" } }  # (a step for the installer's list: uv, python, cli, core, fonts, done)
function Say($t) { Write-Host ''; Write-Host "> $t" -ForegroundColor White }
function Note($t) { Write-Host "  $t" }
function Die($t) {
  Write-Host ''; Write-Host "x $t" -ForegroundColor Red
  if ($Setup) { Stop-Transcript | Out-Null }
  exit 1
}
function Get-Zip($url) {  # (a tool's zip into .tools\bin. Not via %TEMP%: for a user name with a space it is a short
  # 8.3 path (C:\Users\AILAB~1\...) that Windows PowerShell's Remove-Item can't find; .NET deletes by any path)
  $zip = Join-Path $Tools 'download.zip'
  if (Get-Command curl.exe -ErrorAction SilentlyContinue) {  # (Windows 10+ has curl: much faster than Invoke-WebRequest)
    & curl.exe -fsSL --retry 3 -o $zip $url
    if ($LASTEXITCODE -ne 0) { Die "Can't download $url (curl exit $LASTEXITCODE)" }
  } else {
    Invoke-WebRequest -UseBasicParsing -ErrorAction Stop $url -OutFile $zip
  }
  Expand-Archive -Force -ErrorAction Stop -LiteralPath $zip -DestinationPath $Tools
  [IO.File]::Delete($zip)
}
function Run {  # (a native tool; its exit code checked. A plain function: its -flags go through untouched)
  $exe, $rest = $args
  & $exe @rest 2>&1 | ForEach-Object { "$_" } | Out-Host  # (through the host, its errors too: install.log keeps them)
  if ($LASTEXITCODE -ne 0) { Die "$exe $($rest -join ' ') failed (exit $LASTEXITCODE)" }
}

if ($env:OS -ne 'Windows_NT') { Die 'install.ps1 is for Windows. macOS and Linux: ./install.sh' }
if (-not [Environment]::Is64BitOperatingSystem) { Die 'PopkaStudio needs 64-bit Windows.' }

# ---------- 1. The code: this folder (a clone), or a fresh copy in %USERPROFILE%\PopkaStudio ----------
$Here = $PSScriptRoot  # (empty when piped into iex)
if ($Setup) { Start-Transcript -Force -Path (Join-Path $Here 'install.log') | Out-Null }
if ($Here -and ((Test-Path "$Here\studio\studio.py") -or (Test-Path "$Here\PopkaStudio\studio.py"))) {
  $Dir = $Here
  if ((Test-Path "$Dir\.git") -and (Get-Command git -ErrorAction SilentlyContinue)) {
    Say 'Updating PopkaStudio'
    & git -C $Dir pull --ff-only
    if ($LASTEXITCODE -ne 0) { Note "(not updated: $Dir has changes of its own - kept as it is)" }
  }
} else {
  $Dir = if ($env:POPKASTUDIO_DIR) { $env:POPKASTUDIO_DIR } else { Join-Path $env:USERPROFILE 'PopkaStudio' }
  if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Die 'git is needed: winget install --id Git.Git -e   (then open a new PowerShell window and run this again)'
  }
  if (Test-Path "$Dir\.git") {
    Say "Updating PopkaStudio in $Dir"
    Run git -C $Dir pull --ff-only
  } else {
    Say "Downloading PopkaStudio into $Dir"
    Run git clone --depth 1 $Repo $Dir
  }
}
Set-Location $Dir
$Tools = Join-Path $Dir '.tools\bin'
New-Item -ItemType Directory -Force -Path $Tools | Out-Null
$env:PATH = "$Tools;$env:PATH"

# ---------- 2. Python and its libraries (uv: its own Python, in .venv) ----------
Mark uv
if (-not (Get-Command uv -ErrorAction SilentlyContinue)) {
  Say 'Installing uv (a Python manager) into .tools\bin'
  Get-Zip $UvZip  # (uv's own release; its zip holds uv.exe, uvx.exe, uvw.exe)
  if (-not (Test-Path "$Tools\uv.exe")) { Die 'uv did not install (see above).' }
}
Mark python
Say 'Python and its libraries'
# (uv's Python and its cache inside PopkaStudio's folder too: an uninstall takes them, nothing is left in %APPDATA%)
$env:UV_PYTHON_INSTALL_DIR = Join-Path $Dir '.tools\python'
$env:UV_CACHE_DIR = Join-Path $Dir '.tools\uv-cache'
$Py = Join-Path $Dir '.venv\Scripts\python.exe'
# (made once: an update runs this while PopkaStudio itself runs on .venv's python.exe, and Windows can't write over a
# program that is running - uv venv --allow-existing would fail there; the libraries below are checked every time)
# (a .venv whose Python is gone - "uv trampoline failed to spawn Python child process": its base Python moved or was
# replaced by uv - is made again; the libraries come back from uv's cache)
if (Test-Path $Py) {
  & $Py -c "import sys" 2>$null | Out-Null
  if ($LASTEXITCODE -ne 0) {
    Say 'The Python in .venv does not start - making it again'
    [IO.Directory]::Delete((Join-Path $Dir '.venv'), $true)
  }
}
if (-not (Test-Path $Py)) { Run uv venv --quiet --python 3.12 --allow-existing .venv }
Run uv pip install --quiet --python $Py numpy pillow pyserial

# ---------- 3. arduino-cli and the ESP32 core (Load: build and upload the firmware) ----------
Mark cli
$Cli = Join-Path $Tools 'arduino-cli.exe'
if (-not (Test-Path $Cli)) {
  $found = Get-Command arduino-cli -ErrorAction SilentlyContinue
  if ($found) { $Cli = $found.Source } else {
    Say 'Installing arduino-cli into .tools\bin'
    Get-Zip $CliZip
  }
}
Mark core
$cores = (& $Cli core list 2>$null) -join "`n"
if ($env:POPKASTUDIO_SKIP_CORE -eq '1') { Note '(the ESP32 core skipped: POPKASTUDIO_SKIP_CORE)' }
elseif ($cores -notmatch '(?m)^esp32:esp32 +3\.3\.11 ') {
  Say 'The ESP32 tools - the longest step (1.5-3 GB, 5-20 minutes)'
  Run $Cli core update-index --additional-urls $EspUrl
  Run $Cli core install $Core --additional-urls $EspUrl
}

# ---------- 4. The fonts (Google Sans Flex, Font Awesome Free: from their own repositories) ----------
Mark fonts
Say 'Fonts'
Run $Py -X utf8 -c "import sys; sys.path.insert(0, 'tools'); import halo_assets as h; [h.font_path(n) for n in ('GoogleSansFlex.ttf', 'fa-solid-900.ttf')]"

# ---------- 5. The command popkastudio and a Start menu shortcut ----------
$Run = Join-Path $Dir 'bin\popkastudio.cmd'
if (-not (Test-Path $Run)) { Die "$Run is missing - the update did not go in (see above). Run this installer again." }
if ($Setup) {  # (the installer has made the shortcuts and the uninstaller itself)
  Mark done
  Say 'PopkaStudio is ready'
  Stop-Transcript | Out-Null
  exit 0
}
$Ico = Join-Path $Dir '.tools\popkastudio.ico'
& $Py -c "from PIL import Image; Image.open(r'$Dir\studio\app\icon.png').save(r'$Ico', sizes=[(16,16),(32,32),(48,48),(256,256)])" 2>$null
$Menu = Join-Path ([Environment]::GetFolderPath('Programs')) 'PopkaStudio.lnk'
try {
  $sh = (New-Object -ComObject WScript.Shell).CreateShortcut($Menu)
  $sh.TargetPath = $Run
  $sh.WorkingDirectory = $Dir
  $sh.WindowStyle = 7  # (minimized: the console is only the helper's log)
  $sh.Description = 'Watch screens for ESP32 boards'
  if (Test-Path $Ico) { $sh.IconLocation = $Ico }
  $sh.Save()
} catch { Note "(no Start menu shortcut: $($_.Exception.Message))" }
# popkastudio in a new terminal: bin on the user's PATH (only the user's, no administrator)
$bin = Join-Path $Dir 'bin'
$userPath = [Environment]::GetEnvironmentVariable('PATH', 'User')
if (-not (($userPath -split ';') -contains $bin)) {
  [Environment]::SetEnvironmentVariable('PATH', (@($userPath, $bin) | Where-Object { $_ }) -join ';', 'User')
}

Say 'PopkaStudio is ready'
Note 'Start it: PopkaStudio in the Start menu, or popkastudio in a new terminal window.'
Note 'It opens http://localhost:8466 in your browser. Connect the board with a USB-C cable and press Load.'
exit 0
