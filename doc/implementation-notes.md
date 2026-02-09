# StealthMaker Implementation Notes

## Overview

StealthMaker is a C++/Win32 application that protects selected application windows from screen capture, screenshots, and screen sharing using the Windows `WDA_EXCLUDEFROMCAPTURE` API.

## Architecture

### Key Limitation

`SetWindowDisplayAffinity` can only be called by the process that owns the window. To protect other applications' windows, we use **DLL injection**:

1. **InjectDll.dll** - A small DLL that exports `SetProtectThread` and `SetUnprotectThread`
2. **Injector** - Injects the DLL into the target process via `CreateRemoteThread` + `LoadLibraryW`
3. For each window of the target process, we spawn a remote thread that calls our function with the HWND

### Project Structure

```
StealthMaker/
├── src/
│   ├── main.cpp          - Entry point, single-instance mutex
│   ├── MainWindow.cpp    - Main UI, list controls, message handling
│   ├── WindowUtils.cpp   - Window enumeration, process path, protected check
│   ├── Injector.cpp      - DLL injection logic
│   ├── InjectDll.cpp     - DLL to inject (SetWindowDisplayAffinity calls)
│   ├── BorderOverlay.cpp - Transparent overlay windows for red border
│   ├── ConfigManager.cpp - Registry persistence
│   ├── TrayIcon.cpp      - System tray icon and context menu
│   └── Common.h          - Shared types and declarations
├── app.manifest          - UAC requireAdministrator
├── resource.rc           - Manifest embedding
└── CMakeLists.txt
```

### Components

- **MainWindow**: ListBox (apps), ListView (configured apps), Add/Refresh buttons, Auto-monitor, Hide from capture, and Run-at-startup checkboxes
- **TrayIcon**: Shield icon, double-click to show, right-click for app list with Active/Deactive and Start submenus
- **BorderOverlay**: One transparent WS_EX_LAYERED window per protected window, red dotted border, "PROTECTED" label
- **ConfigManager**: Stores config in `HKEY_CURRENT_USER\Software\StealthMaker\Config`, Run key for startup

### Data Flow

1. **Add**: User clicks Add → select mode → list shows all visible windows (one per process) → user selects → inject DLL → protect all windows of process → add to config
2. **Protection**: InjectDll loads in target → CreateRemoteThread calls SetProtectThread(hwnd) for each window
3. **Borders**: One overlay per window, updated every 1s when auto-monitor is on
4. **Running status**: Poll every 10s, match by process path when processId was 0
5. **All windows protection**: Uses `GetAllProcessWindows` (includes hidden windows) for injection; `GetProcessWindows` (visible only) for border overlays

### Build

Requires CMake and Visual Studio 2022 with C++ desktop workload. Run as Administrator.

## Implemented Features

- [x] Add apps via select mode (single selection)
- [x] Escape to cancel select mode
- [x] Protect all windows of process (DLL injection)
- [x] Red dotted border overlay per window
- [x] Configured apps list with Process ID, HWND, Status, Active
- [x] Active/Deactive toggle (double-click or tray menu)
- [x] Delete via right-click context menu
- [x] More menu (shows path, Start if not running)
- [x] Tray icon with app list, Exit
- [x] Double-click tray to show, hide from taskbar
- [x] Registry persistence
- [x] Run at startup (Registry Run key)
- [x] Auto-apply on startup for active entries
- [x] Single instance mutex
- [x] Running status poll every 10s
- [x] Match by process path when app restarts
- [x] "Hide from capture" checkbox - when OFF (default), apps stay visible (no dark window); when ON, apply WDA_EXCLUDEFROMCAPTURE
