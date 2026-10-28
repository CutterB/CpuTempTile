CpuTempTile
===========

Adds a live CPU temperature to Windows Task Manager (Performance tab):
  * a "Temperature:" tile on the CPU page (right of "Base speed")
  * the temperature appended to the CPU entry in the Performance sidebar
    (e.g. "5.47 GHz (44 C)")

Requirements
------------
* Windows 11 (x64) - tested on build 10.0.26100. Windows Update may change
  Task Manager internals and require an updated DLL.
* Core Temp (https://www.alcpu.com/CoreTemp/) - supplies the temperature.
  It must be installed and able to run (its driver reads the CPU sensor).

Install (one time)
------------------
1. Right-click install.bat -> "Run as administrator"
   (or double-click and approve the UAC prompt).
   This:
     - copies the program to %LOCALAPPDATA%\CpuTempTile
     - adds a Windows Defender exclusion for that folder
       (required, because the injector is flagged as suspicious)
     - REPLACES the Start Menu "Task Manager" shortcut (System Tools) so that
       opening "Task Manager" now launches it with the CPU temperature
       (the original shortcut is backed up and restored on uninstall)
     - adds a Desktop shortcut named "CpuTempTile"

Use
---
* Open "Task Manager" from the Start Menu as usual (or use the Desktop
  "CpuTempTile" shortcut). It starts Core Temp, opens Task Manager, and
  injects the temperature.
* A UAC prompt may appear if Task Manager is running elevated.

Uninstall
---------
* Run uninstall.ps1 as administrator. It restores the original Task Manager
  shortcut, removes the Defender exclusion, and deletes the program files.

How it works
------------
CpuTempInject.exe loads CpuTempTile.dll into Taskmgr.exe. The DLL hooks
DirectUI (dui70.dll), clones a tile element into the CPU page, and feeds it
from Core Temp's shared memory (CoreTempMappingObjectEx).

This is a hobby customization of a signed Microsoft binary done via runtime
injection; it does not modify Taskmgr.exe itself.
