#pragma once

#include "LuaFL/include/lua.h"

namespace LuaVideoArgs
{
    const int kLegacyVideoSetArgs = 21;
    const int kLegacyVideoGetValues = 19;

    bool ReadLegacyVideoSet(lua_State* L, float afOut[21]);
    int PushLegacyVideoGet(lua_State* L, const float afIn[19]);
}
