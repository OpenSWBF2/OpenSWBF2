#include "LuaMatrixArg.h"

#include "PebbleFL/Common/PblMatrix.h"
#include "LuaFL/src/lobject.h"

namespace LuaMatrixArg
{

const PblMatrix* Get(lua_State* L, int iIndex)
{
    if (lua_type(L, iIndex) != LUA_TUSERDATA)
    {
        return 0;
    }
    if (lua_getmetatable(L, iIndex))
    {
        lua_pop(L, 1);
        return 0;
    }
    const void* pData = lua_touserdata(L, iIndex);
    const Udata* pUdata = (const Udata*)pData - 1;
    if (pUdata->uv.len != sizeof(PblMatrix))
    {
        return 0;
    }
    return (const PblMatrix*)pData;
}

}
