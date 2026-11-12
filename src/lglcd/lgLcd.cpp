#include <string.h>

#include "lgLcd.h"
#include "RedEngineFL/Memory/engineAlloc.h"

int g_lgLcdLibIsInitialized;

lgLcdInterface* g_lgLcdLibLinkedRemoteInterface;

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

BOOL LinkLgLcdDLL()
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
                    g_lgLcdLibLinkedRemoteInterface = ((GetInterfaceFn)dword_1E2FBFC)(1);
                    bOk = (g_lgLcdLibLinkedRemoteInterface != 0);
                }

                if (!bOk)
                {
                    UnlinkLgLcdDLL();
                }
            }
        }
        RegCloseKey(hKey);
    }

    return bOk;
}

BOOL UnlinkLgLcdDLL()
{
    if (hLibModule)
    {
        FreeLibrary(hLibModule);
        hLibModule = 0;
    }
    g_lgLcdLibLinkedRemoteInterface = 0;

    return TRUE;
}

DWORD WINAPI lgLcdInit()
{
    if (g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_ALREADY_INITIALIZED;
    }

    if (!LinkLgLcdDLL())
    {
        return LGLCD_E_SERVER_UNAVAILABLE;
    }

    g_lgLcdLibIsInitialized = 1;
    return g_lgLcdLibLinkedRemoteInterface->Init();
}

DWORD WINAPI lgLcdConnectA(lgLcdConnectContextA* pCtx)
{
    if (!g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return g_lgLcdLibLinkedRemoteInterface->Connect(pCtx);
}

DWORD WINAPI lgLcdEnumerate(int iConnection, int iIndex, lgLcdDeviceDesc* pDesc)
{
    if (!g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return g_lgLcdLibLinkedRemoteInterface->Enumerate(iConnection, iIndex, pDesc);
}

DWORD WINAPI lgLcdOpen(lgLcdOpenContext* pCtx)
{
    if (!g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return g_lgLcdLibLinkedRemoteInterface->Open(pCtx);
}

DWORD WINAPI lgLcdReadSoftButtons(int iDevice, DWORD* pdwButtons)
{
    if (!g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return g_lgLcdLibLinkedRemoteInterface->ReadSoftButtons(iDevice, pdwButtons);
}

DWORD WINAPI lgLcdUpdateBitmap(int iDevice, void const* pBitmap, DWORD dwPriority)
{
    if (!g_lgLcdLibIsInitialized)
    {
        return LGLCD_E_SERVICE_NOT_ACTIVE;
    }
    return g_lgLcdLibLinkedRemoteInterface->UpdateBitmap(iDevice, pBitmap, dwPriority);
}

























DWORD WINAPI lgLcdDeInit()
{
    if (!g_lgLcdLibIsInitialized)
    {
        return 0;
    }
    DWORD dwResult = g_lgLcdLibLinkedRemoteInterface->DeInit();
    UnlinkLgLcdDLL();
    g_lgLcdLibIsInitialized = 0;
    return dwResult;
}

DWORD WINAPI lgLcdDisconnect(int iConnection)
{
    return g_lgLcdLibIsInitialized ? g_lgLcdLibLinkedRemoteInterface->Disconnect(iConnection) : LGLCD_E_SERVICE_NOT_ACTIVE;
}

DWORD WINAPI lgLcdClose(int iDevice)
{
    typedef DWORD (WINAPI* CloseFn)(int);
    return g_lgLcdLibIsInitialized ? ((CloseFn)g_lgLcdLibLinkedRemoteInterface->mUnknown001C)(iDevice) : LGLCD_E_SERVICE_NOT_ACTIVE;
}
