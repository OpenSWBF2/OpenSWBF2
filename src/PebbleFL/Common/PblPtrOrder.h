#pragma once

inline int PblPtrOrderKey(const void* pPtr)
{
    return (int)((unsigned int)pPtr ^ 0x80000000u);
}

inline bool PblPtrAfter(const void* pA, const void* pB)
{
    return (unsigned int)pA > (unsigned int)pB;
}
