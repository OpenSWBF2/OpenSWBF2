#pragma once

#include "LuaFL/include/lua.h"

namespace LuaHandleArg
{
    const void* Get(lua_State* L, int iIndex);
}
