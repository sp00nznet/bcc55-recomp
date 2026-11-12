# One-click setup for the BCC 5.5 static recompilation. Run it by
# double-clicking Setup.cmd in the repo folder; docs/building.md is
# the same thing by hand, command for command.
#
# It checks the tools, copies your Borland C++ 5.5 folder into game\ (and
# bcc32.exe and ilink32.exe into work\), finds every function, lifts both
# tools to C, builds build\bcc32recomp.exe and build\ilink32recomp.exe, runs
# the conformance harness and leaves a shortcut to a prompt where the
# recompiled tools are on PATH. Each step is skipped when its output already
# exists (-Force redoes them), so a rerun after a failure resumes. Everything
# it does goes to setup.log.
#
#   -GameDir <the BCC55 folder, holding Bin\bcc32.exe>   skip the question (-Game works too)
#   -Yes                                                answer yes to every install question
#
# The toolkit is ..\pcrecomp (PCRECOMP overrides it). Under recomp-launcher
# (RECOMP_LAUNCHER=1) it never waits for Enter, and exit code 0 means the
# shortcut is there. Nothing of Borland C++ is ever downloaded.
param([switch]$Force, [switch]$Yes, [Alias('Game')][string]$GameDir = "")
$Game = $GameDir

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Toolkit = $env:PCRECOMP
if (-not $Toolkit) {
  $Toolkit = Join-Path (Split-Path -Parent $Root) 'pcrecomp'
}
$env:PCRECOMP = $Toolkit   # the Python steps read it too
$Log = Join-Path $Root 'setup.log'
Set-Location $Root
function Log($t) { Add-Content -Path $Log -Value $t -Encoding UTF8 }
Log "==== setup $(Get-Date -Format s)"

function Say($t, $c = 'Gray') { Write-Host $t -ForegroundColor $c; Log $t }
function Step($n, $t) { Write-Host ""; Say "[$n/7] $t" 'Cyan' }
function Pause-End { if (-not $Yes -and $env:RECOMP_LAUNCHER -ne '1') { Read-Host "Press Enter to close" | Out-Null } }
function Fail($t) {
  Say ""; Say "Setup stopped: $t" 'Red'
  Say "The details are in $Log. Fix that and run Setup.cmd again; finished steps are skipped." 'Yellow'
  Pause-End; exit 1
}
trap { Fail "$_" }
function Ask($q) { if ($Yes) { Say "$q yes"; return $true }; $a = Read-Host "$q [Y/n]"; return -not ($a -match '^[nN]') }
function Refresh-Path {
  $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
}
# Run a program with its output in the log. Windows PowerShell turns a native
# program's stderr into errors, so 'Stop' is off while it runs.
function Exec([string[]]$cmd) {
  $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
  $rest = @($cmd | Select-Object -Skip 1)   # @(): a one-element slice would splat as characters
  & $cmd[0] @rest 2>&1 | ForEach-Object { Log "$_" }
  $code = $LASTEXITCODE
  $ErrorActionPreference = $old
  return $code
}
function Run($what, [string[]]$cmd) {
  Say "  $what..."
  $code = Exec $cmd
  if ($code -ne 0) { Fail "$what failed (exit code $code)." }
}

if ($env:RECOMP_LAUNCHER -ne '1') { Clear-Host }
Say "Borland C++ 5.5 (bcc32 + ilink32): static recompilation setup" 'White'
Say "You need your own copy of the Borland C++ 5.5 Free Command-Line Tools (the BCC55 folder holding Bin\bcc32.exe) and about 1 GB free."

