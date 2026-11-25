#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "lglcd/lgLcd.h"
#include "testutil.h"

extern char* lpSubKey;

TEST_CASE_NAMED(FLglcdOffTest1, "Unit::LglcdOff::logitech lcd stays off without its server", "[SmokeFilter]")
{
    static char acAbsentKey[] =
        "SOFTWARE\\Classes\\CLSID\\{73384F02-333F-42FC-A2AD-A387DBB94DCF}\\ServerBinary";
    char* pcSavedKey = lpSubKey;
    lpSubKey = acAbsentKey;
    g_lgLcdLibIsInitialized = 0;
    hLibModule = 0;
    g_lgLcdLibLinkedRemoteInterface = 0;
    DWORD dwFirst = 0;
    DWORD dwSecond = 0;
    int iLines = 0;
    bool bFirstModule = false;
    {
        LogCapture kLog;
        dwFirst = lgLcdInit();
        bFirstModule = hLibModule != 0;
        dwSecond = lgLcdInit();
        iLines = kLog.Count("");
    }
    const int iInitialized = g_lgLcdLibIsInitialized;
    const bool bModule = hLibModule != 0;
    const bool bInterface = g_lgLcdLibLinkedRemoteInterface != 0;
    lpSubKey = pcSavedKey;
    g_lgLcdLibIsInitialized = 0;
    hLibModule = 0;
    g_lgLcdLibLinkedRemoteInterface = 0;
    CHECK(dwFirst == LGLCD_E_SERVER_UNAVAILABLE);
    CHECK(dwSecond == LGLCD_E_SERVER_UNAVAILABLE);
    CHECK(iInitialized == 0);
    CHECK_FALSE(bFirstModule);
    CHECK_FALSE(bModule);
    CHECK_FALSE(bInterface);
    CHECK(iLines == 0);
    CHECK(lgLcdConnectA(0) == LGLCD_E_SERVICE_NOT_ACTIVE);
}
