$ErrorActionPreference = "Stop"

$srcDir     = Split-Path -Parent $MyInvocation.MyCommand.Path
$installDir = Join-Path $env:LOCALAPPDATA "CpuTempTile"

# The Defender exclusion requires admin; self-elevate if needed.
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Requesting administrator privileges (needed for the Defender exclusion)..."
    Start-Process powershell -Verb RunAs -ArgumentList @("-NoProfile","-ExecutionPolicy","Bypass","-File",$PSCommandPath)
    exit
}

Write-Host "Installing CpuTempTile to: $installDir"

New-Item -ItemType Directory -Force -Path $installDir | Out-Null

foreach ($f in @("CpuTempTile.dll","CpuTempInject.exe","CpuTempTile.bat")) {
    Copy-Item (Join-Path $srcDir $f) $installDir -Force
}

Write-Host "Adding Windows Defender exclusion for the install folder..."
Add-MpPreference -ExclusionPath $installDir

# Configure Core Temp to start minimized (to the tray) so it stays in the background
$coreTempIni = "C:\Program Files\Core Temp\CoreTemp.ini"
if (Test-Path $coreTempIni) {
    Copy-Item $coreTempIni (Join-Path $installDir "CoreTemp.ini.orig") -Force
    $content = Get-Content $coreTempIni -Raw
    $content = $content -replace '(?m)^Minimized=\d+', 'Minimized=1'
    Set-Content -Path $coreTempIni -Value $content -NoNewline
    Write-Host "Configured Core Temp to start minimized."
} else {
    Write-Host "Core Temp not found; skipped its minimize setting."
}

$ws       = New-Object -ComObject WScript.Shell
$launcher = Join-Path $installDir "CpuTempTile.bat"
$icon     = "$env:SystemRoot\System32\Taskmgr.exe,0"

# Replace the real "Task Manager" shortcut in System Tools with CpuTempTile
$tmFolder   = "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\System Tools"
$tmShortcut = Join-Path $tmFolder "Task Manager.lnk"

if (Test-Path $tmShortcut) {
    Copy-Item $tmShortcut (Join-Path $installDir "Task Manager.lnk.orig") -Force
    Write-Host "Backed up the original Task Manager shortcut."
}

$tm = $ws.CreateShortcut($tmShortcut)
$tm.TargetPath       = $launcher
$tm.WorkingDirectory = $installDir
$tm.IconLocation     = $icon
$tm.Description      = "Task Manager with CPU temperature"
$tm.Save()
Write-Host "Replaced 'Task Manager' shortcut with CpuTempTile."

# Desktop shortcut (extra convenience)
$desktop = Join-Path ([Environment]::GetFolderPath("Desktop")) "CpuTempTile.lnk"
$dc = $ws.CreateShortcut($desktop)
$dc.TargetPath       = $launcher
$dc.WorkingDirectory = $installDir
$dc.IconLocation     = $icon
$dc.Description      = "Task Manager with CPU temperature"
$dc.Save()

Write-Host ""
Write-Host "Installation complete."
Write-Host "  * The Start Menu 'Task Manager' entry now launches CpuTempTile."
Write-Host "  * Core Temp is auto-started by the shortcut (it must be installed)."
Write-Host ""
