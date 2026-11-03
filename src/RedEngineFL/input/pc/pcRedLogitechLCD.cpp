#include <string.h>

#include "lglcd/lgLcd.h"
#include "RedEngineFL/Memory/engineAlloc.h"



// GLOBAL: SWBF2 0x00783e50
// __xmm@00000190000000000000000000000000

// SYNTHETIC: SWBF2 0x00408600
// `dynamic initializer for 'gLCDPanels''
// SYNTHETIC: SWBF2 0x007699f0
// `dynamic atexit destructor for 'gLCDPanels''

// GLOBAL: SWBF2 0x0099e768
// gLCDPanels
RedLogitechLCD gLCDPanels[1];

// GLOBAL: SWBF2 0x007e1438
char* lpString2 = "Arial";

// GLOBAL: SWBF2 0x007e143c
int dword_7E143C = 11;

// SYNTHETIC: SWBF2 0x006effa0
// RedLogitechLCD::`scalar deleting destructor'

// FUNCTION: SWBF2 0x006eff40
RedLogitechLCD::RedLogitechLCD()
{
    mOpened = false;
    mConnected = false;
    mpBitmapInfo = 0;
    mpBitmap = 0;
    mhBitmap = 0;
    mhOldBitmap = 0;
    mhDC = 0;
    mhFont = 0;
    mSoftButtons = 0;
    mPrevSoftButtons = 0;
}

// FUNCTION: SWBF2 0x006eff90
RedLogitechLCD::~RedLogitechLCD()
{
}

// FUNCTION: SWBF2 0x006effd0
void RedLogitechLCD::PrepareLCDDrawing()
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened)
    {
        return;
    }
    if (!pThis->mConnected)
    {
        return;
    }
    if (pThis->mhOldBitmap)
    {
        return;
    }

    pThis->mhOldBitmap = SelectObject(pThis->mhDC, pThis->mhBitmap);
    SetTextColor(pThis->mhDC, 0x00FFFFFF);
    SetBkColor(pThis->mhDC, 0);

    LOGFONTA lf;
    lf.lfHeight = 0;
    if (pThis->mhFont)
    {
        memset(&lf, 0, sizeof(lf));
        GetObjectA(pThis->mhFont, sizeof(lf), &lf);
    }

    if (lf.lfHeight == -dword_7E143C)
    {
        return;
    }

    if (pThis->mhFont)
    {
        DeleteObject(pThis->mhFont);
    }

    lf.lfHeight = -dword_7E143C;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = 0;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = 0;
    lstrcpyA(lf.lfFaceName, lpString2);

    pThis->mhFont = CreateFontIndirectA(&lf);
}

// FUNCTION: SWBF2 0x006f00d0
void RedLogitechLCD::DisconnectLCDPanel()
{
    RedLogitechLCD* pThis = this;
    if (pThis->mConnected)
    {
        lgLcdDisconnect(pThis->mConnectCtx.connection);
    }
    lgLcdDeInit();
    pThis->mConnected = false;
}

// FUNCTION: SWBF2 0x006f00f0
void RedLogitechLCD::ClearLCD()
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened)
    {
        return;
    }
    if (!pThis->mConnected)
    {
        return;
    }
    if (!pThis->mpBits)
    {
        return;
    }

    memset(pThis->mpBits, 0, pThis->mHeight * pThis->mWidth);
}

// FUNCTION: SWBF2 0x006f0120
void RedLogitechLCD::DrawLCDLine(int x0, int y0, int x1, int y1)
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened || !pThis->mConnected)
    {
        return;
    }
    pThis->PrepareLCDDrawing();
    HGDIOBJ hOldPen = SelectObject(pThis->mhDC, GetStockObject(WHITE_PEN));
    MoveToEx(pThis->mhDC, x0, y0, 0);
    LineTo(pThis->mhDC, x1, y1);
    SelectObject(pThis->mhDC, hOldPen);
}

// FUNCTION: SWBF2 0x006f0180
void RedLogitechLCD::DrawLCDText(char const* lpString, int x, int y)
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened)
    {
        return;
    }
    if (!pThis->mConnected)
    {
        return;
    }

    pThis->PrepareLCDDrawing();

    if (!pThis->mhFont)
    {
        return;
    }

    int iOldMapMode = SetMapMode(pThis->mhDC, MM_TEXT);
    int iOldBkMode = SetBkMode(pThis->mhDC, TRANSPARENT);
    HGDIOBJ hOldFont = SelectObject(pThis->mhDC, pThis->mhFont);

    TextOutA(pThis->mhDC, x, y, lpString, (int)strlen(lpString));

    SetMapMode(pThis->mhDC, iOldMapMode);
    SetBkMode(pThis->mhDC, iOldBkMode);
    SelectObject(pThis->mhDC, hOldFont);
}

