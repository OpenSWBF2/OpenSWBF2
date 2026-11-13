#pragma once

#include <windows.h>

#define LGLCD_E_SERVICE_NOT_ACTIVE  ERROR_SERVICE_NOT_ACTIVE
#define LGLCD_E_ALREADY_INITIALIZED ERROR_ALREADY_INITIALIZED
#define LGLCD_E_SERVER_UNAVAILABLE  RPC_S_SERVER_UNAVAILABLE

#define LGLCD_BMP_FORMAT_160x43x1   0x00000001

#define LGLCD_ASYNC_UPDATE_NORMAL   0x80000080

#define LGLCD_SYNC_UPDATE_NORMAL    0x00000080
#define LGLCD_ASYNC_UPDATE_ALERT    0x800000FF

struct lgLcdConnectContextA
{
    char const* appFriendlyName;
    BOOL isPersistent;
    BOOL isAutostartable;
    void* configCallback;
    void* configContext;
    int connection;
};
 

struct lgLcdOpenContext
{
    int connection;
    int index;
    void* softbuttonsChangedCallback;
    void* softbuttonsChangedContext;
    int device;
};
 

struct lgLcdDeviceDesc
{
    DWORD Width;
    DWORD Height;
    DWORD Bpp;
    DWORD NumSoftButtons;
};
 

struct lgLcdBitmap160x43x1
{
    DWORD Format;
    BYTE pixels[160 * 43];
};
 

struct lgLcdInterface
{
    DWORD (WINAPI* Init)();
    DWORD (WINAPI* DeInit)();
    void* mUnknown0008;
    DWORD (WINAPI* Connect)(lgLcdConnectContextA* pCtx);
    DWORD (WINAPI* Disconnect)(int iConnection);
    DWORD (WINAPI* Enumerate)(int iConnection, int iIndex,
                              lgLcdDeviceDesc* pDesc);
    DWORD (WINAPI* Open)(lgLcdOpenContext* pCtx);
    void* mUnknown001C;
    DWORD (WINAPI* ReadSoftButtons)(int iDevice, DWORD* pdwButtons);
    DWORD (WINAPI* UpdateBitmap)(int iDevice, void const* pBitmap,
                                 DWORD dwPriority);
};

extern "C" {
// FUNCTION: SWBF2 0x004012a5 SYMBOL
// _lgLcdInit@0
DWORD WINAPI lgLcdInit();
// FUNCTION: SWBF2 0x004013b5 SYMBOL
// _lgLcdConnectA@4
DWORD WINAPI lgLcdConnectA(lgLcdConnectContextA* pCtx);
// FUNCTION: SWBF2 0x004013f3 SYMBOL
// _lgLcdEnumerate@12
DWORD WINAPI lgLcdEnumerate(int iConnection, int iIndex, lgLcdDeviceDesc* pDesc);
// FUNCTION: SWBF2 0x0040141a SYMBOL
// _lgLcdOpen@4
DWORD WINAPI lgLcdOpen(lgLcdOpenContext* pCtx);

// FUNCTION: SWBF2 0x00401458 SYMBOL
// _lgLcdReadSoftButtons@8
DWORD WINAPI lgLcdReadSoftButtons(int iDevice, DWORD* pdwButtons);
// FUNCTION: SWBF2 0x0040147b SYMBOL
// _lgLcdUpdateBitmap@12
DWORD WINAPI lgLcdUpdateBitmap(int iDevice, void const* pBitmap, DWORD dwPriority);

// FUNCTION: SWBF2 0x0040138e SYMBOL
// _lgLcdDeInit@0
DWORD WINAPI lgLcdDeInit();
// FUNCTION: SWBF2 0x004013d4 SYMBOL
// _lgLcdDisconnect@4
DWORD WINAPI lgLcdDisconnect(int iConnection);
// FUNCTION: SWBF2 0x00401439 SYMBOL
// _lgLcdClose@4
DWORD WINAPI lgLcdClose(int iDevice);
// FUNCTION: SWBF2 0x004012d4 SYMBOL
// _LinkLgLcdDLL
BOOL LinkLgLcdDLL();
// FUNCTION: SWBF2 0x0040136c SYMBOL
// _UnlinkLgLcdDLL
BOOL UnlinkLgLcdDLL();
}

// GLOBAL: SWBF2 0x01e2fd04
// _g_lgLcdLibIsInitialized
extern "C" int g_lgLcdLibIsInitialized;
// GLOBAL: SWBF2 0x01fac3b0
// _g_lgLcdLibLinkedRemoteInterface
extern "C" lgLcdInterface* g_lgLcdLibLinkedRemoteInterface;
extern HMODULE hLibModule;
extern void* dword_1E2FBFC;

// VTABLE: SWBF2 0x00783e40
class RedLogitechLCD
{
public:

    RedLogitechLCD();
    virtual ~RedLogitechLCD();

    void ConnectLCDPanel(unsigned char ucPortIndex);
    void OpenLCDPanel();
    void ClearLCD();
    void PrepareLCDDrawing();
    void DrawLCDText(char const* lpString, int x, int y);
    void PresentLCD();
    void UpdateLCDPanel(float fPriority);
    void DrawLCDLine(int x0, int y0, int x1, int y1);
    void DrawLCDWideText(const unsigned short* pwString, int x, int y);
    void DrawLCDWrappedText(const unsigned short* pwString);
    void CloseLCDPanel();
    void DisconnectLCDPanel();

    unsigned char mPortIndex;
    bool mOpened;
    bool mConnected;
    unsigned char mUnknown0007;
    lgLcdConnectContextA mConnectCtx;
    lgLcdOpenContext mOpenCtx;
    lgLcdBitmap160x43x1* mpBitmap;
    BITMAPINFO* mpBitmapInfo;
    HDC mhDC;
    HBITMAP mhBitmap;

    HGDIOBJ mhOldBitmap;
    void* mpBits;
    HFONT mhFont;
    int mWidth;
    int mHeight;
    DWORD mSoftButtons;
    DWORD mPrevSoftButtons;
};
 

extern RedLogitechLCD gLCDPanels[1];




extern char* lpString2;
extern int dword_7E143C;
