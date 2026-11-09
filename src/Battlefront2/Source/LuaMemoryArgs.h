#pragma once

#include "LuaFL/include/lua.h"

class MemoryPool;

namespace LuaMemoryArgs
{
    void SetMemoryPoolSize(lua_State* L, MemoryPool* pWeapon, MemoryPool* pAmmo, MemoryPool* pEnergy);
    void SetupTempHeap(lua_State* L, int iRuntimeHeap, int& riTempHeap);
    void ClearTempHeap(int iRuntimeHeap, int& riTempHeap);
    void ForgetTempHeap();
    void StealArtistHeap(lua_State* L, int iArtistHeap);
    void SetHeapCap(lua_State* L);
    void AddWalkerType(lua_State* L, MemoryPool* pWalker, MemoryPool* pLegPair, MemoryPool* pBlend,
                       MemoryPool* pBlendUnit, MemoryPool* pDroideka);
}
