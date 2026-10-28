#include <windows.h>
#include <string.h>

#include "RedEngineFL/Application/pc/pcWindow.h"
#include "RedEngineFL/input/pc/pcRedInputManager.h"

extern bool g_bNoRender;
extern HWND hGlobalWnd;

namespace PcWindow
{

typedef BOOL(WINAPI* SetDpiAwarenessContextFn)(HANDLE);
typedef HRESULT(WINAPI* SetDpiAwarenessFn)(int);
typedef BOOL(WINAPI* SetDpiAwareFn)();
typedef UINT(WINAPI* GetDpiForWindowFn)(HWND);
typedef BOOL(WINAPI* AdjustRectForDpiFn)(LPRECT, DWORD, BOOL, DWORD, UINT);

typedef HANDLE(WINAPI* GetThreadDpiContextFn)();
typedef int(WINAPI* GetAwarenessFromContextFn)(HANDLE);
typedef BOOL(WINAPI* ContextsEqualFn)(HANDLE, HANDLE);
typedef HRESULT(WINAPI* GetDpiAwarenessFn)(HANDLE, int*);

static DisplayLogic::DpiLevel sLevel = DisplayLogic::kDpiLevelNone;
static bool sAlready = false;
static bool sApplying = false;
static unsigned int sClientW = 0;
static unsigned int sClientH = 0;
static bool sPending = false;
static DisplayLogic::DisplayMode sPendingMode = DisplayLogic::kModeFullscreen;
static HMONITOR sPendingMon = 0;

static int StepPmV2(void*)
{
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (!hUser)
    {
        return DisplayLogic::kDpiFail;
    }
    SetDpiAwarenessContextFn pFn =
        (SetDpiAwarenessContextFn)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
    if (!pFn)
    {
        return DisplayLogic::kDpiFail;
    }
    if (pFn((HANDLE)-4))
    {
        return DisplayLogic::kDpiOk;
    }
    if (GetLastError() == ERROR_ACCESS_DENIED)
    {
        sAlready = true;
        return DisplayLogic::kDpiAlready;
    }
    return DisplayLogic::kDpiFail;
}

static HMODULE LoadShcore()
{
    char acPath[MAX_PATH];
    const char acName[] = "\\shcore.dll";
    UINT uiLen = GetSystemDirectoryA(acPath, MAX_PATH);
    if (uiLen == 0 || uiLen + sizeof(acName) > MAX_PATH)
    {
        return 0;
    }
    memcpy(acPath + uiLen, acName, sizeof(acName));
    return LoadLibraryA(acPath);
}

static int StepPm(void*)
{
    HMODULE hShcore = LoadShcore();
    if (!hShcore)
    {
        return DisplayLogic::kDpiFail;
    }
    SetDpiAwarenessFn pFn = (SetDpiAwarenessFn)GetProcAddress(hShcore, "SetProcessDpiAwareness");
    if (!pFn)
    {
        return DisplayLogic::kDpiFail;
    }
    HRESULT hr = pFn(2);
    if (hr == S_OK)
    {
        return DisplayLogic::kDpiOk;
    }
    if (hr == (HRESULT)0x80070005)
    {
        sAlready = true;
        return DisplayLogic::kDpiAlready;
    }
    return DisplayLogic::kDpiFail;
}

static int StepSystem(void*)
{
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (!hUser)
    {
        return DisplayLogic::kDpiFail;
    }
    SetDpiAwareFn pFn = (SetDpiAwareFn)GetProcAddress(hUser, "SetProcessDPIAware");
    if (pFn && pFn())
    {
        return DisplayLogic::kDpiOk;
    }
    return DisplayLogic::kDpiFail;
}

static int QueryAwareness(bool& bPmV2)
{
    bPmV2 = false;
    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser)
    {
        GetThreadDpiContextFn pGet =
            (GetThreadDpiContextFn)GetProcAddress(hUser, "GetThreadDpiAwarenessContext");
        GetAwarenessFromContextFn pAwareness = (GetAwarenessFromContextFn)GetProcAddress(
            hUser, "GetAwarenessFromDpiAwarenessContext");
        ContextsEqualFn pEqual =
            (ContextsEqualFn)GetProcAddress(hUser, "AreDpiAwarenessContextsEqual");
        if (pGet && pAwareness)
        {
            HANDLE hCtx = pGet();
            const int iAwareness = pAwareness(hCtx);
            if (iAwareness >= 0)
            {
                bPmV2 = pEqual && pEqual(hCtx, (HANDLE)-4);
                return iAwareness;
            }
        }
    }
    HMODULE hShcore = LoadShcore();
    if (hShcore)
    {
        GetDpiAwarenessFn pFn =
            (GetDpiAwarenessFn)GetProcAddress(hShcore, "GetProcessDpiAwareness");
        int iAwareness = -1;
        if (pFn && pFn(0, &iAwareness) == S_OK)
        {
            return iAwareness;
        }
    }
    return -1;
}

