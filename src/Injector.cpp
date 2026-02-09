#include "Common.h"
#include <shlwapi.h>
#include <psapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

static HMODULE g_hInjectDll = nullptr;
static FARPROC g_SetProtect = nullptr;
static FARPROC g_SetUnprotect = nullptr;

bool LoadInjectDll() {
    if (g_hInjectDll) return true;
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"InjectDll.dll");
    g_hInjectDll = LoadLibraryW(path);
    if (!g_hInjectDll) return false;
    g_SetProtect = GetProcAddress(g_hInjectDll, "SetProtectThread");
    g_SetUnprotect = GetProcAddress(g_hInjectDll, "SetUnprotectThread");
    return g_SetProtect && g_SetUnprotect;
}

static DWORD_PTR GetFunctionOffset(FARPROC func) {
    if (!g_hInjectDll || !func) return 0;
    return (DWORD_PTR)func - (DWORD_PTR)g_hInjectDll;
}

bool InjectAndProtect(DWORD processId) {
    std::vector<HWND> windows = GetProcessWindows(processId);
    if (windows.empty()) return false;
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return false;
    wchar_t dllPath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, dllPath, MAX_PATH);
    PathRemoveFileSpecW(dllPath);
    PathAppendW(dllPath, L"InjectDll.dll");
    size_t pathLen = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) {
        CloseHandle(hProcess);
        return false;
    }
    if (!WriteProcessMemory(hProcess, remotePath, dllPath, pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, loadLib, remotePath, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    HMODULE hInject = nullptr;
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (DWORD i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
            wchar_t modName[MAX_PATH] = {};
            if (GetModuleFileNameExW(hProcess, hMods[i], modName, MAX_PATH)) {
                if (wcsstr(modName, L"InjectDll.dll")) {
                    hInject = hMods[i];
                    break;
                }
            }
        }
    }
    if (!hInject) {
        CloseHandle(hProcess);
        return false;
    }
    if (!LoadInjectDll()) {
        CloseHandle(hProcess);
        return false;
    }
    DWORD_PTR offset = GetFunctionOffset(g_SetProtect);
    FARPROC remoteProtect = (FARPROC)((DWORD_PTR)hInject + offset);
    for (HWND hwnd : windows) {
        hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)remoteProtect, hwnd, 0, nullptr);
        if (hThread) {
            WaitForSingleObject(hThread, 1000);
            CloseHandle(hThread);
        }
    }
    CloseHandle(hProcess);
    return true;
}

bool InjectAndUnprotect(DWORD processId) {
    std::vector<HWND> windows = GetProcessWindows(processId);
    if (windows.empty()) return false;
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return false;
    wchar_t dllPath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, dllPath, MAX_PATH);
    PathRemoveFileSpecW(dllPath);
    PathAppendW(dllPath, L"InjectDll.dll");
    size_t pathLen = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) { CloseHandle(hProcess); return false; }
    if (!WriteProcessMemory(hProcess, remotePath, dllPath, pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, loadLib, remotePath, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    HMODULE hInject = nullptr;
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (DWORD i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
            wchar_t modName[MAX_PATH] = {};
            if (GetModuleFileNameExW(hProcess, hMods[i], modName, MAX_PATH)) {
                if (wcsstr(modName, L"InjectDll.dll")) {
                    hInject = hMods[i];
                    break;
                }
            }
        }
    }
    if (!hInject) { CloseHandle(hProcess); return false; }
    if (!LoadInjectDll()) { CloseHandle(hProcess); return false; }
    DWORD_PTR offset = GetFunctionOffset(g_SetUnprotect);
    FARPROC remoteUnprotect = (FARPROC)((DWORD_PTR)hInject + offset);
    for (HWND hwnd : windows) {
        hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)remoteUnprotect, hwnd, 0, nullptr);
        if (hThread) {
            WaitForSingleObject(hThread, 1000);
            CloseHandle(hThread);
        }
    }
    CloseHandle(hProcess);
    return true;
}
