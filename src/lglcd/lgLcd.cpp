#include <string.h>

#include "lgLcd.h"
#include "RedEngineFL/Memory/engineAlloc.h"

// GLOBAL: SWBF2 0x01e2fd04
int dword_1E2FD04;

// GLOBAL: SWBF2 0x01fac3b0
lgLcdInterface* dword_1FAC3B0;

// GLOBAL: SWBF2 0x01e2fbf8
HMODULE hLibModule;
// GLOBAL: SWBF2 0x01e2fbfc
void* dword_1E2FBFC;

// GLOBAL: SWBF2 0x007e57c0
char* lpSubKey =
    "SOFTWARE\\Classes\\CLSID\\{FE750200-B72E-11d9-829B-0050DA1A72D3}\\ServerBinary";

char Data[0x104];

char ProcName[] = "GetInterface";

typedef lgLcdInterface* (WINAPI *GetInterfaceFn)(int iVersion);

// FUNCTION: SWBF2 0x004012d4
BOOL sub_4012D4()
{
    BOOL bOk = FALSE;

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        DWORD dwType;
        DWORD cbData = sizeof(Data);
        if (RegQueryValueExA(hKey, 0, 0, &dwType, (LPBYTE)Data, &cbData) == ERROR_SUCCESS)
        {
            hLibModule = LoadLibraryA(Data);
            if (hLibModule)
            {
                dword_1E2FBFC = GetProcAddress(hLibModule, ProcName);
                if (dword_1E2FBFC)
                {
                    dword_1FAC3B0 = ((GetInterfaceFn)dword_1E2FBFC)(1);
                    bOk = (dword_1FAC3B0 != 0);
                }

                if (!bOk)
                {
                    sub_40136C();
                }
            }
        }
        RegCloseKey(hKey);
    }

    return bOk;
}

// FUNCTION: SWBF2 0x0040136c
BOOL sub_40136C()
{
    if (hLibModule)
    {
        FreeLibrary(hLibModule);
        hLibModule = 0;
    }
    dword_1FAC3B0 = 0;

    return TRUE;
}

// FUNCTION: SWBF2 0x004012a5
DWORD WINAPI lgLcdInit()
{
    if (dword_1E2FD04)
    {
        return LGLCD_E_ALREADY_INITIALIZED;
    }

    if (!sub_4012D4())
    {
        return LGLCD_E_SERVER_UNAVAILABLE;
    }

    dword_1E2FD04 = 1;
    return dword_1FAC3B0->Init();
}

// FUNCTION: SWBF2 0x004013b5
DWORD WINAPI lgLcdConnectA(lgLcdConnectContextA* pCtx)
{
    if (!dword_1E2FD04)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return dword_1FAC3B0->Connect(pCtx);
}

// FUNCTION: SWBF2 0x004013f3
DWORD WINAPI lgLcdEnumerate(int iConnection, int iIndex, lgLcdDeviceDesc* pDesc)
{
    if (!dword_1E2FD04)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return dword_1FAC3B0->Enumerate(iConnection, iIndex, pDesc);
}

// FUNCTION: SWBF2 0x0040141a
DWORD WINAPI lgLcdOpen(lgLcdOpenContext* pCtx)
{
    if (!dword_1E2FD04)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return dword_1FAC3B0->Open(pCtx);
}

// FUNCTION: SWBF2 0x00401458
DWORD WINAPI lgLcdReadSoftButtons(int iDevice, DWORD* pdwButtons)
{
    if (!dword_1E2FD04)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return dword_1FAC3B0->ReadSoftButtons(iDevice, pdwButtons);
}

// FUNCTION: SWBF2 0x0040147b
DWORD WINAPI lgLcdUpdateBitmap(int iDevice, void const* pBitmap, DWORD dwPriority)
{
    if (!dword_1E2FD04)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return dword_1FAC3B0->UpdateBitmap(iDevice, pBitmap, dwPriority);
}

























// FUNCTION: SWBF2 0x0040138e
DWORD WINAPI lgLcdDeInit()
{
    if (!dword_1E2FD04)
    {
        return 0;
    }
    DWORD dwResult = dword_1FAC3B0->DeInit();
    sub_40136C();
    dword_1E2FD04 = 0;
    return dwResult;
}

// FUNCTION: SWBF2 0x004013d4
DWORD WINAPI lgLcdDisconnect(int iConnection)
{
    return dword_1E2FD04 ? dword_1FAC3B0->Disconnect(iConnection) : LGLCD_E_SERVICE_NOT_ACTIVE;
}

// FUNCTION: SWBF2 0x00401439
DWORD WINAPI lgLcdClose(int iDevice)
{
    typedef DWORD (WINAPI* CloseFn)(int);
    return dword_1E2FD04 ? ((CloseFn)dword_1FAC3B0->mUnknown001C)(iDevice) : LGLCD_E_SERVICE_NOT_ACTIVE;
}
