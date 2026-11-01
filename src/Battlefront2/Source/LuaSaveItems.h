#pragma once

#include "LuaFL/include/lua.h"

namespace LuaSaveItems
{
    unsigned int Write(lua_State* L, unsigned char* pOut, unsigned int uCapacity);
    int Read(lua_State* L, const unsigned char* pData, unsigned int uSize);
}
