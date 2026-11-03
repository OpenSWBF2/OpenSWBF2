#include <stdlib.h>

#include "RedEngineFL/Memory/RedMemory.h"

namespace GameMemory { int LuaHeap = -1; }

static char* sT1Arena;

extern "C" void Startup_RedInitHeap()
{
    const unsigned int uiArenaSize = 0x4000000;
    const unsigned int uiLuaSize = 0x1000000;
    if (!sT1Arena)
    {
        sT1Arena = (char*)malloc(uiArenaSize);
        if (!sT1Arena)
        {
            return;
        }
    }
    char* pEnd = sT1Arena + uiArenaSize;
    char* pLuaStart = pEnd - uiLuaSize;
    char* pStart = RedInitHeap(sT1Arena, pEnd, 9);
    RedCreateHeap(pStart, pLuaStart, 0x10, "T1");
    GameMemory::LuaHeap = RedCreateHeap(pLuaStart, pEnd, 4, "Lua");
    RedSetCurrentHeap(0);
}
