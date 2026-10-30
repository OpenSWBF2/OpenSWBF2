#pragma once

#include "PebbleFL/Common/PblUnicode.h"

inline int FormatProfileName(unsigned short* pusDest, unsigned int uiCount, const unsigned short* pusTemplate,
                             int iNumber)
{
    if (!uiCount)
    {
        return 0;
    }
    if (!pusTemplate)
    {
        pusDest[0] = 0;
        return 0;
    }

    bool bAccept = true;
    bool bNumber = false;
    for (const unsigned short* p = pusTemplate; *p; ++p)
    {
        if (*p != '%')
        {
            continue;
        }
        ++p;
        if (*p == '%')
        {
            continue;
        }
        if (*p == 'd' && !bNumber)
        {
            bNumber = true;
            continue;
        }
        bAccept = false;
        break;
    }

    if (bAccept)
    {
        return PblUnicode::usnprintf(pusDest, uiCount, pusTemplate, iNumber);
    }
    PblUnicode::ustrlcpy(pusDest, pusTemplate, uiCount);
    return (int)PblUnicode::ustrlen(pusDest);
}
