# LiveExplorer_Skin

LiveExplorer_Skin is a Windows utility that overlays a customizable image on File Explorer to create a personalized desktop experience.

## Features
- Qt-based tray interface for configuration
- Transparent overlay window with click-through behavior
- Multiple image support with opacity and scaling controls
- Optional startup and switch sound effects
- Compatible with Windows File Explorer

## Build
Use CMake with a Qt-enabled toolchain.

The build script will generate the executable as LiveExplorer_Skin.exe.

```powershell
powershell -ExecutionPolicy Bypass -File .\build_and_run.ps1
```

## Notes
The current implementation uses Qt for the app shell and Win32/GDI+ for the overlay rendering layer.
