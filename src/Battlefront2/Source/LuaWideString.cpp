#include "LuaWideString.h"

#include "LuaFL/include/lauxlib.h"

namespace LuaWideString
{

bool HasWideTerminator(const char* pcData, unsigned int uiLen)
{
    for (unsigned int i = 0; i + 1 < uiLen; i += 2)
    {
        if (pcData[i] == 0 && pcData[i + 1] == 0)
        {
            return true;
        }
    }
    return false;
}

const unsigned short* Check(lua_State* L, int iIndex)
{
    if (iIndex < 0 && iIndex > LUA_REGISTRYINDEX)
    {
        iIndex = lua_gettop(L) + iIndex + 1;
    }
    size_t uLen = 0;
    const char* pcData = luaL_checklstring(L, iIndex, &uLen);
    if (HasWideTerminator(pcData, (unsigned int)uLen))
    {
        return (const unsigned short*)pcData;
    }
    lua_pushlstring(L, pcData, uLen);
    lua_pushlstring(L, "\0\0\0", 2 + (uLen & 1));
    lua_concat(L, 2);
    lua_replace(L, iIndex);
    return (const unsigned short*)lua_tostring(L, iIndex);
}

const unsigned short* Opt(lua_State* L, int iIndex)
{
    if (lua_isnoneornil(L, iIndex))
    {
        return 0;
    }
    return Check(L, iIndex);
}

}
