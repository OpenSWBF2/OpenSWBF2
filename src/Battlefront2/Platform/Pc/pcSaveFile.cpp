#include "pcSaveFile.h"

#include <stdio.h>
#include <windows.h>

#include "PebbleFL/Common/PblUnicode.h"

// FUNCTION: SWBF2 0x00426710
void SaveFile::SplitMetagameFilename(unsigned short const* pusFilename,
                                     unsigned short* pusName,
                                     unsigned short* pusDate,
                                     char* pcShortDate,
                                     unsigned int uiGameDateLen,
                                     unsigned int uiShortDateLen)
{
    (void)pcShortDate;
    (void)uiShortDateLen;

    const unsigned int uiLen = PblUnicode::ustrlen(pusFilename);

    if (uiLen < 8)
    {
        if (pusName)
        {
            pusName[0] = 0;
        }
        if (pusDate)
        {
            pusDate[0] = 0;
        }
        return;
    }

    if (pusName)
    {
        const unsigned int uiNameLen = uiLen - 8 < 127 ? uiLen - 8 : 127;
        PblUnicode::ustrlcpy(pusName, pusFilename, uiNameLen + 1);
    }

    if (pusDate)
    {
        int iYear, iMonth, iDay, iHour, iMinute, iSecond;
        StringToTime(&pusFilename[uiLen - 8], iYear, iMonth, iDay, iHour, iMinute,
                     iSecond);

        char acDate[64];
        _snprintf(acDate, 64, "%04d-%02d-%02d %02d:%02d.%02d", iYear, iMonth, iDay,
                  iHour, iMinute, iSecond);
        PblUnicode::ustrncnv(pusDate, acDate, uiGameDateLen);
    }
}

// FUNCTION: SWBF2 0x00426630
SaveFile::SaveFile()
{
    SetFileInfo((const unsigned short*)L"", 0);
    mFileHandle = 0;
}

// FUNCTION: SWBF2 0x00426650
void SaveFile::SetFileInfo(unsigned short const* pusFileName, int iType)
{
    PblUnicode::ustrlcpy(mFileName, pusFileName, sizeof(mFileName) / sizeof(mFileName[0]));
    PblUnicode::ustrlcpy(mGameName, pusFileName, sizeof(mGameName) / sizeof(mGameName[0]));
    mType = iType;
}

// FUNCTION: SWBF2 0x00426690
unsigned short const* SaveFile::GetGameName()
{
    return mGameName;
}

// FUNCTION: SWBF2 0x004266a0
int SaveFile::GetType()
{
    return mType;
}

// FUNCTION: SWBF2 0x004266b0
void SaveFile::MakeMetagameFilename(unsigned short* pusDest, unsigned short const* pusSource,
                                     unsigned int uiFilenameLen)
{
    unsigned short ausStamp[16];
    SYSTEMTIME localTime;
    PblUnicode::ustrncpy(pusDest, pusSource, uiFilenameLen);
    GetLocalTime(&localTime);
    SaveFile::TimeToString(ausStamp, localTime.wYear, localTime.wMonth, localTime.wDay,
               localTime.wHour, localTime.wMinute, localTime.wSecond, 16);
    PblUnicode::ustrlcat(pusDest, ausStamp, uiFilenameLen);
}
