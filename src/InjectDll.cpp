#include <windows.h>
#include <vector>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif

static volatile LONG g_monitorRunning = 0;

struct EnumData {
    DWORD pid;
    std::vector<HWND> windows;
};

static BOOL CALLBACK EnumAllWindowsProc(HWND hwnd, LPARAM lp) {
    auto* d = (EnumData*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == d->pid && !GetWindow(hwnd, GW_OWNER)) {
        d->windows.push_back(hwnd);
    }
    return TRUE;
}

static bool IsWindowProtected(HWND hwnd) {
    DWORD affinity = 0;
    return GetWindowDisplayAffinity(hwnd, &affinity) && affinity == WDA_EXCLUDEFROMCAPTURE;
}

static DWORD WINAPI MonitorThreadProc(LPVOID param) {
    DWORD pid = GetCurrentProcessId();
    wchar_t eventName[64];
    swprintf_s(eventName, L"StealthMaker_Stop_%u", pid);
    HANDLE hStop = CreateEventW(nullptr, TRUE, FALSE, eventName);
    if (!hStop) return 1;

    while (WaitForSingleObject(hStop, 500) == WAIT_TIMEOUT) {
        EnumData ed = {};
        ed.pid = pid;
        EnumWindows(EnumAllWindowsProc, (LPARAM)&ed);
        for (HWND hwnd : ed.windows) {
            if (IsWindow(hwnd) && !IsWindowProtected(hwnd)) {
                SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
            }
        }
    }
    CloseHandle(hStop);
    InterlockedExchange(&g_monitorRunning, 0);
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI SetProtectThread(LPVOID param) {
    HWND hwnd = (HWND)param;
    if (IsWindow(hwnd)) {
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    }
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI SetUnprotectThread(LPVOID param) {
    HWND hwnd = (HWND)param;
    if (IsWindow(hwnd)) {
        SetWindowDisplayAffinity(hwnd, WDA_NONE);
    }
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI StartMonitorThread(LPVOID param) {
    (void)param;
    if (InterlockedCompareExchange(&g_monitorRunning, 1, 0) != 0) return 0;
    HANDLE h = CreateThread(nullptr, 0, MonitorThreadProc, nullptr, 0, nullptr);
    if (h) CloseHandle(h);
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI StopMonitorThread(LPVOID param) {
    (void)param;
    DWORD pid = GetCurrentProcessId();
    wchar_t eventName[64];
    swprintf_s(eventName, L"StealthMaker_Stop_%u", pid);
    HANDLE hStop = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
    if (hStop) {
        SetEvent(hStop);
        CloseHandle(hStop);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    (void)hModule;
    (void)lpReserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
