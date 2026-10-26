#pragma once

namespace PoolMath
{

inline bool PoolAlloc(unsigned int uiPoolSize, unsigned int& uiOffset, unsigned int uiCount, unsigned int& uiStart)
{
    if (uiCount >= uiPoolSize)
    {
        return false;
    }
    if (uiOffset + uiCount >= uiPoolSize)
    {
        uiOffset = 0;
    }
    uiStart = uiOffset;
    uiOffset += uiCount;
    return true;
}

inline bool RenderItemHasRoom(unsigned int uiUsed, unsigned int uiPoolSize)
{
    return uiUsed < uiPoolSize;
}

}