// FUNCTION: SWBF2 0x006f0230
void RedLogitechLCD::DrawLCDWideText(const unsigned short* pwString, int x, int y)
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened || !pThis->mConnected)
    {
        return;
    }
    pThis->PrepareLCDDrawing();
    if (!pThis->mhFont)
    {
        return;
    }
    int iMapMode = SetMapMode(pThis->mhDC, MM_TEXT);
    int iBkMode = SetBkMode(pThis->mhDC, TRANSPARENT);
    HGDIOBJ hOldFont = SelectObject(pThis->mhDC, pThis->mhFont);
    const WCHAR* psz = (const WCHAR*)pwString;
    TextOutW(pThis->mhDC, x, y, psz, lstrlenW(psz));
    SetMapMode(pThis->mhDC, iMapMode);
    SetBkMode(pThis->mhDC, iBkMode);
    SelectObject(pThis->mhDC, hOldFont);
}

// FUNCTION: SWBF2 0x006f02d0
void RedLogitechLCD::DrawLCDWrappedText(const unsigned short* pwString)
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened || !pThis->mConnected)
    {
        return;
    }
    pThis->PrepareLCDDrawing();
    if (!pThis->mhFont)
    {
        return;
    }
    int iMapMode = SetMapMode(pThis->mhDC, MM_TEXT);
    int iBkMode = SetBkMode(pThis->mhDC, TRANSPARENT);
    HGDIOBJ hOldFont = SelectObject(pThis->mhDC, pThis->mhFont);
    RECT rc = { 0, 0, pThis->mWidth - 1, pThis->mHeight - 1 };
    const WCHAR* psz = (const WCHAR*)pwString;
    DrawTextW(pThis->mhDC, psz, lstrlenW(psz), &rc, DT_WORDBREAK);
    SetMapMode(pThis->mhDC, iMapMode);
    SetBkMode(pThis->mhDC, iBkMode);
    SelectObject(pThis->mhDC, hOldFont);
}

// FUNCTION: SWBF2 0x006f03b0
void RedLogitechLCD::PresentLCD()
{
    RedLogitechLCD* pThis = this;
    pThis->UpdateLCDPanel(-1.0f);
}

// FUNCTION: SWBF2 0x006f03c0
void RedLogitechLCD::ConnectLCDPanel(unsigned char ucPortIndex)
{
    RedLogitechLCD* pThis = this;
    pThis->mPortIndex = ucPortIndex;

    DWORD dwResult = lgLcdInit();
    pThis->mConnected = (dwResult == 0 || dwResult == LGLCD_E_ALREADY_INITIALIZED);

    if (pThis->mConnected)
    {
        pThis->mConnectCtx.isAutostartable = 0;
        pThis->mConnectCtx.appFriendlyName = "Star Wars Battlefront II";
        pThis->mConnectCtx.isPersistent = 1;
        pThis->mConnectCtx.configCallback = 0;
        pThis->mConnectCtx.configContext = 0;
        pThis->mConnectCtx.connection = -1;

        if (lgLcdConnectA(&pThis->mConnectCtx) != 0)
        {
            pThis->mConnected = false;
        }
    }
}

