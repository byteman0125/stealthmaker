#include "Common.h"
#include "MainWindow.h"
#include "TrayIcon.h"
#include <commctrl.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static HWND g_hMainWnd = nullptr;
static HWND g_hLblApps = nullptr;
static HWND g_hLblConfig = nullptr;
static HWND g_hListApps = nullptr;
static HWND g_hListConfig = nullptr;
static HWND g_hBtnAdd = nullptr;
static HWND g_hBtnRefresh = nullptr;
static HWND g_hChkAutoMonitor = nullptr;
static HWND g_hChkStartup = nullptr;
static HWND g_hChkProtection = nullptr;
static HWND g_hStatus = nullptr;

static std::vector<ConfigEntry> g_config;
static bool g_autoMonitor = true;
static bool g_runAtStartup = false;
static bool g_captureProtection = true;
static bool g_selectMode = false;
static UINT_PTR g_timerMonitor = 0;
static UINT_PTR g_timerRunning = 0;

static void UpdateConfigList(HWND hwnd);
static void UpdateAppList(HWND hwnd);
static void ApplyProtection(int index);
static void RemoveProtection(int index);
static void StartApp(int index);

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icex);
        g_hLblApps = CreateWindowW(L"STATIC", L"Protected Apps", WS_CHILD | WS_VISIBLE,
            10, 8, 400, 18, hwnd, (HMENU)IDC_LBL_APPS, GetModuleHandle(nullptr), nullptr);
        g_hListApps = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            10, 28, 600, 105, hwnd, (HMENU)IDC_LIST_APPS, GetModuleHandle(nullptr), nullptr);
        g_hBtnAdd = CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 140, 80, 25, hwnd, (HMENU)IDC_BTN_ADD, GetModuleHandle(nullptr), nullptr);
        g_hBtnRefresh = CreateWindowW(L"BUTTON", L"Refresh List", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, 140, 90, 25, hwnd, (HMENU)IDC_BTN_REFRESH, GetModuleHandle(nullptr), nullptr);
        g_hChkAutoMonitor = CreateWindowW(L"BUTTON", L"Auto-monitor protected windows", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            200, 140, 220, 25, hwnd, (HMENU)IDC_CHK_AUTOMONITOR, GetModuleHandle(nullptr), nullptr);
        g_hChkProtection = CreateWindowW(L"BUTTON", L"Hide from capture", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            430, 140, 110, 25, hwnd, (HMENU)IDC_CHK_PROTECTION, GetModuleHandle(nullptr), nullptr);
        g_hChkStartup = CreateWindowW(L"BUTTON", L"Run at system startup", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            550, 140, 160, 25, hwnd, (HMENU)IDC_CHK_STARTUP, GetModuleHandle(nullptr), nullptr);
        g_hLblConfig = CreateWindowW(L"STATIC", L"Configured Apps", WS_CHILD | WS_VISIBLE,
            10, 150, 400, 18, hwnd, (HMENU)IDC_LBL_CONFIG, GetModuleHandle(nullptr), nullptr);
        g_hListConfig = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LVS_REPORT | LVS_SINGLESEL,
            10, 170, 600, 275, hwnd, (HMENU)IDC_LIST_CONFIG, GetModuleHandle(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(g_hListConfig, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(g_hListConfig, RGB(255, 255, 255));
        ListView_SetTextBkColor(g_hListConfig, RGB(255, 255, 255));
        ListView_SetTextColor(g_hListConfig, RGB(0, 0, 0));
        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT;
        lvc.cx = 150; lvc.pszText = (LPWSTR)L"Name"; ListView_InsertColumn(g_hListConfig, 0, &lvc);
        lvc.cx = 70; lvc.pszText = (LPWSTR)L"PID"; ListView_InsertColumn(g_hListConfig, 1, &lvc);
        lvc.cx = 70; lvc.pszText = (LPWSTR)L"HWND"; ListView_InsertColumn(g_hListConfig, 2, &lvc);
        lvc.cx = 95; lvc.pszText = (LPWSTR)L"Status"; ListView_InsertColumn(g_hListConfig, 3, &lvc);
        lvc.cx = 80; lvc.pszText = (LPWSTR)L"Active"; ListView_InsertColumn(g_hListConfig, 4, &lvc);
        g_hStatus = CreateWindowW(L"STATIC", L"Ready", WS_CHILD | WS_VISIBLE,
            10, 455, 600, 25, hwnd, (HMENU)IDC_STATUSBAR, GetModuleHandle(nullptr), nullptr);
        SetWindowTheme(g_hLblApps, L"", L"");
        SetWindowTheme(g_hLblConfig, L"", L"");
        SetWindowTheme(g_hListApps, L"", L"");
        SetWindowTheme(g_hListConfig, L"", L"");
        SetWindowTheme(g_hBtnAdd, L"", L"");
        SetWindowTheme(g_hBtnRefresh, L"", L"");
        SetWindowTheme(g_hChkAutoMonitor, L"", L"");
        SetWindowTheme(g_hChkProtection, L"", L"");
        SetWindowTheme(g_hChkStartup, L"", L"");
        SetWindowTheme(g_hStatus, L"", L"");
        SetWindowTheme(hwnd, L"", L"");
        CheckDlgButton(hwnd, IDC_CHK_AUTOMONITOR, g_autoMonitor ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHK_PROTECTION, g_captureProtection ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHK_STARTUP, g_runAtStartup ? BST_CHECKED : BST_UNCHECKED);
        LoadConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
        UpdateConfigList(hwnd);
        UpdateAppList(hwnd);
        for (size_t i = 0; i < g_config.size(); i++) {
            if (g_config[i].active && g_config[i].processId && g_captureProtection) {
                ApplyProtection((int)i);
            }
        }
        CreateTrayIcon(hwnd);
        g_timerMonitor = SetTimer(hwnd, 1, 1000, nullptr);
        g_timerRunning = SetTimer(hwnd, 2, 10000, nullptr);
        break;
    }
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            ShowWindow(hwnd, SW_HIDE);
        } else if (wp != SIZE_MINIMIZED && LOWORD(lp) > 0 && HIWORD(lp) > 0) {
            int cx = LOWORD(lp), cy = HIWORD(lp);
            int listW = cx - 20;
            SetWindowPos(g_hLblApps, nullptr, 10, 8, listW, 18, SWP_NOZORDER);
            SetWindowPos(g_hListApps, nullptr, 10, 28, listW, 105, SWP_NOZORDER);
            SetWindowPos(g_hBtnAdd, nullptr, 10, 140, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetWindowPos(g_hBtnRefresh, nullptr, 100, 140, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetWindowPos(g_hChkAutoMonitor, nullptr, 200, 140, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetWindowPos(g_hChkProtection, nullptr, 430, 140, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetWindowPos(g_hChkStartup, nullptr, 550, 140, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetWindowPos(g_hLblConfig, nullptr, 10, 150, listW, 18, SWP_NOZORDER);
            SetWindowPos(g_hListConfig, nullptr, 10, 170, listW, cy - 210, SWP_NOZORDER);
            SetWindowPos(g_hStatus, nullptr, 10, cy - 35, listW, 25, SWP_NOZORDER);
            ListView_SetColumnWidth(g_hListConfig, 0, (listW > 305) ? (listW - 305) : 100);
            ListView_SetColumnWidth(g_hListConfig, 1, 70);
            ListView_SetColumnWidth(g_hListConfig, 2, 80);
            ListView_SetColumnWidth(g_hListConfig, 3, 95);
            ListView_SetColumnWidth(g_hListConfig, 4, 60);
        }
        break;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, g_timerMonitor);
        KillTimer(hwnd, g_timerRunning);
        DestroyTrayIcon();
        DestroyAllBorderOverlays();
        SaveConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
        PostQuitMessage(0);
        break;
    case WM_TRAYICON:
        if (lp == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        } else if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShowTrayContextMenu(hwnd, g_config, pt.x, pt.y);
        }
        break;
    case WM_TIMER:
        if (wp == 1 && g_autoMonitor) UpdateAllBorderOverlays();
        if (wp == 2) {
            for (size_t i = 0; i < g_config.size(); i++) {
                if (g_config[i].processId) {
                    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_config[i].processId);
                    g_config[i].isRunning = (h != nullptr);
                    if (h) CloseHandle(h);
                    if (!g_config[i].isRunning) {
                        g_config[i].processId = 0;
                        g_config[i].mainHwnd = nullptr;
                        g_config[i].isProtected = false;
                    }
                } else {
                    DWORD pid = FindProcessByPath(g_config[i].processPath);
                    if (pid) {
                        g_config[i].processId = pid;
                        g_config[i].isRunning = true;
                        auto wins = GetProcessWindows(pid);
                        if (!wins.empty()) g_config[i].mainHwnd = wins[0];
                        if (g_config[i].active) ApplyProtection((int)i);
                    }
                }
            }
            UpdateConfigList(hwnd);
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == IDC_BTN_ADD) {
            g_selectMode = true;
            SetWindowTextW(g_hBtnAdd, L"Select an app...");
            UpdateAppList(hwnd);
        } else if (id == IDC_BTN_REFRESH) {
            UpdateAppList(hwnd);
            UpdateConfigList(hwnd);
        } else if (id == IDC_CHK_AUTOMONITOR) {
            g_autoMonitor = (IsDlgButtonChecked(hwnd, IDC_CHK_AUTOMONITOR) == BST_CHECKED);
        } else if (id == IDC_CHK_PROTECTION) {
            g_captureProtection = (IsDlgButtonChecked(hwnd, IDC_CHK_PROTECTION) == BST_CHECKED);
            if (!g_captureProtection) {
                for (size_t i = 0; i < g_config.size(); i++) {
                    if (g_config[i].isProtected) RemoveProtection((int)i);
                }
            } else {
                for (size_t i = 0; i < g_config.size(); i++) {
                    if (g_config[i].active && g_config[i].processId) ApplyProtection((int)i);
                }
            }
            SaveConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
            UpdateConfigList(hwnd);
        } else if (id == IDC_CHK_STARTUP) {
            g_runAtStartup = (IsDlgButtonChecked(hwnd, IDC_CHK_STARTUP) == BST_CHECKED);
            SaveConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
        } else if (id == IDC_LIST_APPS && HIWORD(wp) == LBN_SELCHANGE && g_selectMode) {
            int sel = (int)SendMessageW(g_hListApps, LB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                auto list = EnumVisibleWindows();
                if (sel < (int)list.size()) {
                    const auto& wi = list[sel];
                    bool duplicate = false;
                    for (const auto& c : g_config) {
                        if (_wcsicmp(c.processPath.c_str(), wi.processPath.c_str()) == 0) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        ConfigEntry e;
                        e.processPath = wi.processPath;
                        e.filePath = wi.processPath;
                        e.name = wi.title;
                        e.active = true;
                        e.processId = wi.processId;
                        e.mainHwnd = wi.hwnd;
                        e.isRunning = true;
                        e.isProtected = false;
                        g_config.push_back(e);
                        ApplyProtection((int)g_config.size() - 1);
                        SaveConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
                    }
                }
                g_selectMode = false;
                SetWindowTextW(g_hBtnAdd, L"Add");
                UpdateAppList(hwnd);
                UpdateConfigList(hwnd);
            }
        } else if (id == ID_TRAY_EXIT) {
            DestroyWindow(hwnd);
        } else if (id >= ID_TRAY_APP_BASE && id < ID_TRAY_APP_BASE + 1000) {
            int idx = (id - ID_TRAY_APP_BASE) / 10;
            int action = (id - ID_TRAY_APP_BASE) % 10;
            if (idx >= 0 && idx < (int)g_config.size()) {
                if (action == 1) {
                    g_config[idx].active = !g_config[idx].active;
                    if (g_config[idx].active) ApplyProtection(idx);
                    else RemoveProtection(idx);
                } else if (action == 2) {
                    StartApp(idx);
                }
            }
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(240, 240, 240));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)GetStockObject(LTGRAY_BRUSH);
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE && g_selectMode) {
            g_selectMode = false;
            SetWindowTextW(g_hBtnAdd, L"Add");
            UpdateAppList(hwnd);
        }
        break;
    case WM_NOTIFY: {
        auto* nm = (LPNMHDR)lp;
        if (nm->idFrom == IDC_LIST_CONFIG) {
            if (nm->code == NM_CUSTOMDRAW) {
                auto* lvcd = (LPNMLVCUSTOMDRAW)lp;
                if (lvcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (lvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    int idx = (int)lvcd->nmcd.dwItemSpec;
                    if (idx >= 0 && idx < (int)g_config.size()) {
                        const auto& e = g_config[idx];
                        if (!e.isRunning) {
                            lvcd->clrText = RGB(128, 128, 128);
                            lvcd->clrTextBk = RGB(245, 245, 245);
                        } else if (e.isProtected) {
                            lvcd->clrText = RGB(0, 70, 130);
                            lvcd->clrTextBk = RGB(220, 235, 255);
                        } else {
                            lvcd->clrText = RGB(0, 80, 0);
                            lvcd->clrTextBk = RGB(225, 255, 225);
                        }
                    }
                    return CDRF_DODEFAULT;
                }
                return CDRF_DODEFAULT;
            }
            if (nm->code == NM_DBLCLK) {
                int sel = ListView_GetNextItem(g_hListConfig, -1, LVNI_SELECTED);
                if (sel >= 0 && sel < (int)g_config.size()) {
                    g_config[sel].active = !g_config[sel].active;
                    if (g_config[sel].active) ApplyProtection(sel);
                    else RemoveProtection(sel);
                    UpdateConfigList(hwnd);
                }
            } else if (nm->code == NM_RCLICK) {
                int sel = ListView_GetNextItem(g_hListConfig, -1, LVNI_SELECTED);
                if (sel >= 0 && sel < (int)g_config.size()) {
                    HMENU ctx = CreatePopupMenu();
                    if (!g_config[sel].isRunning) {
                        AppendMenuW(ctx, MF_STRING, IDC_CTX_START, L"Start");
                    }
                    AppendMenuW(ctx, MF_STRING, IDC_CTX_DELETE, L"Delete");
                    AppendMenuW(ctx, MF_STRING, IDC_CTX_MORE, L"More...");
                    POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);
                    UINT cmd = TrackPopupMenu(ctx, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
                    DestroyMenu(ctx);
                    if (cmd == IDC_CTX_START) {
                        StartApp(sel);
                    } else if (cmd == IDC_CTX_DELETE) {
                        RemoveProtection(sel);
                        g_config.erase(g_config.begin() + sel);
                        SaveConfig(g_config, g_autoMonitor, g_runAtStartup, g_captureProtection);
                        UpdateConfigList(hwnd);
                    } else if (cmd == IDC_CTX_MORE) {
                        if (g_config[sel].isRunning) {
                            MessageBoxW(hwnd, g_config[sel].filePath.c_str(), L"File Path", MB_OK);
                        } else {
                            StartApp(sel);
                        }
                    }
                }
            }
        }
        break;
    }
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

static void UpdateAppList(HWND hwnd) {
    SetWindowTextW(g_hLblApps, g_selectMode ? L"Running Apps (Select one to add)" : L"Protected Apps");
    SendMessageW(g_hListApps, LB_RESETCONTENT, 0, 0);
    if (g_selectMode) {
        auto list = EnumVisibleWindows();
        for (const auto& wi : list) {
            std::wstring s = L"[" + std::to_wstring(wi.processId) + L"] " + wi.title;
            SendMessageW(g_hListApps, LB_ADDSTRING, 0, (LPARAM)s.c_str());
        }
        SetWindowTextW(g_hStatus, (L"Select mode: Choose an app to add (" + std::to_wstring(list.size()) + L" apps)").c_str());
    } else {
        auto list = EnumProtectedWindows();
        for (const auto& wi : list) {
            std::wstring s = L"[" + std::to_wstring(wi.processId) + L"] " + wi.title;
            SendMessageW(g_hListApps, LB_ADDSTRING, 0, (LPARAM)s.c_str());
        }
        SetWindowTextW(g_hStatus, (L"Protected: " + std::to_wstring(list.size()) + L" applications").c_str());
    }
}

static void UpdateConfigList(HWND hwnd) {
    ListView_DeleteAllItems(g_hListConfig);
    wchar_t buf[64];
    for (size_t i = 0; i < g_config.size(); i++) {
        const auto& e = g_config[i];
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.lParam = i;
        lvi.pszText = (LPWSTR)e.name.c_str();
        ListView_InsertItem(g_hListConfig, &lvi);
        wcscpy_s(buf, e.processId ? std::to_wstring(e.processId).c_str() : L"N/A");
        ListView_SetItemText(g_hListConfig, i, 1, buf);
        swprintf_s(buf, L"%p", (void*)e.mainHwnd);
        ListView_SetItemText(g_hListConfig, i, 2, e.mainHwnd ? buf : (LPWSTR)L"N/A");
        wcscpy_s(buf, e.isRunning ? (e.isProtected ? L"Protected" : L"Running") : L"Not running");
        ListView_SetItemText(g_hListConfig, i, 3, buf);
        wcscpy_s(buf, e.active ? L"Active" : L"Inactive");
        ListView_SetItemText(g_hListConfig, i, 4, buf);
    }
}

static void ApplyProtection(int index) {
    if (index < 0 || index >= (int)g_config.size()) return;
    auto& e = g_config[index];
    if (!e.processId) return;
    if (!g_captureProtection) return;
    if (InjectAndProtect(e.processId)) {
        e.isProtected = true;
        auto windows = GetProcessWindows(e.processId);
        for (HWND hwnd : windows) {
            CreateBorderOverlay(hwnd);
        }
        if (!windows.empty()) e.mainHwnd = windows[0];
    }
}

static void RemoveProtection(int index) {
    if (index < 0 || index >= (int)g_config.size()) return;
    auto& e = g_config[index];
    if (e.processId) {
        InjectAndUnprotect(e.processId);
        auto windows = GetProcessWindows(e.processId);
        for (HWND hwnd : windows) {
            DestroyBorderOverlay(hwnd);
        }
    }
    e.isProtected = false;
}

static void StartApp(int index) {
    if (index < 0 || index >= (int)g_config.size()) return;
    const auto& e = g_config[index];
    if (e.isRunning) return;
    ShellExecuteW(nullptr, L"open", e.filePath.c_str(), nullptr, nullptr, SW_SHOW);
}

HWND CreateMainWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"StealthMaker";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);
    g_hMainWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"StealthMaker", L"StealthMaker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 560,
        nullptr, nullptr, hInstance, nullptr);
    if (g_hMainWnd) {
        BOOL useLight = FALSE;
        DwmSetWindowAttribute(g_hMainWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useLight, sizeof(useLight));
        RECT rc;
        GetWindowRect(g_hMainWnd, &rc);
        int w = 620, h = 560;
        int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
        SetWindowPos(g_hMainWnd, nullptr, x, y, w, h, SWP_NOZORDER);
    }
    return g_hMainWnd;
}

void RefreshMainWindow(HWND hwnd) {
    UpdateAppList(hwnd);
    UpdateConfigList(hwnd);
}
