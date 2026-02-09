#include "Common.h"
#include <vector>

static std::unordered_map<HWND, HWND> g_borderMap;
static HWND g_overlayClass = nullptr;

static LRESULT CALLBACK BorderWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HPEN pen = CreatePen(PS_DOT, 4, RGB(255, 0, 0));
        HGDIOBJ old = SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, old);
        DeleteObject(pen);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0));
        DrawTextW(hdc, L"\U0001F6E1 PROTECTED", -1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void CreateBorderOverlays() {
    if (g_overlayClass) return;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = BorderWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"StealthMakerBorder";
    RegisterClassExW(&wc);
    g_overlayClass = (HWND)1;
}

HWND CreateBorderOverlay(HWND targetHwnd) {
    CreateBorderOverlays();
    RECT rc;
    GetWindowRect(targetHwnd, &rc);
    int w = rc.right - rc.left + 8;
    int h = rc.bottom - rc.top + 8;
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"StealthMakerBorder",
        nullptr,
        WS_POPUP,
        rc.left - 4, rc.top - 4, w, h,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (hwnd) {
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        g_borderMap[targetHwnd] = hwnd;
    }
    return hwnd;
}

void UpdateBorderOverlay(HWND targetHwnd) {
    auto it = g_borderMap.find(targetHwnd);
    if (it == g_borderMap.end()) return;
    if (!IsWindow(targetHwnd)) {
        DestroyWindow(it->second);
        g_borderMap.erase(it);
        return;
    }
    RECT rc;
    GetWindowRect(targetHwnd, &rc);
    SetWindowPos(it->second, HWND_TOPMOST, rc.left - 4, rc.top - 4, rc.right - rc.left + 8, rc.bottom - rc.top + 8, SWP_NOACTIVATE);
}

void DestroyBorderOverlay(HWND targetHwnd) {
    auto it = g_borderMap.find(targetHwnd);
    if (it != g_borderMap.end()) {
        DestroyWindow(it->second);
        g_borderMap.erase(it);
    }
}

void DestroyAllBorderOverlays() {
    for (auto& p : g_borderMap) {
        DestroyWindow(p.second);
    }
    g_borderMap.clear();
}

void UpdateAllBorderOverlays() {
    std::vector<HWND> toRemove;
    for (auto& p : g_borderMap) {
        if (!IsWindow(p.first)) {
            toRemove.push_back(p.first);
        } else {
            UpdateBorderOverlay(p.first);
        }
    }
    for (HWND h : toRemove) {
        DestroyBorderOverlay(h);
    }
}
