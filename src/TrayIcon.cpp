#include "Common.h"
#include <shellapi.h>

static NOTIFYICONDATAW g_nid = {};
static bool g_trayCreated = false;

void CreateTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(nullptr, IDI_SHIELD);
    wcscpy_s(g_nid.szTip, L"StealthMaker");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
    g_trayCreated = true;
}

void DestroyTrayIcon() {
    if (g_trayCreated) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_trayCreated = false;
    }
}

void ShowTrayContextMenu(HWND hwnd, const std::vector<ConfigEntry>& entries, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    for (size_t i = 0; i < entries.size(); i++) {
        const auto& e = entries[i];
        std::wstring label = e.name + L" (" + (e.processId ? std::to_wstring(e.processId) : L"N/A") + L")";
        std::wstring prefix = e.isRunning ? (e.isProtected ? L"\x25CF " : L"\x25CB ") : L"\x25E6 ";
        HMENU sub = CreatePopupMenu();
        AppendMenuW(sub, MF_STRING, ID_TRAY_APP_BASE + (int)i * 10 + 1, L"Active/Deactive");
        if (!e.isRunning) AppendMenuW(sub, MF_STRING, ID_TRAY_APP_BASE + (int)i * 10 + 2, L"Start");
        AppendMenuW(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)sub, (prefix + label).c_str());
    }
    if (entries.empty()) AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, L"No apps configured");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}
