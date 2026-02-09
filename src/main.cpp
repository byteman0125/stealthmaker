#include "Common.h"
#include "MainWindow.h"
#include <shellapi.h>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static BOOL IsRunAsAdmin() {
    BOOL elevated = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION te = {};
        DWORD size = sizeof(te);
        if (GetTokenInformation(hToken, TokenElevation, &te, size, &size)) {
            elevated = te.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return elevated;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    if (!IsRunAsAdmin()) {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = path;
        sei.lpParameters = lpCmdLine;
        sei.nShow = SW_NORMAL;
        if (ShellExecuteExW(&sei)) return 0;
        MessageBoxW(nullptr, L"StealthMaker requires administrator privileges.", L"StealthMaker", MB_OK | MB_ICONERROR);
        return 1;
    }
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"StealthMaker_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        HWND existing = FindWindowW(L"StealthMaker", L"StealthMaker");
        if (existing) {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        return 0;
    }
    HWND hwnd = CreateMainWindow(hInstance);
    if (!hwnd) return 1;
    ShowWindow(hwnd, SW_SHOW);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (hMutex) CloseHandle(hMutex);
    return (int)msg.wParam;
}
