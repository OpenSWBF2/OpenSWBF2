#pragma once

inline int ClampDifficultyLevel(int iLevel)
{
    if (iLevel < 1)
    {
        return 1;
    }
    if (iLevel > 3)
    {
        return 3;
    }
    return iLevel;
}
