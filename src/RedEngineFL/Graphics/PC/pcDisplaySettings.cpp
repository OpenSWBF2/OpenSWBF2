#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "pcDisplaySettings.h"

#include "RedEngineFL/Util/RedLog.h"

namespace DisplayCfg
{

DisplaySettings gCliDisplay;
DisplaySettings gIniDisplay;
DisplaySettings gDisplay;

static const char* KeyName(unsigned int uiKey)
{
    switch (uiKey)
    {
    case kBorderless:
        return "Borderless";
    case kAdapter:
        return "Adapter";
    case kRefresh:
        return "Refresh";
    case kVsync:
        return "Vsync";
    case kFps:
        return "Fps";
    case kNoWaitQuery:
        return "NoWaitQuery";
    case kNoDpi:
        return "NoDpi";
    case kDisplayIni:
        return "DisplayIni";
    case kAdapterName:
        return "AdapterName";
    }
    return "?";
}

static bool ShouldWrite(const DisplaySettings& s, unsigned int uiKey)
{
    return (s.uiCliMask & uiKey) == 0;
}

static bool FullPath(const char* pcPath, char* pcOut)
{
    if (pcPath == 0 || pcPath[0] == '\0' || strlen(pcPath) >= MAX_PATH)
    {
        return false;
    }
    DWORD dwLen = GetFullPathNameA(pcPath, MAX_PATH, pcOut, 0);
    return dwLen > 0 && dwLen < MAX_PATH;
}

static bool PutKey(const char* pcKey, const char* pcValue, const char* pcFile)
{
    return WritePrivateProfileStringA("Display", pcKey, pcValue, pcFile) != 0;
}

void SetDefaults(DisplaySettings& kSettings)
{
    memset(&kSettings, 0, sizeof(kSettings));
    kSettings.uiVsync = 1;
    kSettings.uiFps = 80;
}

bool ParseUInt(const char* pcText, unsigned int uiLo, unsigned int uiHi, unsigned int& uiOut)
{
    if (pcText == 0 || pcText[0] == '\0')
    {
        return false;
    }
    unsigned int uiValue = 0;
    for (const char* pc = pcText; *pc != '\0'; ++pc)
    {
        if (*pc < '0' || *pc > '9')
        {
            return false;
        }
        unsigned int uiDigit = (unsigned int)(*pc - '0');
        if (uiValue > (0xFFFFFFFFu - uiDigit) / 10)
        {
            return false;
        }
        uiValue = uiValue * 10 + uiDigit;
    }
    if (uiValue < uiLo || uiValue > uiHi)
    {
        return false;
    }
    uiOut = uiValue;
    return true;
}

bool ParseBool(const char* pcText, bool& bOut)
{
    if (pcText == 0)
    {
        return false;
    }
    if (strcmp(pcText, "1") == 0 || _stricmp(pcText, "on") == 0 || _stricmp(pcText, "true") == 0)
    {
        bOut = true;
        return true;
    }
    if (strcmp(pcText, "0") == 0 || _stricmp(pcText, "off") == 0 || _stricmp(pcText, "false") == 0)
    {
        bOut = false;
        return true;
    }
    return false;
}

bool ParseVsync(const char* pcText, unsigned int& uiOut)
{
    if (pcText == 0)
    {
        return false;
    }
    if (strcmp(pcText, "0") == 0 || _stricmp(pcText, "off") == 0)
    {
        uiOut = 0;
        return true;
    }
    if (strcmp(pcText, "1") == 0 || _stricmp(pcText, "on") == 0)
    {
        uiOut = 1;
        return true;
    }
    if (strcmp(pcText, "2") == 0 || _stricmp(pcText, "half") == 0)
    {
        uiOut = 2;
        return true;
    }
    return false;
}

bool SetFromText(DisplaySettings& kSettings, unsigned int uiKey, const char* pcValue, const char* pcSource)
{
    bool bValid = false;
    bool bBool = false;
    unsigned int uiValue = 0;
    switch (uiKey)
    {
    case kBorderless:
        bValid = ParseBool(pcValue, bBool);
        if (bValid)
        {
            kSettings.bBorderless = bBool;
        }
        break;
    case kAdapter:
        bValid = ParseUInt(pcValue, 0, 15, uiValue);
        if (bValid)
        {
            kSettings.uiAdapter = uiValue;
        }
        break;
    case kRefresh:
        bValid = ParseUInt(pcValue, 0, 1000, uiValue);
        if (bValid)
        {
            kSettings.uiRefresh = uiValue;
        }
        break;
    case kVsync:
        bValid = ParseVsync(pcValue, uiValue);
        if (bValid)
        {
            kSettings.uiVsync = uiValue;
        }
        break;
    case kFps:
        bValid = ParseUInt(pcValue, 0, 1000, uiValue);
        if (bValid)
        {
            kSettings.uiFps = uiValue;
        }
        break;
    case kNoWaitQuery:
        bValid = ParseBool(pcValue, bBool);
        if (bValid)
        {
            kSettings.bNoWaitQuery = bBool;
        }
        break;
    case kNoDpi:
        bValid = ParseBool(pcValue, bBool);
        if (bValid)
        {
            kSettings.bNoDpi = bBool;
        }
        break;
    case kDisplayIni:
        bValid = pcValue != 0 && pcValue[0] != '\0' && strlen(pcValue) < sizeof(kSettings.acDisplayIni);
        if (bValid)
        {
            strcpy(kSettings.acDisplayIni, pcValue);
        }
        break;
    case kAdapterName:
        bValid = pcValue != 0 && strlen(pcValue) < sizeof(kSettings.acAdapterName);
        if (bValid)
        {
            strcpy(kSettings.acAdapterName, pcValue);
        }
        break;
    }
    if (!bValid)
    {
        LOG(LogRender, Warning, "display: bad %s '%.64s' from %s, using default", KeyName(uiKey),
            pcValue ? pcValue : "", pcSource ? pcSource : "?");
        return false;
    }
    kSettings.uiSet |= uiKey;
    return true;
}

void SetFlag(DisplaySettings& kSettings, unsigned int uiKey)
{
    switch (uiKey)
    {
    case kBorderless:
        kSettings.bBorderless = true;
        break;
    case kNoWaitQuery:
        kSettings.bNoWaitQuery = true;
        break;
    case kNoDpi:
        kSettings.bNoDpi = true;
        break;
    default:
        return;
    }
    kSettings.uiSet |= uiKey;
}

void Merge(DisplaySettings& dst, const DisplaySettings& cli)
{
    if (cli.uiSet & kBorderless)
    {
        dst.bBorderless = cli.bBorderless;
    }
    if (cli.uiSet & kAdapter)
    {
        dst.uiAdapter = cli.uiAdapter;
    }
    if (cli.uiSet & kAdapterName)
    {
        strcpy(dst.acAdapterName, cli.acAdapterName);
    }
    if (cli.uiSet & kRefresh)
    {
        dst.uiRefresh = cli.uiRefresh;
    }
    if (cli.uiSet & kVsync)
    {
        dst.uiVsync = cli.uiVsync;
    }
    if (cli.uiSet & kFps)
    {
        dst.uiFps = cli.uiFps;
    }
    if (cli.uiSet & kNoWaitQuery)
    {
        dst.bNoWaitQuery = cli.bNoWaitQuery;
    }
    if (cli.uiSet & kNoDpi)
    {
        dst.bNoDpi = cli.bNoDpi;
    }
    if (cli.uiSet & kDisplayIni)
    {
        strcpy(dst.acDisplayIni, cli.acDisplayIni);
    }
    dst.uiSet |= cli.uiSet;
    dst.uiCliMask |= cli.uiSet;
}

void LoadIni(DisplaySettings& kSettings, const char* pcPath)
{
    char acFull[MAX_PATH];
    if (!FullPath(pcPath, acFull) || GetFileAttributesA(acFull) == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }
    static const unsigned int auiKeys[] = {kAdapter, kAdapterName, kBorderless, kRefresh,
                                           kVsync,   kFps,         kNoWaitQuery, kNoDpi};
    for (unsigned int i = 0; i < sizeof(auiKeys) / sizeof(auiKeys[0]); ++i)
    {
        char acValue[256];
        GetPrivateProfileStringA("Display", KeyName(auiKeys[i]), "\x7f", acValue, sizeof(acValue), acFull);
        acValue[sizeof(acValue) - 1] = '\0';
        if (strcmp(acValue, "\x7f") == 0)
        {
            continue;
        }
        SetFromText(kSettings, auiKeys[i], acValue, "display.ini");
    }
}

bool SaveIni(const DisplaySettings& kSettings, const char* pcPath, bool bGoodFrame)
{
    if (!bGoodFrame)
    {
        return false;
    }
    char acFull[MAX_PATH];
    char acTemp[MAX_PATH + 8];
    if (!FullPath(pcPath, acFull) || strlen(acFull) + 4 >= sizeof(acTemp))
    {
        return false;
    }
    sprintf(acTemp, "%s.tmp", acFull);
    DeleteFileA(acTemp);
    CopyFileA(acFull, acTemp, FALSE);
    bool bOk = true;
    char acValue[32];
    if (ShouldWrite(kSettings, kAdapter))
    {
        sprintf(acValue, "%u", kSettings.uiAdapter);
        bOk = PutKey("Adapter", acValue, acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kAdapterName))
    {
        bOk = PutKey("AdapterName", kSettings.acAdapterName, acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kBorderless))
    {
        bOk = PutKey("Borderless", kSettings.bBorderless ? "1" : "0", acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kRefresh))
    {
        sprintf(acValue, "%u", kSettings.uiRefresh);
        bOk = PutKey("Refresh", acValue, acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kVsync))
    {
        sprintf(acValue, "%u", kSettings.uiVsync);
        bOk = PutKey("Vsync", acValue, acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kFps))
    {
        sprintf(acValue, "%u", kSettings.uiFps);
        bOk = PutKey("Fps", acValue, acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kNoWaitQuery))
    {
        bOk = PutKey("NoWaitQuery", kSettings.bNoWaitQuery ? "1" : "0", acTemp) && bOk;
    }
    if (ShouldWrite(kSettings, kNoDpi))
    {
        bOk = PutKey("NoDpi", kSettings.bNoDpi ? "1" : "0", acTemp) && bOk;
    }
    WritePrivateProfileStringA(0, 0, 0, acTemp);
    if (bOk)
    {
        bOk = MoveFileExA(acTemp, acFull, MOVEFILE_REPLACE_EXISTING) != 0;
    }
    if (!bOk)
    {
        DeleteFileA(acTemp);
    }
    return bOk;
}

void Init()
{
    SetDefaults(gIniDisplay);
    char acPath[MAX_PATH];
    bool bHavePath = false;
    if (gCliDisplay.acDisplayIni[0] != '\0')
    {
        bHavePath = FullPath(gCliDisplay.acDisplayIni, acPath);
    }
    else
    {
        DWORD dwLen = GetModuleFileNameA(0, acPath, MAX_PATH);
        if (dwLen > 0 && dwLen < MAX_PATH)
        {
            char* pcSlash = strrchr(acPath, '\\');
            if (pcSlash != 0 && strlen(acPath) + 11 < MAX_PATH)
            {
                strcpy(pcSlash + 1, "display.ini");
                bHavePath = true;
            }
        }
    }
    if (bHavePath)
    {
        LoadIni(gIniDisplay, acPath);
    }
    gDisplay = gIniDisplay;
    Merge(gDisplay, gCliDisplay);
}

}
