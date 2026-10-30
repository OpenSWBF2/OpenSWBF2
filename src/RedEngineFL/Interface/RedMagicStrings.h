#pragma once

#include "PebbleFL/Common/PblUnicode.h"

inline unsigned int ReplaceMagicStrings(const unsigned short* pcSource, unsigned short* pcDest, unsigned int uiDestCount,
                                        unsigned short (*pfnFind)(const char* pcName))
{
    if (!uiDestCount)
    {
        return 0;
    }

    unsigned short acMagic[256];
    char acMagicA[256];
    unsigned int uiCount = 0;
    unsigned int uiOut = 0;
    bool bInMagic = false;
    bool bOverlong = false;
    unsigned short uiLen = 0;

    for (const unsigned short* pc = pcSource; *pc && uiOut < uiDestCount - 1; ++pc)
    {
        if (*pc == '{')
        {
            bInMagic = true;
            bOverlong = false;
            uiLen = 0;
        }
        else if (bInMagic)
        {
            if (*pc == '}')
            {
                bInMagic = false;
                unsigned short c = 0;
                if (!bOverlong)
                {
                    acMagic[uiLen] = 0;
                    PblUnicode::ustrncnv(acMagicA, acMagic, 256);
                    c = pfnFind(acMagicA);
                }
                if (c)
                {
                    pcDest[uiOut++] = c;
                    ++uiCount;
                }
                else
                {
                    pcDest[uiOut++] = ' ';
                }
            }
            else if (uiLen < 255)
            {
                acMagic[uiLen++] = *pc;
            }
            else
            {
                bOverlong = true;
            }
        }
        else
        {
            pcDest[uiOut++] = *pc;
        }
    }

    pcDest[uiOut] = 0;
    return uiCount;
}
