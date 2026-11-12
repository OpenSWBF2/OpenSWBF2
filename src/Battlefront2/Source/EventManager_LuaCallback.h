#pragma once

#include <stdio.h>

#include "EventManager.h"
#include "LuaHelper.h"

namespace EventManager
{

template <class ITEM>
bool LuaPushItem(ITEM& item);

// VTABLE: SWBF2 0x0079e250
// class LuaCallback<CommandPost, struct DamageOwner>
template <class ITEM, class PARAM>
class LuaCallback : public Event<ITEM, PARAM>::Callback
{
public:

    LuaCallback(int iRef);
    virtual ~LuaCallback();
    virtual bool operator()(ITEM& item, PARAM& param);

private:
    int mFunction;
};

// TEMPLATE: SWBF2 0x0050e3b0
// EventManager::LuaCallback<Character,char>::LuaCallback<Character,char>

// TEMPLATE: SWBF2 0x005115c0
// EventManager::LuaCallback<CommandPost,DamageOwner>::LuaCallback<CommandPost,DamageOwner>

// TEMPLATE: SWBF2 0x005133b0
// EventManager::LuaCallback<FlagItem,char>::LuaCallback<FlagItem,char>

// TEMPLATE: SWBF2 0x00514330
// EventManager::LuaCallback<EventManager::LuaObject,EventManager::LuaObject>::LuaCallback<EventManager::LuaObject,EventManager::LuaObject>

// TEMPLATE: SWBF2 0x0051c650
// EventManager::LuaCallback<Timer,char>::LuaCallback<Timer,char>

template <class ITEM, class PARAM>
inline LuaCallback<ITEM, PARAM>::LuaCallback(int iRef)
{
    mFunction = -1;
    if (LuaHelper::PushRefProc(iRef))
    {
        mFunction = luaL_ref(LuaHelper::mState, LUA_REGISTRYINDEX);
    }
}

template <class ITEM, class PARAM>
inline LuaCallback<ITEM, PARAM>::~LuaCallback()
{
    if (mFunction != -1)
    {
        lua_unref(LuaHelper::mState, mFunction);
    }
}

// TEMPLATE: SWBF2 0x0050e250
// EventManager::LuaCallback<Character,DamageOwner>::operator()

// TEMPLATE: SWBF2 0x005143c0
// EventManager::LuaCallback<EventManager::LuaObject,EventManager::LuaObject>::operator()

// TEMPLATE: SWBF2 0x0051c6e0
// EventManager::LuaCallback<Timer,char>::operator()

template <class ITEM, class PARAM>
inline bool LuaCallback<ITEM, PARAM>::operator()(ITEM& item, PARAM& param)
{
    if (mFunction == -1 || !LuaHelper::PushRefProc(mFunction))
    {
        return false;
    }

    if (!LuaPushItem(item))
    {
        lua_pop(LuaHelper::mState, 1);
        return false;
    }

    if (!LuaPushItem(param))
    {
        lua_pop(LuaHelper::mState, 2);
        return false;
    }

    LuaHelper::CallProc(0);
    return true;
}

// TEMPLATE: SWBF2 0x0050d0b0
// EventManager::RegisterLuaCallback<Character,GameObject>

// TEMPLATE: SWBF2 0x005100a0
// EventManager::RegisterLuaCallback<CommandPost,unsigned __int64>

// TEMPLATE: SWBF2 0x00513020
// EventManager::RegisterLuaCallback<FlagItem,char>

// TEMPLATE: SWBF2 0x00513fb0
// EventManager::RegisterLuaCallback<EventManager::LuaObject,EventManager::LuaObject>

// TEMPLATE: SWBF2 0x0051b340
// EventManager::RegisterLuaCallback<Team,int>

template <class ITEM, class PARAM>
int RegisterLuaCallback(lua_State*)
{
    Event<ITEM, PARAM>* pEvent =
        (Event<ITEM, PARAM>*)lua_topointer(LuaHelper::mState, lua_upvalueindex(1));
    if (lua_isfunction(LuaHelper::mState, 1))
    {
        lua_pushvalue(LuaHelper::mState, 1);
        int iRef = luaL_ref(LuaHelper::mState, LUA_REGISTRYINDEX);
        LuaCallback<ITEM, PARAM>* pCallback = new LuaCallback<ITEM, PARAM>(iRef);
        pEvent->Register(*pCallback, 0);
        luaL_unref(LuaHelper::mState, LUA_REGISTRYINDEX, iRef);
        lua_pushlightuserdata(LuaHelper::mState, pCallback);
        pCallback->Release();
        return 1;
    }
    lua_pushnil(LuaHelper::mState);
    return 1;
}

template <class ITEM, class PARAM>
int UnregisterLuaCallback(lua_State*)
{
    Event<ITEM, PARAM>* pEvent =
        (Event<ITEM, PARAM>*)lua_topointer(LuaHelper::mState, lua_upvalueindex(1));
    if (lua_islightuserdata(LuaHelper::mState, 1))
    {
        typename Event<ITEM, PARAM>::Callback* pCallback =
            (typename Event<ITEM, PARAM>::Callback*)lua_topointer(LuaHelper::mState, 1);
        if (pCallback)
        {
            pEvent->Unregister(*pCallback);
        }
    }
    return 0;
}

template <class ITEM, class PARAM>
void RegisterUnregisterLuaCallback(Event<ITEM, PARAM>& event, lua_CFunction pfnClient)
{
    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "Release%s", event.GetName());
    acName[sizeof(acName) - 1] = 0;
    lua_pushlightuserdata(LuaHelper::mState, &event);
    if (NetGame::IsEnabled() && NetGame::IsClient())
    {
        LuaHelper::Register(acName, pfnClient, 0);
    }
    else
    {
        LuaHelper::Register(acName, UnregisterLuaCallback<ITEM, PARAM>, 1);
    }
}

}
