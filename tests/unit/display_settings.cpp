#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "testutil.h"
#include "RedEngineFL/Graphics/PC/pcDisplaySettings.h"

using namespace DisplayCfg;

namespace
{

const char* IniPath()
{
    static char acPath[MAX_PATH];
    GetFullPathNameA("display_settings_test.ini", MAX_PATH, acPath, 0);
    return acPath;
}

void WriteFile(const char* pcPath, const char* pcText, size_t uiLen)
{
    FILE* pFile = fopen(pcPath, "wb");
    if (pFile)
    {
        fwrite(pcText, 1, uiLen, pFile);
        fclose(pFile);
    }
}

void WriteText(const char* pcText)
{
    WriteFile(IniPath(), pcText, strlen(pcText));
}

void Cleanup()
{
    char acTemp[MAX_PATH + 8];
    DeleteFileA(IniPath());
    sprintf(acTemp, "%s.tmp", IniPath());
    DeleteFileA(acTemp);
}

bool Exists(const char* pcPath)
{
    return GetFileAttributesA(pcPath) != INVALID_FILE_ATTRIBUTES;
}

}

TEST_CASE("display settings parse uint is strict" * doctest::test_suite("unit"))
{
    unsigned int uiOut = 77;
    CHECK(ParseUInt("12", 0, 15, uiOut));
    CHECK(uiOut == 12);
    uiOut = 77;
    CHECK(!ParseUInt(0, 0, 15, uiOut));
    CHECK(!ParseUInt("", 0, 15, uiOut));
    CHECK(!ParseUInt("abc", 0, 15, uiOut));
    CHECK(!ParseUInt("-1", 0, 15, uiOut));
    CHECK(!ParseUInt("+1", 0, 15, uiOut));
    CHECK(!ParseUInt("1x", 0, 15, uiOut));
    CHECK(!ParseUInt(" 1", 0, 15, uiOut));
    CHECK(!ParseUInt("99999999999", 0, 0xFFFFFFFFu, uiOut));
    CHECK(!ParseUInt("16", 0, 15, uiOut));
    CHECK(uiOut == 77);
}

TEST_CASE("display settings parse bool and vsync" * doctest::test_suite("unit"))
{
    bool bOut = false;
    CHECK(ParseBool("ON", bOut));
    CHECK(bOut);
    CHECK(ParseBool("False", bOut));
    CHECK(!bOut);
    CHECK(!ParseBool("yes", bOut));
    CHECK(!ParseBool("", bOut));
    unsigned int uiOut = 9;
    CHECK(ParseVsync("half", uiOut));
    CHECK(uiOut == 2);
    CHECK(ParseVsync("off", uiOut));
    CHECK(uiOut == 0);
    CHECK(!ParseVsync("3", uiOut));
}

TEST_CASE("display settings defaults" * doctest::test_suite("unit"))
{
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    CHECK(kSettings.uiVsync == 1);
    CHECK(kSettings.uiFps == 80);
    CHECK(kSettings.uiRefresh == 0);
    CHECK(kSettings.uiAdapter == 0);
    CHECK(!kSettings.bBorderless);
    CHECK(kSettings.uiSet == 0);
}

TEST_CASE("display settings bad value keeps default and logs once" * doctest::test_suite("unit"))
{
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    LogCapture capture;
    CHECK(!SetFromText(kSettings, kRefresh, "abc", "command line"));
    CHECK(capture.Contains("display: bad Refresh 'abc'"));
    CHECK(kSettings.uiRefresh == 0);
    CHECK((kSettings.uiSet & kRefresh) == 0);
    CHECK(!SetFromText(kSettings, kAdapter, "16", "command line"));
    CHECK(kSettings.uiAdapter == 0);
    CHECK(SetFromText(kSettings, kFps, "120", "command line"));
    CHECK(kSettings.uiFps == 120);
    CHECK((kSettings.uiSet & kFps) != 0);
}

TEST_CASE("display settings missing ini is silent" * doctest::test_suite("unit"))
{
    Cleanup();
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    LogCapture capture;
    LoadIni(kSettings, IniPath());
    CHECK(!capture.Contains("display:"));
    CHECK(kSettings.uiSet == 0);
    CHECK(kSettings.uiFps == 80);
}

TEST_CASE("display settings ini loads values" * doctest::test_suite("unit"))
{
    WriteText("[Display]\r\nRefresh=60\r\nVsync=half\r\nAdapterName=\\\\.\\DISPLAY2\r\nBorderless=1\r\n");
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    LoadIni(kSettings, IniPath());
    Cleanup();
    CHECK(kSettings.uiRefresh == 60);
    CHECK(kSettings.uiVsync == 2);
    CHECK(strcmp(kSettings.acAdapterName, "\\\\.\\DISPLAY2") == 0);
    CHECK(kSettings.bBorderless);
}

TEST_CASE("display settings garbage ini loads nothing" * doctest::test_suite("unit"))
{
    char acGarbage[5120];
    unsigned int uiSeed = 12345;
    for (size_t i = 0; i < sizeof(acGarbage); ++i)
    {
        uiSeed = uiSeed * 1664525u + 1013904223u;
        acGarbage[i] = (char)(1 + ((uiSeed >> 16) % 254));
        if (acGarbage[i] == '[' || acGarbage[i] == '=')
        {
            acGarbage[i] = 'x';
        }
    }
    WriteFile(IniPath(), acGarbage, sizeof(acGarbage));
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    LoadIni(kSettings, IniPath());
    Cleanup();
    CHECK(kSettings.uiSet == 0);
    CHECK(kSettings.uiFps == 80);
}

