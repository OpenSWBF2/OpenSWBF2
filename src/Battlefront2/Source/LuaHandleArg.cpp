#include "LuaHandleArg.h"

namespace LuaHandleArg
{

const void* Get(lua_State* L, int iIndex)
{
    if (lua_type(L, iIndex) != LUA_TLIGHTUSERDATA)
    {
        return 0;
    }
    return lua_topointer(L, iIndex);
}

}