// FUNCTION: SWBF2 0x006f0430
void RedLogitechLCD::OpenLCDPanel()
{
    RedLogitechLCD* pThis = this;
    if (pThis->mOpened)
    {
        return;
    }
    if (!pThis->mConnected)
    {
        return;
    }

    pThis->mpBitmapInfo = 0;
    pThis->mpBitmap = 0;
    pThis->mhFont = 0;
    pThis->mSoftButtons = 0;
    pThis->mPrevSoftButtons = 0;

    lgLcdDeviceDesc desc;
    if (lgLcdEnumerate(pThis->mConnectCtx.connection, pThis->mPortIndex, &desc) != 0)
    {
        return;
    }

    pThis->mOpenCtx.connection = pThis->mConnectCtx.connection;
    pThis->mOpenCtx.index = pThis->mPortIndex;
    pThis->mOpenCtx.softbuttonsChangedCallback = 0;
    pThis->mOpenCtx.softbuttonsChangedContext = 0;
    pThis->mOpenCtx.device = -1;
    if (lgLcdOpen(&pThis->mOpenCtx) != 0)
    {
        return;
    }

    pThis->mpBitmap = new lgLcdBitmap160x43x1;
    pThis->mpBitmap->Format = LGLCD_BMP_FORMAT_160x43x1;
    lgLcdUpdateBitmap(pThis->mOpenCtx.device, pThis->mpBitmap, LGLCD_ASYNC_UPDATE_NORMAL);

    pThis->mhDC = CreateCompatibleDC(0);

    const int iInfoSize = sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD);
    pThis->mpBitmapInfo = (BITMAPINFO*)new BYTE[iInfoSize];
    pThis->mWidth = 160;
    pThis->mHeight = 43;
    memset(pThis->mpBitmapInfo, 0, iInfoSize);

    pThis->mpBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pThis->mpBitmapInfo->bmiHeader.biWidth = pThis->mWidth;
    pThis->mpBitmapInfo->bmiHeader.biHeight = -pThis->mHeight;
    pThis->mpBitmapInfo->bmiHeader.biPlanes = 1;
    pThis->mpBitmapInfo->bmiHeader.biBitCount = 8;
    pThis->mpBitmapInfo->bmiHeader.biCompression = BI_RGB;

    pThis->mpBitmapInfo->bmiHeader.biSizeImage = pThis->mpBitmapInfo->bmiHeader.biBitCount
                                                 * pThis->mpBitmapInfo->bmiHeader.biHeight
                                                 * pThis->mpBitmapInfo->bmiHeader.biWidth / 8;
    pThis->mpBitmapInfo->bmiHeader.biXPelsPerMeter = 3200;
    pThis->mpBitmapInfo->bmiHeader.biYPelsPerMeter = 3200;
    pThis->mpBitmapInfo->bmiHeader.biClrUsed = 256;
    pThis->mpBitmapInfo->bmiHeader.biClrImportant = 256;

    for (int i = 0; i < 256; i++)
    {
        BYTE cLevel = (i <= 128) ? 0 : 0xFF;
        pThis->mpBitmapInfo->bmiColors[i].rgbRed = cLevel;
        pThis->mpBitmapInfo->bmiColors[i].rgbGreen = cLevel;
        pThis->mpBitmapInfo->bmiColors[i].rgbBlue = cLevel;
        pThis->mpBitmapInfo->bmiColors[i].rgbReserved = 0;
    }

    pThis->mhBitmap = CreateDIBSection(pThis->mhDC, pThis->mpBitmapInfo, DIB_RGB_COLORS,
                                       &pThis->mpBits, 0, 0);
    pThis->mhOldBitmap = 0;
    pThis->mOpened = true;
}

// FUNCTION: SWBF2 0x006f0620
void RedLogitechLCD::CloseLCDPanel()
{
    RedLogitechLCD* pThis = this;
    if (pThis->mpBitmap)
    {
        delete pThis->mpBitmap;
        pThis->mpBitmap = 0;
    }
    if (pThis->mhBitmap)
    {
        DeleteObject(pThis->mhBitmap);
        pThis->mhBitmap = 0;
        pThis->mpBits = 0;
    }
    if (pThis->mpBitmapInfo)
    {
        delete[] (BYTE*)pThis->mpBitmapInfo;
        pThis->mpBitmapInfo = 0;
    }
    if (pThis->mhDC)
    {
        DeleteDC(pThis->mhDC);
        pThis->mhDC = 0;
    }
    if (pThis->mhFont)
    {
        DeleteObject(pThis->mhFont);
        pThis->mhFont = 0;
    }
    pThis->mhOldBitmap = 0;
    if (pThis->mOpened && pThis->mConnected)
    {
        lgLcdClose(pThis->mOpenCtx.device);
        pThis->mOpened = false;
    }
}

// FUNCTION: SWBF2 0x006f06c0
void RedLogitechLCD::UpdateLCDPanel(float fPriority)
{
    RedLogitechLCD* pThis = this;
    if (!pThis->mOpened)
    {
        return;
    }
    if (!pThis->mConnected)
    {
        return;
    }

    if (fPriority > 0.0f)
    {
        pThis->mPrevSoftButtons = pThis->mSoftButtons;
        lgLcdReadSoftButtons(pThis->mOpenCtx.device, &pThis->mSoftButtons);
    }

    if (!pThis->mpBitmap)
    {
        return;
    }
    if (!pThis->mpBits)
    {
        return;
    }

    if (pThis->mhOldBitmap)
    {
        GdiFlush();
        SelectObject(pThis->mhDC, pThis->mhOldBitmap);
        pThis->mhOldBitmap = 0;
        memcpy(pThis->mpBitmap->pixels, pThis->mpBits,
               pThis->mWidth * pThis->mHeight);
        memset(pThis->mpBits, 0, pThis->mWidth * pThis->mHeight);
    }

    lgLcdUpdateBitmap(pThis->mOpenCtx.device, pThis->mpBitmap,
                      0.0f > fPriority ? LGLCD_ASYNC_UPDATE_ALERT
                                       : LGLCD_SYNC_UPDATE_NORMAL);
}
