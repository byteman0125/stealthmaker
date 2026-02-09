#include <windows.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    (void)hModule;
    (void)lpReserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
