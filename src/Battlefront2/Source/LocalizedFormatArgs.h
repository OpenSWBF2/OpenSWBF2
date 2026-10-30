#pragma once

inline bool LocalizedFormatArgsFit(const unsigned short* pusFormat, int iArgs)
{
    int iUsed = 0;
    for (const unsigned short* p = pusFormat; *p; ++p)
    {
        if (*p != '%')
        {
            continue;
        }
        ++p;
        if (*p == 'c' || *p == 'd' || *p == 'f' || *p == 's')
        {
            if (++iUsed > iArgs)
            {
                return false;
            }
        }
        else if (*p != '%')
        {
            return true;
        }
    }
    return true;
}
