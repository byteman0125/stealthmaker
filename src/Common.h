#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif

#define WM_TRAYICON (WM_USER + 1)
#define WM_UPDATELIST (WM_USER + 2)
#define WM_RUNNINGSTATUS (WM_USER + 3)

#define IDC_LBL_APPS 1008
#define IDC_LBL_CONFIG 1009
#define IDC_LIST_APPS 1001
#define IDC_LIST_CONFIG 1002
#define IDC_BTN_ADD 1003
#define IDC_BTN_REFRESH 1004
#define IDC_CHK_AUTOMONITOR 1005
#define IDC_CHK_STARTUP 1006
#define IDC_CHK_PROTECTION 1010
#define IDC_STATUSBAR 1007

#define ID_TRAY_EXIT 2001
#define ID_TRAY_SHOW 2002
#define ID_TRAY_APP_BASE 2100
#define IDC_CTX_DELETE 3001
#define IDC_CTX_MORE 3002
#define IDC_CTX_START 3003

struct ConfigEntry {
    std::wstring processPath;
    std::wstring filePath;
    std::wstring name;
    bool active;
    DWORD processId = 0;
    HWND mainHwnd = nullptr;
    bool isRunning = false;
    bool isProtected = false;
};

struct WindowInfo {
    HWND hwnd;
    DWORD processId;
    std::wstring title;
    std::wstring processPath;
};

std::wstring GetWindowTitle(HWND hwnd);
std::wstring GetProcessPath(DWORD processId);
std::vector<WindowInfo> EnumVisibleWindows();
std::vector<WindowInfo> EnumProtectedWindows();
std::vector<HWND> GetProcessWindows(DWORD processId);
std::vector<HWND> GetAllProcessWindows(DWORD processId);
bool IsWindowProtected(HWND hwnd);
DWORD FindProcessByPath(const std::wstring& path);

HWND CreateBorderOverlay(HWND targetHwnd);
void UpdateBorderOverlay(HWND targetHwnd);
void DestroyBorderOverlay(HWND targetHwnd);
void DestroyAllBorderOverlays();
void UpdateAllBorderOverlays();

bool SaveConfig(const std::vector<ConfigEntry>& entries, bool autoMonitor, bool runAtStartup, bool captureProtection);
bool LoadConfig(std::vector<ConfigEntry>& entries, bool& autoMonitor, bool& runAtStartup, bool& captureProtection);

bool InjectAndProtect(DWORD processId);
bool InjectAndUnprotect(DWORD processId);
