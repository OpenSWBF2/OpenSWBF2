#pragma once

inline bool SplitDataFileName(const char* pName, char* pcFileName, char* pcLvlName, unsigned int uiSize)
{
    unsigned int i = 0;
    for (; pName[i] != '\0' && pName[i] != ';'; ++i)
    {
        if (i + 1 >= uiSize)
        {
            return false;
        }
        pcFileName[i] = pName[i];
    }
    pcFileName[i] = '\0';
    pcLvlName[0] = '\0';
    if (pName[i] == ';')
    {
        unsigned int j = 0;
        for (++i; pName[i] != '\0'; ++i)
        {
            if (j + 1 >= uiSize)
            {
                return false;
            }
            pcLvlName[j++] = pName[i];
        }
        pcLvlName[j] = '\0';
    }
    return true;
}
