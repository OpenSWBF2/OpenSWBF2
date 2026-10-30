#pragma once

#include "LuaFL/include/lua.h"

namespace LuaWideString
{
    bool HasWideTerminator(const char* pcData, unsigned int uiLen);
    const unsigned short* Check(lua_State* L, int iIndex);
    const unsigned short* Opt(lua_State* L, int iIndex);
}
