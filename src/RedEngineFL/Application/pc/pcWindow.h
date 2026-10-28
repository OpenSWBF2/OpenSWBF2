#pragma once

#include <windows.h>

#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"

namespace PcWindow
{

void InitDpi(bool bNoDpi);

DisplayLogic::DpiLevel GetDpiLevel();

unsigned int QueryDpi(HWND hWnd);

bool MonitorRectsFor(HMONITOR hMon, DisplayLogic::Rect& monitor, DisplayLogic::Rect& work);

bool PrimaryMonitorRects(DisplayLogic::Rect& monitor, DisplayLogic::Rect& work);

DisplayLogic::Frame MeasureFrame(unsigned long style, unsigned long exStyle, HWND hWnd);

void ApplyWindowMode(DisplayLogic::DisplayMode mode, unsigned int uiClientW, unsigned int uiClientH,
                     HMONITOR hMon, bool bMinimized);

void ApplyPending();

bool IsApplying();

void OnDpiChanged(HWND hWnd, WPARAM wParam, LPARAM lParam, DisplayLogic::DisplayMode mode);

}
