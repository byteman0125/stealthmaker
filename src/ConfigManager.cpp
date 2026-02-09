#include "Common.h"
#include <fstream>
#include <sstream>

static const wchar_t* REG_PATH = L"Software\\StealthMaker\\Config";
static const wchar_t* REG_RUN = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool SaveConfig(const std::vector<ConfigEntry>& entries, bool autoMonitor, bool runAtStartup, bool captureProtection) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    std::wstringstream ss;
    ss << (autoMonitor ? 1 : 0) << L"," << (runAtStartup ? 1 : 0) << L"," << (captureProtection ? 1 : 0) << L"\n";
    for (const auto& e : entries) {
        ss << e.processPath << L"\t" << e.filePath << L"\t" << e.name << L"\t" << (e.active ? 1 : 0) << L"\n";
    }
    std::wstring data = ss.str();
    RegSetValueExW(hKey, L"Data", 0, REG_SZ, (BYTE*)data.c_str(), (DWORD)((data.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    if (runAtStartup) {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"StealthMaker", 0, REG_SZ, (BYTE*)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    } else {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, L"StealthMaker");
            RegCloseKey(hKey);
        }
    }
    return true;
}

bool LoadConfig(std::vector<ConfigEntry>& entries, bool& autoMonitor, bool& runAtStartup, bool& captureProtection) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        autoMonitor = true;
        runAtStartup = false;
        captureProtection = true;
        return true;
    }
    wchar_t data[65536] = {};
    DWORD size = sizeof(data);
    if (RegQueryValueExW(hKey, L"Data", nullptr, nullptr, (BYTE*)data, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        autoMonitor = true;
        runAtStartup = false;
        captureProtection = true;
        return true;
    }
    RegCloseKey(hKey);
    std::wstringstream ss(data);
    std::wstring line;
    if (std::getline(ss, line)) {
        int am, rs, cp;
        if (swscanf_s(line.c_str(), L"%d,%d,%d", &am, &rs, &cp) == 3) {
            autoMonitor = (am != 0);
            runAtStartup = (rs != 0);
            captureProtection = (cp != 0);
        } else if (swscanf_s(line.c_str(), L"%d,%d", &am, &rs) == 2) {
            autoMonitor = (am != 0);
            runAtStartup = (rs != 0);
            captureProtection = true;
        }
    }
    entries.clear();
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        ConfigEntry e;
        size_t p1 = line.find(L'\t');
        size_t p2 = line.find(L'\t', p1 + 1);
        size_t p3 = line.find(L'\t', p2 + 1);
        if (p1 != std::wstring::npos && p2 != std::wstring::npos && p3 != std::wstring::npos) {
            e.processPath = line.substr(0, p1);
            e.filePath = line.substr(p1 + 1, p2 - p1 - 1);
            e.name = line.substr(p2 + 1, p3 - p2 - 1);
            e.active = (line.substr(p3 + 1) == L"1");
            entries.push_back(e);
        }
    }
    return true;
}
