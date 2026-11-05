#pragma once

#include <stdio.h>

class SaveFile
{
public:

    static void __cdecl SplitMetagameFilename(unsigned short const* pusFilename,
                                              unsigned short* pusName,
                                              unsigned short* pusDate,
                                              char* pcShortDate,
                                              unsigned int uiGameDateLen,
                                              unsigned int uiShortDateLen);

    static void MakeMetagameFilename(unsigned short* pusDest, unsigned short const* pusSource,
                                     unsigned int uiFilenameLen);

protected:

    static void TimeToString(unsigned short* pusDest, int iYear, int iMonth, int iDay,
                            int iHour, int iMinute, int iSecond, unsigned int uiOutLen);

    static void __cdecl StringToTime(unsigned short const* pusString, int& iYear,
                                     int& iMonth, int& iDay, int& iHour,
                                     int& iMinute, int& iSecond);

public:

    void SetFileInfo(unsigned short const* pusFileName, int iType);
    SaveFile();

    unsigned short const* GetGameName();
    int GetType();

    unsigned short mFileName[260];
    unsigned short mGameName[260];
    FILE* mFileHandle;
    int mType;
};
