#include "Common.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

std::wstring GetWindowTitle(HWND hwnd) {
    wchar_t buf[256] = {};
    if (GetWindowTextW(hwnd, buf, 256) > 0) {
        return buf;
    }
    return L"";
}

std::wstring GetProcessPath(DWORD processId) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!h) return L"";
    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    BOOL ok = QueryFullProcessImageNameW(h, 0, path, &size);
    CloseHandle(h);
    return ok ? path : L"";
}

static std::wstring ExtractFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? path.substr(pos + 1) : path;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* out = (std::vector<WindowInfo>*)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetWindow(hwnd, GW_OWNER)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return TRUE;
    std::wstring processPath = GetProcessPath(pid);
    wchar_t title[256] = {};
    GetWindowTextW(hwnd, title, 256);
    std::wstring displayTitle = title[0] ? title : (processPath.empty() ? L"Untitled" : ExtractFileName(processPath));
    if (displayTitle.empty()) return TRUE;
    WindowInfo wi;
    wi.hwnd = hwnd;
    wi.processId = pid;
    wi.title = displayTitle;
    wi.processPath = processPath;
    out->push_back(wi);
    return TRUE;
}

std::vector<WindowInfo> EnumVisibleWindows() {
    std::vector<WindowInfo> list;
    EnumWindows(EnumWindowsProc, (LPARAM)&list);
    std::sort(list.begin(), list.end(), [](const WindowInfo& a, const WindowInfo& b) {
        if (a.processId != b.processId) return a.processId < b.processId;
        return a.title < b.title;
    });
    auto last = std::unique(list.begin(), list.end(), [](const WindowInfo& a, const WindowInfo& b) {
        return a.processId == b.processId;
    });
    list.erase(last, list.end());
    return list;
}

static BOOL CALLBACK EnumProtectedProc(HWND hwnd, LPARAM lParam) {
    auto* out = (std::vector<WindowInfo>*)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (!IsWindowProtected(hwnd)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return TRUE;
    WindowInfo wi;
    wi.hwnd = hwnd;
    wi.processId = pid;
    wi.title = GetWindowTitle(hwnd);
    wi.processPath = GetProcessPath(pid);
    out->push_back(wi);
    return TRUE;
}

std::vector<WindowInfo> EnumProtectedWindows() {
    std::vector<WindowInfo> list;
    EnumWindows(EnumProtectedProc, (LPARAM)&list);
    return list;
}

struct EnumProcData { std::vector<HWND> out; DWORD pid; };

static BOOL CALLBACK EnumProcessWindowsProc(HWND hwnd, LPARAM lp) {
    auto* d = (EnumProcData*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == d->pid && IsWindowVisible(hwnd) && !GetWindow(hwnd, GW_OWNER)) {
        d->out.push_back(hwnd);
    }
    return TRUE;
}

static BOOL CALLBACK EnumAllProcessWindowsProc(HWND hwnd, LPARAM lp) {
    auto* d = (EnumProcData*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == d->pid && !GetWindow(hwnd, GW_OWNER)) {
        d->out.push_back(hwnd);
    }
    return TRUE;
}

std::vector<HWND> GetProcessWindows(DWORD processId) {
    EnumProcData d = {};
    d.pid = processId;
    EnumWindows(EnumProcessWindowsProc, (LPARAM)&d);
    return d.out;
}

std::vector<HWND> GetAllProcessWindows(DWORD processId) {
    EnumProcData d = {};
    d.pid = processId;
    EnumWindows(EnumAllProcessWindowsProc, (LPARAM)&d);
    return d.out;
}

DWORD FindProcessByPath(const std::wstring& path) {
    std::vector<WindowInfo> list = EnumVisibleWindows();
    for (const auto& wi : list) {
        if (_wcsicmp(wi.processPath.c_str(), path.c_str()) == 0) {
            return wi.processId;
        }
    }
    return 0;
}

bool IsWindowProtected(HWND hwnd) {
    DWORD affinity = 0;
    return GetWindowDisplayAffinity(hwnd, &affinity) && affinity == WDA_EXCLUDEFROMCAPTURE;
}
