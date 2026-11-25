#pragma once

#include "LuaFL/include/lua.h"

namespace LuaSaveItems
{
    unsigned int Write(lua_State* L, unsigned char* pOut, unsigned int uCapacity);
    int Read(lua_State* L, const unsigned char* pData, unsigned int uSize);
    int Load(lua_State* L, const unsigned char* pData, unsigned int uSize, const char* pcType,
             const unsigned short* pusName);
}

#ifdef OPENSWBF2_TEST
namespace SaveRejects
{
    void Note();
    unsigned int Count();
}
#endif
