#pragma once

namespace DisplayCfg
{

const unsigned int kBorderless = 1;
const unsigned int kAdapter = 2;
const unsigned int kRefresh = 4;
const unsigned int kVsync = 8;
const unsigned int kFps = 16;
const unsigned int kNoWaitQuery = 32;
const unsigned int kNoDpi = 64;
const unsigned int kDisplayIni = 128;
const unsigned int kAdapterName = 256;

const unsigned int kHashBorderless = 0x49DA825A;
const unsigned int kHashAdapter = 0xE7CAE794;
const unsigned int kHashRefresh = 0xD4F26634;
const unsigned int kHashVsync = 0x06F8F066;
const unsigned int kHashFps = 0xBDD34AA8;
const unsigned int kHashNoWaitQuery = 0xFF222165;
const unsigned int kHashNoDpi = 0x5B1931B5;
const unsigned int kHashDisplayIni = 0x8CDD8845;

struct DisplaySettings
{
    bool bBorderless;
    unsigned int uiAdapter;
    char acAdapterName[64];
    unsigned int uiRefresh;
    unsigned int uiVsync;
    unsigned int uiFps;
    bool bNoWaitQuery;
    bool bNoDpi;
    char acDisplayIni[260];
    unsigned int uiSet;
    unsigned int uiCliMask;
};

void SetDefaults(DisplaySettings& kSettings);
bool ParseUInt(const char* pcText, unsigned int uiLo, unsigned int uiHi, unsigned int& uiOut);
bool ParseBool(const char* pcText, bool& bOut);
bool ParseVsync(const char* pcText, unsigned int& uiOut);
bool SetFromText(DisplaySettings& kSettings, unsigned int uiKey, const char* pcValue, const char* pcSource);
void SetFlag(DisplaySettings& kSettings, unsigned int uiKey);
void Merge(DisplaySettings& dst, const DisplaySettings& cli);
void LoadIni(DisplaySettings& kSettings, const char* pcPath);
bool SaveIni(const DisplaySettings& kSettings, const char* pcPath, bool bGoodFrame);
bool DisplayIniPath(char* pcPath, unsigned int uiSize);
bool SaveAdapter(unsigned int uiIndex, const char* pcName);
void Init();

extern DisplaySettings gCliDisplay;
extern DisplaySettings gIniDisplay;
extern DisplaySettings gDisplay;

}
