# The Windows installer installed and taken away again, on a runner whose service account may not write Program Files:
# a test build (make_installer.ps1 -TestRoot) installed silently for the current user under a folder of the test's own,
# every staged file checked at its place byte for byte, the installed game started there for a launch's check
# (Knect4.exe --smoke: its window opened, a few frames drawn, closed), installed again over itself as an upgrade, and
# the uninstaller leaving nothing of the game's - while two sibling products' folders beside it are left as they were.
#
#   pwsh -File packaging\windows\test_installer.ps1 -DistDir dist -Version 2.0.3 [-SkipLaunch]
#
# -SkipLaunch leaves out the launch's check, for a machine with no window station to draw on. The test build has an id,
# a name and a folder of its own and never reuses a previous install's folder, so it never meets a real install of
# Knect 4 on the machine.
param(
    [Parameter(Mandatory = $true)] [string] $DistDir,
    [Parameter(Mandatory = $true)] [string] $Version,
    [switch] $SkipLaunch
)
$ErrorActionPreference = 'Stop'
$dist = (Resolve-Path $DistDir).Path
$work = Join-Path ([IO.Path]::GetTempPath()) "knect4-installer-test-$PID"
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $work
$root = Join-Path $work 'Reckless Technology'
$app = Join-Path $root 'Knect 4'
$siblings = @{ 'IR Capture\RecklessIRCapture.exe' = 'a sibling product'; 'Amphitheater\Amphitheater.exe' = 'another' }
foreach ($sibling in $siblings.Keys) {
    $path = Join-Path $root $sibling
    New-Item -ItemType Directory -Force -Path (Split-Path $path) | Out-Null
    Set-Content -Path $path -Value $siblings[$sibling]
}
function Fail([string] $why) { throw "test_installer.ps1: $why" }

try {
    & (Join-Path $PSScriptRoot 'make_installer.ps1') -DistDir $dist -Version $Version -OutDir (Join-Path $work 'out') `
        -TestRoot $root -Fetch
    $setup = Join-Path $work "out\knect4-$Version-win64-setup-test.exe"
    function Install() {
        $p = Start-Process -FilePath $setup -Wait -PassThru -ArgumentList @(
            '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', '/NOICONS', "/LOG=`"$work\install.log`"")
        if ($p.ExitCode -ne 0) { Get-Content "$work\install.log" -ErrorAction SilentlyContinue | Write-Host; Fail "the install exited $($p.ExitCode)" }
    }
    Install

    # Every staged file at its place, byte for byte, and nothing else but the uninstaller's own.
    $staged = Get-ChildItem -Recurse -File $dist
    foreach ($file in $staged) {
        $relative = $file.FullName.Substring($dist.Length).TrimStart('\')
        $installed = Join-Path $app $relative
        if (-not (Test-Path $installed)) { Fail "$relative is not installed" }
        if ((Get-FileHash $installed).Hash -ne (Get-FileHash $file.FullName).Hash) { Fail "$relative is not the staged file" }
    }
    $extra = Get-ChildItem -Recurse -File $app | Where-Object { $_.Name -notmatch '^unins\d+\.(exe|dat)$' } |
             Where-Object { -not (Test-Path (Join-Path $dist $_.FullName.Substring($app.Length).TrimStart('\'))) }
    if ($extra) { Fail "files the stage does not hold: $(($extra | ForEach-Object Name) -join ', ')" }
    Write-Host "$($staged.Count) staged files installed under $app"

    # The game runs from where it was installed.
    if (-not $SkipLaunch) {
        $p = Start-Process -FilePath (Join-Path $app 'Knect4.exe') -ArgumentList '--smoke' -Wait -PassThru
        if ($p.ExitCode -ne 0) { Fail "the installed game's launch check exited $($p.ExitCode)" }
        Write-Host "the installed game opened its window and drew its frames"
    }

    # An upgrade: the same release installed over itself, everything still in place.
    Install
    foreach ($file in $staged) {
        if (-not (Test-Path (Join-Path $app $file.FullName.Substring($dist.Length).TrimStart('\')))) { Fail "the upgrade lost $($file.Name)" }
    }

    # The uninstaller takes the game away and leaves the sibling products.
    $uninstaller = Get-ChildItem -File (Join-Path $app 'unins*.exe') | Select-Object -First 1
    if (-not $uninstaller) { Fail "no uninstaller in $app" }
    $p = Start-Process -FilePath $uninstaller.FullName -Wait -PassThru -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
    if ($p.ExitCode -ne 0) { Fail "the uninstaller exited $($p.ExitCode)" }
    # The uninstaller copies itself away and finishes from there: give it a moment to take its own folder.
    $deadline = (Get-Date).AddSeconds(30)
    while ((Test-Path $app) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
    if (Test-Path $app) {
        $left = Get-ChildItem -Recurse $app | ForEach-Object { $_.FullName.Substring($app.Length) }
        Fail "the uninstaller left $app behind: $($left -join ', ')"
    }
    foreach ($sibling in $siblings.Keys) {
        if ((Get-Content (Join-Path $root $sibling)) -ne $siblings[$sibling]) { Fail "the uninstaller touched $sibling" }
    }
    Write-Host "the installer holds the stage at ${Version}: installed, launched, upgraded and uninstalled cleanly, the sibling products untouched"
}
finally {
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $work
}
