$ErrorActionPreference = "Stop"

$installDir = Join-Path $env:LOCALAPPDATA "CpuTempTile"

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Requesting administrator privileges..."
    Start-Process powershell -Verb RunAs -ArgumentList @("-NoProfile","-ExecutionPolicy","Bypass","-File",$PSCommandPath)
    exit
}

Write-Host "Removing CpuTempTile..."

# Restore the original Task Manager shortcut
$tmShortcut = "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\System Tools\Task Manager.lnk"
$orig = Join-Path $installDir "Task Manager.lnk.orig"
if (Test-Path $orig) {
    Copy-Item $orig $tmShortcut -Force
    Write-Host "Restored the original Task Manager shortcut."
} else {
    Write-Host "No original shortcut backup found."
}

# Restore the original Core Temp settings (minimized)
$coreTempIni = "C:\Program Files\Core Temp\CoreTemp.ini"
$coreTempOrig = Join-Path $installDir "CoreTemp.ini.orig"
if (Test-Path $coreTempOrig) {
    Copy-Item $coreTempOrig $coreTempIni -Force
    Write-Host "Restored Core Temp settings."
}

# Remove Defender exclusion
try { Remove-MpPreference -ExclusionPath $installDir -ErrorAction SilentlyContinue } catch {}

# Remove Desktop shortcut
$desktop = Join-Path ([Environment]::GetFolderPath("Desktop")) "CpuTempTile.lnk"
if (Test-Path $desktop) { Remove-Item $desktop -Force }

# Remove install folder
if (Test-Path $installDir) { Remove-Item $installDir -Recurse -Force }

Write-Host "Uninstall complete."
