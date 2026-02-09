#pragma once

#include "Common.h"

void CreateTrayIcon(HWND hwnd);
void DestroyTrayIcon();
void ShowTrayContextMenu(HWND hwnd, const std::vector<ConfigEntry>& entries, int x, int y);
