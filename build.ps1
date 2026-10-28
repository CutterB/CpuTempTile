$ErrorActionPreference = "Stop"
$root  = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root "build"
New-Item -ItemType Directory -Force -Path $build | Out-Null

$gcc   = "gcc"
$flags = @("-O2", "-Wall", "-DWINVER=0x0601", "-D_WIN32_WINNT=0x0601")

Write-Output "Building CpuTempTile.dll..."
& $gcc -shared `
    (Join-Path $root "src\cputemp_tile.c") `
    (Join-Path $root "src\coretemp.c") `
    -I (Join-Path $root "src") `
    -o (Join-Path $build "CpuTempTile.dll") `
    "-static" $flags "-lkernel32" "-luser32" "-lmsvcrt"
if (-not $?) { throw "CpuTempTile.dll failed" }

Write-Output "Building CpuTempInject.exe..."
& $gcc (Join-Path $root "src\inject.c") `
    -o (Join-Path $build "CpuTempInject.exe") `
    "-municode" "-static" $flags "-luser32" "-lkernel32" "-lshell32"
if (-not $?) { throw "CpuTempInject.exe failed" }

Copy-Item (Join-Path $build "CpuTempTile.dll")    (Join-Path $root "package\CpuTempTile.dll") -Force
Copy-Item (Join-Path $build "CpuTempInject.exe")  (Join-Path $root "package\CpuTempInject.exe") -Force

Write-Output "Done. Binaries in package\"
