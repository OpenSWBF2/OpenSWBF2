#pragma once

#include "LuaFL/include/lua.h"

class PblMatrix;

namespace LuaMatrixArg
{
    const PblMatrix* Get(lua_State* L, int iIndex);
}
