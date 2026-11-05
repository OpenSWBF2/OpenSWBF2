#pragma once

inline int ClampLegPairCount(int iCount)
{
    if (iCount < 0)
    {
        iCount = 0;
    }
    else if (iCount > 3)
    {
        iCount = 3;
    }
    return iCount;
}

template <class INFO>
inline INFO* LegPairAt(INFO* pInfo, int iPair, int iCount)
{
    if (pInfo && iPair >= 0 && iPair < iCount)
    {
        return &pInfo[iPair];
    }
    return 0;
}

template <class PAIR>
inline float LegPairTurnState(PAIR* const* ppPairs, int iCount, int iPair)
{
    return iCount > iPair ? ppPairs[iPair]->GetTurnState() : 0.0f;
}
