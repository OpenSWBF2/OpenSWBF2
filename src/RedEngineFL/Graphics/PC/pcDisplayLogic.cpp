#include <string.h>

#include <d3d9.h>

#include "pcDisplayLogic.h"

#include "RedEngineFL/Util/RedLog.h"

namespace DisplayLogic
{

static bool FindOrdinal(const AdapterRec* pAdapters, unsigned int uiCount, unsigned int uiOrdinal)
{
    for (unsigned int i = 0; i < uiCount; i++)
    {
        if (pAdapters[i].ordinal == uiOrdinal)
        {
            return true;
        }
    }
    return false;
}

unsigned int BuildModeList(const DisplayModeRec* pSrc, unsigned int uiSrc, unsigned int uiAdapter,
                           DisplayModeRec* pOut, unsigned int uiOutCap)
{
    unsigned int uiTotal = 0;
    DisplayModeRec kLast = {0, 0, 0, 0};
    for (unsigned int i = 0; i < uiSrc; i++)
    {
        if (pSrc[i].adapter != uiAdapter)
        {
            continue;
        }
        bool bDuplicate = uiTotal > 0 && pSrc[i].w == kLast.w && pSrc[i].h == kLast.h
            && pSrc[i].refresh == kLast.refresh;
        if (bDuplicate)
        {
            continue;
        }
        if (pOut && uiTotal < uiOutCap)
        {
            pOut[uiTotal] = pSrc[i];
        }
        kLast = pSrc[i];
        uiTotal++;
    }
    return uiTotal;
}

unsigned int UniqueResolutions(const DisplayModeRec* pList, unsigned int uiCount, unsigned int uiMinW,
                               unsigned int uiMinH, DisplayModeRec* pOut, unsigned int uiOutCap)
{
    unsigned int uiTotal = 0;
    for (unsigned int i = 0; i < uiCount; i++)
    {
        if (pList[i].w < uiMinW || pList[i].h < uiMinH)
        {
            continue;
        }
        bool bSeen = false;
        for (unsigned int j = 0; j < i && !bSeen; j++)
        {
            bSeen = pList[j].w == pList[i].w && pList[j].h == pList[i].h;
        }
        if (bSeen)
        {
            continue;
        }
        if (pOut && uiTotal < uiOutCap)
        {
            pOut[uiTotal] = pList[i];
        }
        uiTotal++;
    }
    return uiTotal;
}

void DesktopDefault(const DisplayModeRec* pList, unsigned int uiCount, unsigned int uiDeskW,
                    unsigned int uiDeskH, unsigned int& uiW, unsigned int& uiH)
{
    if (uiDeskW == 0 || uiDeskH == 0 || uiCount == 0)
    {
        uiW = kFallbackWidth;
        uiH = kFallbackHeight;
        return;
    }
    uiW = 0;
    uiH = 0;
    for (unsigned int i = 0; i < uiCount; i++)
    {
        if (pList[i].w > uiDeskW || pList[i].h > uiDeskH)
        {
            continue;
        }
        unsigned long long ullArea = (unsigned long long)pList[i].w * pList[i].h;
        unsigned long long ullBest = (unsigned long long)uiW * uiH;
        if (ullArea > ullBest || (ullArea == ullBest && pList[i].w > uiW))
        {
            uiW = pList[i].w;
            uiH = pList[i].h;
        }
    }
    if (uiW == 0 && uiH == 0 && uiCount != 0 && uiDeskW != 0 && uiDeskH != 0)
    {
        uiW = kFallbackWidth;
        uiH = kFallbackHeight;
    }
}

unsigned int PickRefresh(const DisplayModeRec* pList, unsigned int uiCount, unsigned int uiW,
                         unsigned int uiH, unsigned int uiRequested, bool bWindowed)
{
    if (bWindowed || uiRequested == 0)
    {
        return 0;
    }
    unsigned int uiHighest = 0;
    for (unsigned int i = 0; i < uiCount; i++)
    {
        if (pList[i].w != uiW || pList[i].h != uiH)
        {
            continue;
        }
        if (pList[i].refresh == uiRequested)
        {
            return uiRequested;
        }
        if (pList[i].refresh > uiHighest)
        {
            uiHighest = pList[i].refresh;
        }
    }
    if (uiHighest == 0)
    {
        return 0;
    }
    LOG(LogRender, Warning, "display: refresh %uHz not listed for %ux%u, using %uHz", uiRequested, uiW,
        uiH, uiHighest);
    return uiHighest;
}

unsigned int ResolveAdapter(unsigned int uiReqOrdinal, bool bOrdinalFromCli, const char* pcReqName,
                            const AdapterRec* pAdapters, unsigned int uiCount, bool bNoRender)
{
    if (bNoRender)
    {
        return kPrimaryAdapter;
    }
    bool bNameTried = !bOrdinalFromCli && pcReqName && pcReqName[0] != '\0';
    if (bNameTried)
    {
        for (unsigned int i = 0; i < uiCount; i++)
        {
            if (pAdapters[i].name && _stricmp(pAdapters[i].name, pcReqName) == 0)
            {
                return pAdapters[i].ordinal;
            }
        }
    }
    if (FindOrdinal(pAdapters, uiCount, uiReqOrdinal))
    {
        if (bNameTried)
        {
            LOG(LogRender, Warning, "display: adapter name '%.64s' not found, using ordinal %u",
                pcReqName, uiReqOrdinal);
        }
        return uiReqOrdinal;
    }
    LOG(LogRender, Warning, "display: adapter %u%s%.64s%s not found, using adapter %u", uiReqOrdinal,
        bNameTried ? " / '" : "", bNameTried ? pcReqName : "", bNameTried ? "'" : "", kPrimaryAdapter);
    return kPrimaryAdapter;
}

bool UsableUncached(bool bInList, bool bIsDesktop, bool bMsNone, unsigned int uiMsLevels,
                    unsigned int uiQuality)
{
    if (!bInList && !bIsDesktop)
    {
        return false;
    }
    if (bMsNone)
    {
        return true;
    }
    return uiMsLevels > uiQuality;
}

unsigned int EffectiveVsync(bool bKeySet, unsigned int uiKeyValue, bool bProfile)
{
    return bKeySet ? uiKeyValue : (bProfile ? 1u : 0u);
}

unsigned int ResolveVsyncInterval(unsigned int uiRequested, unsigned long ulCaps, bool bWindowed,
                                  bool* pbFellBack)
{
    if (pbFellBack)
    {
        *pbFellBack = false;
    }
    if (uiRequested == 0)
    {
        if (ulCaps != 0 && !(ulCaps & kIntervalImmediate))
        {
            if (pbFellBack)
            {
                *pbFellBack = true;
            }
            return kIntervalDefault;
        }
        return kIntervalImmediate;
    }
    if (uiRequested == 2)
    {
        if (!bWindowed && (ulCaps & kIntervalTwo))
        {
            return kIntervalTwo;
        }
        if (pbFellBack)
        {
            *pbFellBack = true;
        }
        return kIntervalDefault;
    }
    return kIntervalDefault;
}

const ResetBounds kStartupBounds = {0, 30000};
const ResetBounds kRuntimeBounds = {20, 0};

static ResetAction RevertOrFatal(bool bRevertAvailable)
{
    if (bRevertAvailable)
    {
        return kResetRevert;
    }
    return kResetFatal;
}

static bool OverBounds(unsigned int uiFailures, unsigned int uiElapsedMs, const ResetBounds& bounds)
{
    if (bounds.uiMaxFailures != 0 && uiFailures >= bounds.uiMaxFailures)
    {
        return true;
    }
    if (bounds.uiMaxMs != 0 && uiElapsedMs >= bounds.uiMaxMs)
    {
        return true;
    }
    return false;
}

ResetAction CoopStep(long hrCoop, unsigned int uiElapsedMs, const ResetBounds& bounds)
{
    if (hrCoop == D3DERR_DEVICELOST)
    {
        if (OverBounds(0, uiElapsedMs, bounds))
        {
            return kResetFatal;
        }
        return kResetWait;
    }
    if (SUCCEEDED(hrCoop) || hrCoop == D3DERR_DEVICENOTRESET)
    {
        return kResetNow;
    }
    return kResetFatal;
}

ResetAction ResetStep(long hr, unsigned int uiFailures, unsigned int uiElapsedMs,
                      bool bRevertAvailable, const ResetBounds& bounds)
{
    if (SUCCEEDED(hr))
    {
        return kResetDone;
    }
    if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
    {
        if (OverBounds(uiFailures, uiElapsedMs, bounds))
        {
            return RevertOrFatal(bRevertAvailable);
        }
        return kResetWait;
    }
    if (hr == D3DERR_INVALIDCALL || hr == D3DERR_NOTAVAILABLE || hr == E_OUTOFMEMORY ||
        hr == D3DERR_OUTOFVIDEOMEMORY)
    {
        return RevertOrFatal(bRevertAvailable);
    }
    return kResetFatal;
}

static const unsigned long kMaxClient = 16384;
static const unsigned long kBorderlessExStyle = 0;

DisplayMode ResolveMode(bool bBorderless, bool bCliWin, bool bProfileWindowed)
{
    if (bBorderless)
    {
        return kModeBorderless;
    }
    if (bCliWin || bProfileWindowed)
    {
        return kModeWindowed;
    }
    return kModeFullscreen;
}

bool D3DWindowed(DisplayMode m)
{
    return m != kModeFullscreen;
}

bool MinimizeOnDeactivate(DisplayMode m)
{
    return m == kModeFullscreen;
}

bool Topmost(DisplayMode m)
{
    return m == kModeFullscreen;
}

WinStyle WindowStyleFor(DisplayMode m, bool bVisible)
{
    WinStyle ws;
    if (m == kModeWindowed)
    {
        ws.style = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX |
                   (bVisible ? WS_VISIBLE : 0);
        ws.exStyle = 0;
        return ws;
    }
    ws.style = WS_POPUP | WS_VISIBLE;
    if (m == kModeBorderless)
    {
        ws.exStyle = kBorderlessExStyle;
    }
    else
    {
        ws.exStyle = WS_EX_TOPMOST;
    }
    if (!bVisible)
    {
        ws.style &= ~(unsigned long)WS_VISIBLE;
    }
    return ws;
}

Rect PlaceWindow(DisplayMode m, const Rect& monitor, const Rect& work, unsigned long clientW,
                 unsigned long clientH, const Frame& frame)
{
    if (m == kModeBorderless)
    {
        return monitor;
    }
    if (clientW > kMaxClient)
    {
        clientW = kMaxClient;
    }
    if (clientH > kMaxClient)
    {
        clientH = kMaxClient;
    }
    const long lWidth = (long)clientW + frame.r - frame.l;
    const long lHeight = (long)clientH + frame.b - frame.t;
    Rect out;
    if (m == kModeWindowed)
    {
        out.l = (work.l - lWidth + work.r) / 2;
        out.t = (work.t - lHeight + work.b) / 2;
        if (out.l < work.l)
        {
            out.l = work.l;
        }
        if (out.t < work.t)
        {
            out.t = work.t;
        }
    }
    else
    {
        out.l = (monitor.l - lWidth + monitor.r) / 2;
        out.t = (monitor.t - lHeight + monitor.b) / 2;
    }
    out.r = out.l + lWidth;
    out.b = out.t + lHeight;
    return out;
}

Rect DpiChangedRect(const Rect& suggested, unsigned long clientW, unsigned long clientH,
                    const Frame& frame)
{
    Rect out;
    out.l = suggested.l;
    out.t = suggested.t;
    out.r = out.l + (long)clientW + frame.r - frame.l;
    out.b = out.t + (long)clientH + frame.b - frame.t;
    return out;
}

static bool DpiSettled(DpiStepFn fn, void* pCtx)
{
    const int iResult = fn(pCtx);
    if (iResult == kDpiAlready)
    {
        return true;
    }
    return iResult == kDpiOk;
}

DpiLevel DpiCascade(const DpiSteps& steps, bool bNoDpi)
{
    if (bNoDpi)
    {
        return kDpiLevelOff;
    }
    if (steps.pmv2)
    {
        if (DpiSettled(steps.pmv2, steps.pCtx))
        {
            return kDpiLevelPmV2;
        }
    }
    if (steps.pm)
    {
        if (DpiSettled(steps.pm, steps.pCtx))
        {
            return kDpiLevelPm;
        }
    }
    if (steps.system)
    {
        if (DpiSettled(steps.system, steps.pCtx))
        {
            return kDpiLevelSystem;
        }
    }
    return kDpiLevelNone;
}

const char* DpiLevelName(DpiLevel l)
{
    switch (l)
    {
    case kDpiLevelPmV2:
        return "pmv2";
    case kDpiLevelPm:
        return "pm";
    case kDpiLevelSystem:
        return "system";
    case kDpiLevelOff:
        return "off";
    default:
        return "none";
    }
}

DpiLevel PresetDpiLevel(int iAwareness, bool bPmV2, DpiLevel fallback)
{
    switch (iAwareness)
    {
    case 0:
        return kDpiLevelNone;
    case 1:
        return kDpiLevelSystem;
    case 2:
        return bPmV2 ? kDpiLevelPmV2 : kDpiLevelPm;
    default:
        return fallback;
    }
}

static int MapAxis(int v, unsigned int uiSrc, unsigned int uiDst)
{
    if (uiSrc == 0 || uiDst == 0)
    {
        return v < 0 ? 0 : v;
    }
    long long iResult = ((long long)v * (long long)uiDst + (long long)(uiSrc / 2)) / (long long)uiSrc;
    if (iResult > (long long)uiDst - 1)
    {
        iResult = (long long)uiDst - 1;
    }
    if (iResult < 0)
    {
        iResult = 0;
    }
    return (int)iResult;
}

void MapBackBufferToClient(int x, int y, unsigned int bbW, unsigned int bbH, unsigned int cW,
                           unsigned int cH, int& ox, int& oy)
{
    ox = MapAxis(x, bbW, cW);
    oy = MapAxis(y, bbH, cH);
}

void MapClientToBackBuffer(int x, int y, unsigned int cW, unsigned int cH, unsigned int bbW,
                           unsigned int bbH, int& ox, int& oy)
{
    ox = MapAxis(x, cW, bbW);
    oy = MapAxis(y, cH, bbH);
}

ShellSize ShellSizePolicy(DisplayMode m, bool bIsShell, unsigned int uiCliW, unsigned int uiCliH,
                          unsigned int uiProfW, unsigned int uiProfH, unsigned int uiDeskW,
                          unsigned int uiDeskH)
{
    const bool bCli = uiCliW > 0 && uiCliH > 0;
    const bool bProf = uiProfW > 0 && uiProfH > 0;
    unsigned int uiReqW = 0;
    unsigned int uiReqH = 0;
    if (bCli)
    {
        uiReqW = uiCliW;
        uiReqH = uiCliH;
    }
    else if (bIsShell)
    {
        uiReqW = 0;
        uiReqH = 0;
    }
    else if (bProf)
    {
        uiReqW = uiProfW;
        uiReqH = uiProfH;
    }

    ShellSize out;
    out.bWarnIgnoredSize = false;
    out.bWarnNoMonitor = false;
    out.uiBackW = uiReqW;
    out.uiBackH = uiReqH;
    out.uiClientW = uiReqW;
    out.uiClientH = uiReqH;

    if (m == kModeWindowed && !bCli && bProf)
    {
        out.uiClientW = uiProfW;
        out.uiClientH = uiProfH;
    }
    else if (m == kModeBorderless && (uiDeskW == 0 || uiDeskH == 0))
    {
        out.bWarnNoMonitor = !bIsShell;
    }
    else if (m == kModeBorderless)
    {
        out.uiClientW = uiDeskW;
        out.uiClientH = uiDeskH;
        if (!bIsShell)
        {
            out.uiBackW = uiDeskW;
            out.uiBackH = uiDeskH;
            out.bWarnIgnoredSize = uiReqW > 0 && uiDeskW > 0 && (uiReqW != uiDeskW || uiReqH != uiDeskH);
        }
    }
    out.bClearMsaa = out.uiBackW == 0 || out.uiBackH == 0;
    return out;
}

}
