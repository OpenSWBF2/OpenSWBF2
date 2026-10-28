#include "LuaVideoArgs.h"

#include "LuaFL/include/lauxlib.h"

namespace LuaVideoArgs
{

bool ReadLegacyVideoSet(lua_State* L, float afOut[21])
{
    if (lua_gettop(L) != kLegacyVideoSetArgs)
    {
        return false;
    }
    float afRead[kLegacyVideoSetArgs];
    for (int i = 0; i < kLegacyVideoSetArgs; ++i)
    {
        afRead[i] = (float)luaL_checknumber(L, i + 1);
    }
    for (int i = 0; i < kLegacyVideoSetArgs; ++i)
    {
        afOut[i] = afRead[i];
    }
    return true;
}

int PushLegacyVideoGet(lua_State* L, const float afIn[19])
{
    for (int i = 0; i < kLegacyVideoGetValues; ++i)
    {
        lua_pushnumber(L, afIn[i]);
    }
    return kLegacyVideoGetValues;
}

}