# ---------------------------------------------------------------- tools
Step 1 "Checking the tools"
# A Python that answers "Python 3.x". The Store's placeholder "python" (the
# one that opens the Store) answers nothing, so asking is the reliable test.
function Find-Python {
  # The per-user install winget makes, before PATH has it.
  $user = @(Get-ChildItem "$env:LOCALAPPDATA\Programs\Python\Python3*\python.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | ForEach-Object { ,@($_.FullName) })
  foreach ($c in (@(@('py', '-3'), @('python'), @('python3')) + $user)) {
    if (-not (Get-Command $c[0] -ErrorAction SilentlyContinue)) { continue }
    $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $rest = @($c | Select-Object -Skip 1)
    $v = (& $c[0] @rest --version 2>&1 | Out-String).Trim()
    $ErrorActionPreference = $old
    if ($v -match '^Python 3\.(\d+)' -and [int]$Matches[1] -ge 10) { return ,$c }
  }
  return $null
}
$pyargs = Find-Python
if (-not $pyargs) {
  Say "  Python 3.10 or newer is not installed."
  if (-not (Ask "  Install Python 3.12 now (winget, for your user only, about 30 MB)?")) { Fail "Python 3 is required." }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Fail "winget is missing. Install 'App Installer' from the Microsoft Store, or install Python yourself."
  }
  Exec @('winget', 'install', '-e', '--id', 'Python.Python.3.12', '--scope', 'user', '--accept-package-agreements', '--accept-source-agreements') | Out-Null
  Refresh-Path
  $pyargs = Find-Python
  if (-not $pyargs) { Fail "Python installed, but it can't be found. Install Python 3.12 from python.org yourself." }
}
Say "  Python: $($pyargs -join ' ')"

if ((Exec ($pyargs + @('-c', 'import pefile, capstone'))) -ne 0) {
  Say "  The Python packages pefile and capstone are missing (about 20 MB)."
  if (-not (Ask "  Install them now (pip, for your user only)?")) { Fail "pefile and capstone are required." }
  Run "Installing pefile and capstone" ($pyargs + @('-m', 'pip', 'install', '--user', 'pefile', 'capstone'))
}

if (-not (Test-Path (Join-Path $Toolkit 'tools\lift\lift32.py'))) {
  Say "  The pcrecomp toolkit is not beside this folder ($Toolkit)."
  if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    if (-not (Ask "  git is needed to fetch it. Install Git for Windows now (winget, about 60 MB)?")) { Fail "git is needed to fetch pcrecomp." }
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { Fail "winget is missing. Install Git for Windows yourself." }
    Exec @('winget', 'install', '-e', '--id', 'Git.Git', '--accept-package-agreements', '--accept-source-agreements') | Out-Null
    Refresh-Path
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Fail "Git did not install. Install Git for Windows yourself." }
  }
  if (-not (Ask "  Download it now (git, about 20 MB)?")) { Fail "pcrecomp is required at $Toolkit." }
  # The branch with this project's toolkit fixes, until it is merged.
  Run "Cloning pcrecomp" @('git', 'clone', '--depth', '1', '-b', 'pcrecomp-bcc55', 'https://github.com/sp00nznet/pcrecomp', $Toolkit)
}
Say "  pcrecomp: $Toolkit"

# Visual Studio 2022 or its Build Tools, with the C++ x86 compiler; CMake and
# Ninja ship inside that workload, so build.cmd needs nothing else.
function Find-VS {
  foreach ($p in @("${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
                   "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe")) {
    if (Test-Path $p) {
      $vs = & $p -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
      if ($vs) { return $vs }
    }
  }
  foreach ($d in (Get-ChildItem "$env:ProgramFiles\Microsoft Visual Studio\2022", "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022" -Directory -ErrorAction SilentlyContinue)) {
    if (Test-Path "$($d.FullName)\VC\Auxiliary\Build\vcvarsall.bat") { return $d.FullName }
  }
  return $null
}
if (-not (Find-VS)) {
  Say "  Visual Studio 2022 with the C++ tools is not installed. The recompiled tools are C, and this is the compiler."
  if (-not (Ask "  Install the Visual Studio 2022 Build Tools with the C++ workload now (winget, about 3 GB)?")) { Fail "a C compiler (Visual Studio 2022 C++) is required." }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { Fail "winget is missing. Install Visual Studio 2022 Build Tools with 'Desktop development with C++' yourself." }
  Exec @('winget', 'install', '-e', '--id', 'Microsoft.VisualStudio.2022.BuildTools', '--accept-package-agreements', '--accept-source-agreements',
         '--override', '--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended') | Out-Null
  if (-not (Find-VS)) { Fail "the Build Tools did not install. Install Visual Studio 2022 with 'Desktop development with C++' yourself." }
}
Say "  C compiler: $(Find-VS)"