void InitDpi(bool bNoDpi)
{
    DisplayLogic::DpiSteps steps;
    steps.pmv2 = StepPmV2;
    steps.pm = StepPm;
    steps.system = StepSystem;
    steps.pCtx = 0;
    sLevel = DisplayLogic::DpiCascade(steps, bNoDpi);
    if (sAlready && sLevel != DisplayLogic::kDpiLevelOff)
    {
        bool bPmV2 = false;
        const int iAwareness = QueryAwareness(bPmV2);
        sLevel = DisplayLogic::PresetDpiLevel(iAwareness, bPmV2, sLevel);
    }
}

DisplayLogic::DpiLevel GetDpiLevel()
{
    return sLevel;
}

unsigned int QueryDpi(HWND hWnd)
{
    if (hWnd)
    {
        HMODULE hUser = GetModuleHandleA("user32.dll");
        GetDpiForWindowFn pFn =
            hUser ? (GetDpiForWindowFn)GetProcAddress(hUser, "GetDpiForWindow") : 0;
        if (pFn)
        {
            UINT uiDpi = pFn(hWnd);
            if (uiDpi)
            {
                return uiDpi;
            }
        }
    }
    HDC hDc = GetDC(0);
    if (hDc)
    {
        int iDpi = GetDeviceCaps(hDc, LOGPIXELSX);
        ReleaseDC(0, hDc);
        if (iDpi > 0)
        {
            return (unsigned int)iDpi;
        }
    }
    return 96;
}

static bool QueryMonitor(HMONITOR hMon, DisplayLogic::Rect& monitor, DisplayLogic::Rect& work)
{
    MONITORINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (!hMon || !GetMonitorInfoA(hMon, &mi))
    {
        return false;
    }
    monitor.l = mi.rcMonitor.left;
    monitor.t = mi.rcMonitor.top;
    monitor.r = mi.rcMonitor.right;
    monitor.b = mi.rcMonitor.bottom;
    work.l = mi.rcWork.left;
    work.t = mi.rcWork.top;
    work.r = mi.rcWork.right;
    work.b = mi.rcWork.bottom;
    return true;
}

bool MonitorRectsFor(HMONITOR hMon, DisplayLogic::Rect& monitor, DisplayLogic::Rect& work)
{
    if (g_bNoRender)
    {
        return false;
    }
    if (!hMon)
    {
        if (hGlobalWnd)
        {
            hMon = MonitorFromWindow(hGlobalWnd, MONITOR_DEFAULTTOPRIMARY);
        }
        else
        {
            POINT pt = {0, 0};
            hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        }
    }
    return QueryMonitor(hMon, monitor, work);
}

bool PrimaryMonitorRects(DisplayLogic::Rect& monitor, DisplayLogic::Rect& work)
{
    if (g_bNoRender)
    {
        return false;
    }
    POINT pt = {0, 0};
    return QueryMonitor(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), monitor, work);
}

static void AdjustForDpi(RECT& rc, DWORD style, DWORD exStyle, HWND hWnd, UINT uiDpi)
{
    if (sLevel == DisplayLogic::kDpiLevelPmV2)
    {
        HMODULE hUser = GetModuleHandleA("user32.dll");
        AdjustRectForDpiFn pFn =
            hUser ? (AdjustRectForDpiFn)GetProcAddress(hUser, "AdjustWindowRectExForDpi") : 0;
        if (pFn && pFn(&rc, style, FALSE, exStyle, uiDpi))
        {
            return;
        }
    }
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);
}

static void FixClientSize(HWND hWnd, HWND hAfter, const DisplayLogic::Rect& rc,
                          unsigned int uiClientW, unsigned int uiClientH, UINT uiFlags)
{
    RECT client;
    if (GetClientRect(hWnd, &client))
    {
        long lDx = (long)uiClientW - (client.right - client.left);
        long lDy = (long)uiClientH - (client.bottom - client.top);
        if (lDx != 0 || lDy != 0)
        {
            SetWindowPos(hWnd, hAfter, rc.l, rc.t, rc.r - rc.l + lDx, rc.b - rc.t + lDy, uiFlags);
        }
    }
}

