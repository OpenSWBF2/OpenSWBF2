#pragma once

#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Util/RedStack.h"

template <class TYPE>
inline TYPE** AllocFormationList(RedStack& stack, unsigned int uNumFormations)
{
    return (TYPE**)stack.Alloc((int)(uNumFormations * sizeof(TYPE*)));
}

template <unsigned int N>
inline void AppendFormation(unsigned int (&auFormations)[N], unsigned int& uNumFormations, const char* pszName)
{
    if (uNumFormations < N)
    {
        auFormations[uNumFormations] = PblHash(pszName);
        uNumFormations++;
    }
}