# ---------------------------------------------------------------- the program
Step 2 "Copying your Borland C++ 5.5 into game\ and work\"
if ((Test-Path 'game\Bin\bcc32.exe') -and (Test-Path 'work\bcc32.exe') -and (Test-Path 'work\ilink32.exe') -and -not $Force) {
  Say "  Already there (skipping)."
} else {
  $Game = $Game.Trim('"', ' ')
  if ($Game -and (Test-Path $Game -PathType Leaf)) { $Game = Split-Path -Parent $Game }
  if ($Game -and (Split-Path -Leaf $Game) -eq 'Bin') { $Game = Split-Path -Parent $Game }
  if (-not $Game) {
    # Where its installer puts it by default.
    foreach ($d in @('C:\Borland\BCC55', (Join-Path $Root 'game'))) {
      if (Test-Path (Join-Path $d 'Bin\bcc32.exe')) { $Game = $d; Say "  Found Borland C++ 5.5 in $Game"; break }
    }
  }
  if (-not $Game -and $Yes) { Fail "Borland C++ 5.5 wasn't found. Give its BCC55 folder with -GameDir." }
  while (-not $Game) {
    $Game = (Read-Host "  Where is Borland C++ 5.5? The BCC55 folder holding Bin\bcc32.exe").Trim('"', ' ')
  }
  if (-not (Test-Path (Join-Path $Game 'Bin\bcc32.exe'))) { Fail "there is no Bin\bcc32.exe in $Game. Give the BCC55 folder." }
  New-Item -ItemType Directory -Force 'game', 'work' | Out-Null
  if ((Resolve-Path $Game).Path -ne (Resolve-Path 'game').Path) { Copy-Item (Join-Path $Game '*') 'game' -Recurse -Force }
  Copy-Item 'game\Bin\bcc32.exe', 'game\Bin\ilink32.exe' 'work' -Force
  Say "  Copied to game\ and work\."
}

# ---------------------------------------------------------------- catalog
Step 3 "Finding every function (about a minute)"
foreach ($t in 'bcc32', 'ilink32') {
  if ((Test-Path "work\$t.functions.json") -and -not $Force) { Say "  $t already done (skipping)."; continue }
  Run "Disassembling $t.exe" ($pyargs + @((Join-Path $Toolkit 'tools\disasm\disasm32.py'), "work\$t.exe", '-o', "work\$t.functions.json"))
}

# ---------------------------------------------------------------- lift
Step 4 "Recompiling both tools to C (about a minute)"
foreach ($t in 'bcc32', 'ilink32') {
  if ((Test-Path "src\recomp\$t\recomp_dispatch.c") -and -not $Force) { Say "  $t already done (skipping)."; continue }
  Run "Lifting $t" ($pyargs + @('run_lift.py', $t))
}

# ---------------------------------------------------------------- build
Step 5 "Building build\bcc32recomp.exe and build\ilink32recomp.exe (about 3 minutes)"
if ((Test-Path 'build\bcc32recomp.exe') -and (Test-Path 'build\ilink32recomp.exe') -and -not $Force) { Say "  Already built (skipping)." }
else {
  $env:CMAKE_ARGS = "-DPCRECOMP=$($Toolkit -replace '\\', '/')"
  Run "Compiling" @('cmd', '/d', '/c', "`"$Root\build.cmd`"")
}

# ---------------------------------------------------------------- harness
Step 6 "Checking the recompiled tools against the originals"
Run "Running tools\bcc_diff.py" ($pyargs + @('tools\bcc_diff.py'))
Say "  Every test program compiled and linked to the same bytes, and runs."

# ---------------------------------------------------------------- shortcut
Step 7 "Making the shortcut"
$lnk = Join-Path $Root 'BCC55 Recomp Prompt.lnk'
$sh = New-Object -ComObject WScript.Shell
$s = $sh.CreateShortcut($lnk)
$s.TargetPath = "$env:SystemRoot\System32\cmd.exe"
$s.Arguments = "/k `"set PATH=$Root\build;%PATH%&& set BCC55_BIN=$Root\game\Bin&& echo bcc32recomp and ilink32recomp are on PATH`""
$s.WorkingDirectory = $Root
$s.Save()
Say "  $lnk"

if (-not (Test-Path $lnk)) { Fail "the shortcut was not made." }
Write-Host ""
Say "Done. 'BCC55 Recomp Prompt' in this folder opens a prompt with bcc32recomp and ilink32recomp on PATH." 'Green'
Pause-End
exit 0
