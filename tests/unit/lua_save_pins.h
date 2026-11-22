#pragma once

struct lua_State;

extern const unsigned char kPinnedItems[89];

void PushPinnedItems(lua_State* L);