TEST_CASE("display settings bad ini keys warn and keep defaults" * doctest::test_suite("unit"))
{
    char acBig[6000];
    strcpy(acBig, "[Display]\r\nRefresh=");
    size_t uiLen = strlen(acBig);
    memset(acBig + uiLen, 'x', 5000);
    strcpy(acBig + uiLen + 5000, "\r\nFps=abc\r\nAdapter=99\r\n");
    WriteText(acBig);
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    LogCapture capture;
    LoadIni(kSettings, IniPath());
    Cleanup();
    CHECK(capture.Contains("display: bad Refresh"));
    CHECK(capture.Contains("display: bad Fps 'abc'"));
    CHECK(capture.Contains("display: bad Adapter '99'"));
    CHECK(kSettings.uiSet == 0);
    CHECK(kSettings.uiFps == 80);
}

TEST_CASE("display settings merge precedence" * doctest::test_suite("unit"))
{
    DisplaySettings kIni;
    SetDefaults(kIni);
    SetFromText(kIni, kRefresh, "60", "display.ini");
    SetFromText(kIni, kFps, "100", "display.ini");
    DisplaySettings kCli;
    SetDefaults(kCli);
    SetFromText(kCli, kRefresh, "75", "command line");
    DisplaySettings kFinal = kIni;
    Merge(kFinal, kCli);
    CHECK(kFinal.uiRefresh == 75);
    CHECK(kFinal.uiFps == 100);
    CHECK(kFinal.uiVsync == 1);
    CHECK((kFinal.uiCliMask & kRefresh) != 0);
    CHECK((kFinal.uiCliMask & kFps) == 0);
}

TEST_CASE("display settings save gate refuses without a good frame" * doctest::test_suite("unit"))
{
    Cleanup();
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    CHECK(!SaveIni(kSettings, IniPath(), false));
    CHECK(!Exists(IniPath()));
}

TEST_CASE("display settings save round trips and leaves no temp" * doctest::test_suite("unit"))
{
    Cleanup();
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    SetFromText(kSettings, kRefresh, "144", "test");
    SetFromText(kSettings, kVsync, "half", "test");
    SetFromText(kSettings, kAdapterName, "\\\\.\\DISPLAY3", "test");
    SetFlag(kSettings, kBorderless);
    CHECK(SaveIni(kSettings, IniPath(), true));
    char acTemp[MAX_PATH + 8];
    sprintf(acTemp, "%s.tmp", IniPath());
    CHECK(!Exists(acTemp));
    DisplaySettings kLoaded;
    SetDefaults(kLoaded);
    LoadIni(kLoaded, IniPath());
    Cleanup();
    CHECK(kLoaded.uiRefresh == 144);
    CHECK(kLoaded.uiVsync == 2);
    CHECK(strcmp(kLoaded.acAdapterName, "\\\\.\\DISPLAY3") == 0);
    CHECK(kLoaded.bBorderless);
    CHECK(kLoaded.uiFps == 80);
}

TEST_CASE("display settings save skips command line values" * doctest::test_suite("unit"))
{
    WriteText("[Display]\r\nRefresh=60\r\nFps=100\r\n");
    DisplaySettings kIni;
    SetDefaults(kIni);
    LoadIni(kIni, IniPath());
    DisplaySettings kCli;
    SetDefaults(kCli);
    SetFromText(kCli, kRefresh, "75", "command line");
    SetFromText(kCli, kFps, "30", "command line");
    DisplaySettings kFinal = kIni;
    Merge(kFinal, kCli);
    SetFromText(kFinal, kVsync, "0", "test");
    CHECK(SaveIni(kFinal, IniPath(), true));
    DisplaySettings kLoaded;
    SetDefaults(kLoaded);
    LoadIni(kLoaded, IniPath());
    Cleanup();
    CHECK(kLoaded.uiRefresh == 60);
    CHECK(kLoaded.uiFps == 100);
    CHECK(kLoaded.uiVsync == 0);
}

TEST_CASE("display settings save never writes unset keys" * doctest::test_suite("unit"))
{
    Cleanup();
    DisplaySettings kSettings;
    SetDefaults(kSettings);
    SetFromText(kSettings, kRefresh, "144", "test");
    CHECK(SaveIni(kSettings, IniPath(), true));
    DisplaySettings kLoaded;
    SetDefaults(kLoaded);
    LoadIni(kLoaded, IniPath());
    Cleanup();
    CHECK(kLoaded.uiRefresh == 144);
    CHECK((kLoaded.uiSet & kVsync) == 0);
    CHECK((kLoaded.uiSet & kFps) == 0);
    CHECK((kLoaded.uiSet & kAdapter) == 0);
}

TEST_CASE("display settings init merges defaults ini and command line" * doctest::test_suite("unit"))
{
    WriteText("[Display]\r\nRefresh=60\r\nFps=100\r\n");
    SetDefaults(gCliDisplay);
    SetFromText(gCliDisplay, kRefresh, "75", "command line");
    SetFromText(gCliDisplay, kDisplayIni, IniPath(), "command line");
    Init();
    Cleanup();
    CHECK(gDisplay.uiRefresh == 75);
    CHECK(gDisplay.uiFps == 100);
    CHECK(gDisplay.uiVsync == 1);
    SetDefaults(gCliDisplay);
}
