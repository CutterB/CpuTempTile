# CpuTempTile

Adds a live **CPU temperature** to Windows Task Manager's Performance tab — a
dedicated `Temperature:` tile on the CPU page, plus the temperature appended to
the CPU entry in the sidebar (e.g. `5.47 GHz (44 °C)`).

| CPU page tile | Sidebar |
|---|---|
| `Temperature:` `44 °C` | `5.47 GHz (44 °C)` |

Temperature data comes from **Core Temp** (an AMD/Intel sensor reader). The
tile is a real DirectUI element, so it scales and reflows natively with the
window — it is not an overlay.

## How it works

Task Manager's Performance tab is rendered by **DirectUI** (`dui70.dll`).
`CpuTempInject.exe` loads `CpuTempTile.dll` into `Taskmgr.exe`; the DLL hooks
`DirectUI::Element::SetContentString`, clones an element into the CPU page's
tile row, and feeds it from Core Temp's shared memory
(`CoreTempMappingObjectEx`). `Taskmgr.exe` itself is **never modified**.

The GPU temperature tile already exists in Task Manager; this mirrors that
mechanism for the CPU, which otherwise has no temperature display.

## Requirements

* Windows 11 (x64) — tested on build `10.0.26100`. Task Manager internals are
  version-specific, so a Windows Update may require a rebuilt DLL.
* [Core Temp](https://www.alcpu.com/CoreTemp/) — supplies the temperature
  readings (its driver reads the CPU sensor).

## Install (prebuilt)

Download the latest `CpuTempTile.zip` from
[Releases](../../releases), extract it, then:

1. Right-click `install.bat` → **Run as administrator** (approve UAC).
2. Open **Task Manager** from the Start Menu as usual.

The installer:
* copies the program to `%LOCALAPPDATA%\CpuTempTile`
* adds a Windows Defender exclusion for that folder (required — the injector
  is flagged as suspicious, as all process-injection tools are)
* replaces the Start Menu **Task Manager** shortcut so it launches with the
  temperature (the original is backed up and restored on uninstall)
* configures Core Temp to start minimized to the tray

Uninstall with `uninstall.ps1`.

## Build from source

Requirements: MinGW-w64 (x86_64) `gcc`.

```
powershell -ExecutionPolicy Bypass -File build.ps1
```

This produces `package\CpuTempTile.dll` and `package\CpuTempInject.exe`.

## Reverse-engineering summary

The interesting parts of the RE (done with Ghidra):

* The Performance tab is DirectUI, not XAML.
* GPU temperature is set via
  `FindDescendent(container, StrToID("gpuTemperatureValue"))` + `SetContentString`.
* The CPU detail tiles are base `Element` objects styled by a `Class` property
  (`dynamicData`/`staticData`), laid out by a `FillLayout` cell.
* `Element::Create(dType, ...)` (dui70.dll export) clones an element; the tile
  value is then styled with `SetClass`, `SetForegroundColor`, `SetFontSize`,
  and positioned with `SetPadding`.

## Disclaimer

This is a hobby customization of a signed Microsoft binary done via runtime
injection. It is not affiliated with Microsoft. Use at your own risk.

## License

MIT — see [LICENSE](LICENSE).
