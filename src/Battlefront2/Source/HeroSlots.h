#pragma once

#include "PebbleFL/Common/PblIndex.h"

template <class T, unsigned int N>
inline bool HeroSlotSet(const T (&aSlots)[N], int iTeam)
{
    return PblInBounds(aSlots, iTeam) && aSlots[iTeam];
}

template <class T, unsigned int N>
inline bool HeroSlotIs(const T (&aSlots)[N], int iTeam, T value)
{
    return PblInBounds(aSlots, iTeam) && aSlots[iTeam] == value;
}

template <class T, unsigned int N>
inline T HeroSlotOr(const T (&aSlots)[N], int iTeam, T fallback)
{
    return PblInBounds(aSlots, iTeam) ? aSlots[iTeam] : fallback;
}

template <class T, unsigned int N>
inline void SetHeroSlot(T (&aSlots)[N], int iTeam, T value)
{
    if (PblInBounds(aSlots, iTeam))
    {
        aSlots[iTeam] = value;
    }
}

template <class T, unsigned int N>
inline bool HasRulesSlot(const T (&aSlots)[N], int iRules)
{
    return PblInBounds(aSlots, iRules);
}