DisplayLogic::Frame MeasureFrame(unsigned long style, unsigned long exStyle, HWND hWnd)
{
    RECT rc = {0, 0, 0, 0};
    AdjustForDpi(rc, style, exStyle, hWnd, QueryDpi(hWnd));
    DisplayLogic::Frame frame;
    frame.l = rc.left;
    frame.t = rc.top;
    frame.r = rc.right;
    frame.b = rc.bottom;
    return frame;
}

bool IsApplying()
{
    return sApplying;
}

void ApplyWindowMode(DisplayLogic::DisplayMode mode, unsigned int uiClientW, unsigned int uiClientH,
                     HMONITOR hMon, bool bMinimized)
{
    if (g_bNoRender || !hGlobalWnd)
    {
        return;
    }

    sClientW = uiClientW;
    sClientH = uiClientH;
    if (IsIconic(hGlobalWnd))
    {
        sPending = true;
        sPendingMode = mode;
        sPendingMon = hMon;
        return;
    }
    sPending = false;
    sApplying = true;

    RedMouse* pMouse = RedInputManager::GetMouse(0);
    bool bMouse = pMouse && pMouse->Exists() && pMouse->m_connected;
    if (bMouse)
    {
        pMouse->Remove();
    }

    const DWORD kStateBits = WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE;
    DisplayLogic::WinStyle ws = DisplayLogic::WindowStyleFor(mode, true);
    ws.style = (ws.style & ~kStateBits) | ((DWORD)GetWindowLongA(hGlobalWnd, GWL_STYLE) & kStateBits);

    DisplayLogic::Rect monitor = {0, 0, 0, 0};
    DisplayLogic::Rect work = {0, 0, 0, 0};
    if (!MonitorRectsFor(hMon, monitor, work))
    {
        PrimaryMonitorRects(monitor, work);
    }

    DisplayLogic::Frame frame = MeasureFrame(ws.style, ws.exStyle, hGlobalWnd);
    DisplayLogic::Rect rc = DisplayLogic::PlaceWindow(mode, monitor, work, uiClientW, uiClientH, frame);

    UINT uiFlags = SWP_FRAMECHANGED | SWP_NOACTIVATE | (bMinimized ? SWP_NOZORDER : 0);
    HWND hAfter = DisplayLogic::Topmost(mode) ? HWND_TOPMOST : HWND_NOTOPMOST;

    SetWindowLongA(hGlobalWnd, GWL_STYLE, (LONG)ws.style);
    SetWindowLongA(hGlobalWnd, GWL_EXSTYLE, (LONG)ws.exStyle);
    SetWindowPos(hGlobalWnd, hAfter, rc.l, rc.t, rc.r - rc.l, rc.b - rc.t, uiFlags);

    if (mode == DisplayLogic::kModeWindowed)
    {
        FixClientSize(hGlobalWnd, hAfter, rc, uiClientW, uiClientH, uiFlags);
    }

    if (bMouse)
    {
        pMouse->Insert();
    }
    sApplying = false;
}

void ApplyPending()
{
    if (!sPending || sApplying)
    {
        return;
    }
    ApplyWindowMode(sPendingMode, sClientW, sClientH, sPendingMon, false);
}

void OnDpiChanged(HWND hWnd, WPARAM wParam, LPARAM lParam, DisplayLogic::DisplayMode mode)
{
    if (!lParam || mode != DisplayLogic::kModeWindowed || sApplying || !sClientW || !sClientH ||
        !hWnd)
    {
        return;
    }
    const RECT* pSuggested = (const RECT*)lParam;
    UINT uiDpi = HIWORD(wParam);

    RECT rcFrame = {0, 0, 0, 0};
    AdjustForDpi(rcFrame, (DWORD)GetWindowLongA(hWnd, GWL_STYLE),
                 (DWORD)GetWindowLongA(hWnd, GWL_EXSTYLE), hWnd, uiDpi);
    DisplayLogic::Frame frame = {rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom};
    DisplayLogic::Rect suggested = {pSuggested->left, pSuggested->top, pSuggested->right,
                                    pSuggested->bottom};
    DisplayLogic::Rect rc = DisplayLogic::DpiChangedRect(suggested, sClientW, sClientH, frame);
    SetWindowPos(hWnd, 0, rc.l, rc.t, rc.r - rc.l, rc.b - rc.t, SWP_NOZORDER | SWP_NOACTIVATE);
    FixClientSize(hWnd, 0, rc, sClientW, sClientH, SWP_NOZORDER | SWP_NOACTIVATE);
}

}
