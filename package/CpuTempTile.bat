@echo off
rem CpuTempTile launcher - starts Core Temp (minimized) and injects into Task Manager.
if exist "C:\Program Files\Core Temp\Core Temp.exe" (
    start /min "" "C:\Program Files\Core Temp\Core Temp.exe"
)
"%~dp0CpuTempInject.exe"
