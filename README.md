# StealthMaker

Window Protector - Screen Share Blocker. Protects selected application windows from screen capture, screenshots, and screen sharing.

## Requirements

- Windows 10 version 2004 (May 2020) or later
- Visual Studio 2022 with "Desktop development with C++"
- Run as Administrator

## Build

### Using CMake

```cmd
cd e:\Project\StealthMaker
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

Output: `build\Release\StealthMaker.exe` and `build\Release\InjectDll.dll`

### Using Visual Studio

1. Open Visual Studio 2022
2. File → Open → CMake → Select `CMakeLists.txt` in project root
3. Build → Build All (or Ctrl+Shift+B)

The executable will be in `build/Release/StealthMaker.exe` (or `build/StealthMaker/Release/`). The `InjectDll.dll` is copied automatically next to the exe.

## Usage

1. Run StealthMaker **as Administrator**
2. Click **Add** to enter select mode
3. Click an app in the list to add and protect it
4. Press **Escape** to cancel select mode
5. Use **Active/Deactive** to toggle protection
6. Right-click configured app for **Delete** or **More** (path, Start)
7. Double-click tray icon to show main window
8. Right-click tray icon for app menu and Exit

## Features

- Screen capture protection via `WDA_EXCLUDEFROMCAPTURE`
- Red dotted border around protected windows
- Tray icon with app list
- Auto-monitor border position
- Run at system startup
- Registry persistence
