# PopkaStudio-Setup.exe's first step (installer/PopkaStudio.iss runs it hidden and shows its "@@" lines as a list):
# PopkaStudio itself is not inside the .exe - it is a git clone of the repository, exactly what install.ps1 makes, so
# the studio's own Update (the number on the heart) works for an installed copy too. Then install.ps1 -Setup.
#
#   bootstrap.ps1 -Dir <the install folder>
#
# git: the one on the PATH, else MinGit (Git for Windows' portable, ~50 MB unpacked) in <Dir>\.tools\git - for this
# copy only, nothing system-wide; bin\popkastudio.cmd and the studio's update find it there.
# POPKASTUDIO_REPO / POPKASTUDIO_BRANCH: another repository or branch (a test of a fork; CI).
# (Plain ASCII, as install.ps1: Windows PowerShell 5.1 reads a script without a BOM in the system's code page.)
param([Parameter(Mandatory = $true)][string]$Dir)
$ErrorActionPreference = 'Continue'
if ($PSVersionTable.PSEdition -eq 'Desktop') {  # (started from PowerShell 7: its module path breaks 5.1's - install.ps1)
  $env:PSModulePath = @("$HOME\Documents\WindowsPowerShell\Modules", "$env:ProgramFiles\WindowsPowerShell\Modules",
                        "$env:SystemRoot\system32\WindowsPowerShell\v1.0\Modules") -join ';'
}
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

$Repo = if ($env:POPKASTUDIO_REPO) { $env:POPKASTUDIO_REPO } else { 'https://github.com/PopaSpinka/PopkaStudio.git' }
$Branch = if ($env:POPKASTUDIO_BRANCH) { $env:POPKASTUDIO_BRANCH } else { 'main' }
$Log = Join-Path $Dir 'setup.log'  # (install.log is install.ps1's)

function Say($t) { Write-Host "> $t" }
function Die($t) { Write-Host "x $t"; Add-Content -LiteralPath $Log -Value "x $t" -ErrorAction SilentlyContinue; exit 1 }
function RunGit {  # (git, its exit code checked; its output into the installer's detail line and install.log)
  & git.exe @args 2>&1 | ForEach-Object { "$_" } | ForEach-Object {  # (not Tee-Object: in 5.1 it appends UTF-16)
    Add-Content -LiteralPath $Log -Value $_ -ErrorAction SilentlyContinue; $_ } | Out-Host
  if ($LASTEXITCODE -ne 0) { Die "git $($args -join ' ') failed (exit $LASTEXITCODE)" }
}

New-Item -ItemType Directory -Force -Path $Dir | Out-Null
Set-Content -LiteralPath $Log -Value "PopkaStudio setup $(Get-Date -Format s)" -ErrorAction SilentlyContinue

# ---------- 1. git ----------
Write-Host '@@ git'
$MinGit = Join-Path $Dir '.tools\git'
if (Test-Path "$MinGit\cmd\git.exe") { $env:PATH = "$MinGit\cmd;$env:PATH" }
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
  Say 'Downloading git (MinGit, portable - for PopkaStudio only)'
  try {
    $rel = Invoke-RestMethod -UseBasicParsing -Headers @{ 'User-Agent' = 'PopkaStudio-Setup' } `
      'https://api.github.com/repos/git-for-windows/git/releases/latest'
    $asset = $rel.assets | Where-Object { $_.name -match '^MinGit-[\d.]+-64-bit\.zip$' } | Select-Object -First 1
  } catch { Die "Can't reach GitHub for git: $($_.Exception.Message)" }
  if (-not $asset) { Die "No MinGit in git-for-windows' latest release" }
  New-Item -ItemType Directory -Force -Path $MinGit | Out-Null
  $zip = Join-Path $MinGit 'mingit.zip'  # (not %TEMP%: for a user name with a space it is a short 8.3 path)
  & curl.exe -fsSL --retry 3 -o $zip $asset.browser_download_url
  if ($LASTEXITCODE -ne 0) { Die "Can't download $($asset.browser_download_url) (curl exit $LASTEXITCODE)" }
  Expand-Archive -Force -LiteralPath $zip -DestinationPath $MinGit -ErrorAction Stop
  [IO.File]::Delete($zip)
  $env:PATH = "$MinGit\cmd;$env:PATH"
  if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Die 'git did not install' }
}

# ---------- 2. PopkaStudio: a clone (as install.ps1 makes it), or the one already here brought up to date ----------
Write-Host '@@ code'
Set-Location -LiteralPath $Dir
# (a clone counts once it has a commit: a first run cut off in its download left a .git without one - start that over)
& git.exe -C $Dir rev-parse -q --verify HEAD 2>$null | Out-Null
if ((Test-Path (Join-Path $Dir '.git')) -and $LASTEXITCODE -eq 0) {
  Say 'Updating PopkaStudio'
  & git.exe -C $Dir pull --ff-only 2>&1 | ForEach-Object { "$_" } | ForEach-Object {
    Add-Content -LiteralPath $Log -Value $_ -ErrorAction SilentlyContinue; $_ } | Out-Host
  # (no update - offline, or the code changed by hand - is no reason to stop: the tools still get set up)
  if ($LASTEXITCODE -ne 0) { Say "Couldn't update PopkaStudio (git pull: exit $LASTEXITCODE) - going on with this version" }
} else {
  # (git clone wants an empty folder; this one already holds .tools and the uninstaller - and, over a copy from the
  # first installer, PopkaStudio's files without git: checkout -f puts the repository's over them, projects\ stays)
  Say "Downloading PopkaStudio ($Repo)"
  RunGit -C $Dir init -q  # (no -b: git before 2.28 lacks it; checkout -B names the branch)
  & git.exe -C $Dir remote remove origin 2>$null | Out-Null
  RunGit -C $Dir remote add origin $Repo
  RunGit -C $Dir fetch --depth 1 origin "+refs/heads/${Branch}:refs/remotes/origin/$Branch"
  RunGit -C $Dir checkout -q -f -B $Branch "origin/$Branch"
  RunGit -C $Dir branch -q --set-upstream-to "origin/$Branch"
}
if (-not (Test-Path (Join-Path $Dir 'install.ps1'))) { Die "$Dir has no install.ps1 - not PopkaStudio's repository?" }

# ---------- 3. Python, arduino-cli, the ESP32 core, the fonts: install.ps1 -Setup (its own @@ steps) ----------
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Dir 'install.ps1') -Setup | Out-Host
exit $LASTEXITCODE
