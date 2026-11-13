#include <stdio.h>

#include "EventManager_LuaUser.h"
#include "EventManager_Character.h"
#include "NetGame.h"

#include "EventManager_Object.h"

#include "EventManager_LuaCallback.h"

namespace EventManager
{

// FUNCTION: SWBF2 0x005139f0
template <>
bool LuaPushItem<LuaObject>(LuaObject& item)
{
    item.Push();
    return true;
}

// FUNCTION: SWBF2 0x00513a10
template <>
LuaObjectMatches LuaGetItem<LuaObjectMatches>(int iIndex)
{
    return LuaObjectMatches(LuaHelper::mState, iIndex);
}



namespace LuaUserEvent
{

// FUNCTION: SWBF2 0x00513a70
static int Trigger(lua_State*)
{
    Event<LuaObject, LuaObject>* pEvent =
        (Event<LuaObject, LuaObject>*)lua_topointer(LuaHelper::mState, lua_upvalueindex(1));
    LuaObject item(LuaHelper::mState, 1);
    LuaObject param(LuaHelper::mState, 2);
    pEvent->Trigger(item, param);
    return 0;
}
}

namespace LuaUserEvent
{

// FUNCTION: SWBF2 0x00513b70
int Create(lua_State*)
{
    const char* pcName = luaL_checklstring(LuaHelper::mState, 1, 0);
    Event<LuaObject, LuaObject>* pEvent = new Event<LuaObject, LuaObject>((char*)pcName);
    RegisterUnregisterLuaCallback(*pEvent, DummyLuaCallback);
    RegisterRegisterLuaCallback(*pEvent, DummyLuaCallback);
    RegisterRegisterLuaCallback<LuaObject, LuaObject, LuaObjectMatches>(*pEvent, DummyLuaCallback);

    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "Trigger%s", pcName);
    acName[sizeof(acName) - 1] = 0;
    lua_pushlightuserdata(LuaHelper::mState, pEvent);
    LuaHelper::Register(acName, EventManager::LuaUserEvent::Trigger, 1);
    return 0;
}
}


// TEMPLATE: SWBF2 0x00513ca0
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Callback::operator()

// SYNTHETIC: SWBF2 0x00513cb0
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Callback::`scalar deleting destructor'

// FUNCTION: SWBF2 0x00513d20
template <class ITEM, class PARAM>
void Event<ITEM, PARAM>::Callback::Trigger(ITEM& item, PARAM& param)
{
    AddRef();
    if ((*this)(item, param))
    {
        Callback* pChild = (Callback*)mChild;
        if (pChild)
        {
            pChild->AddRef();
        }
        while (pChild)
        {
            if (!pChild->IsAlive())
            {
                pChild->Release();
                break;
            }
            Callback* pNext = (Callback*)pChild->mSibling;
            if (pNext)
            {
                pNext->AddRef();
            }
            pChild->Trigger(item, param);
            pChild->Release();
            pChild = pNext;
        }
    }
    Release();
}




// TEMPLATE: SWBF2 0x005145d0
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::GetFilter<EventManager::LuaObjectMatches>

// SYNTHETIC: SWBF2 0x005144a0
// EventManager::LuaCallback<EventManager::LuaObject,EventManager::LuaObject>::`scalar deleting destructor'


// TEMPLATE: SWBF2 0x00514490
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Filter<EventManager::LuaObjectMatches>::GetName

// TEMPLATE: SWBF2 0x00514430
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Filter<EventManager::LuaObjectMatches>::operator()

// SYNTHETIC: SWBF2 0x00514540
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Filter<EventManager::LuaObjectMatches>::`scalar deleting destructor'

// TEMPLATE: SWBF2 0x00513e40
// EventManager::UnregisterLuaCallback<EventManager::LuaObject,EventManager::LuaObject>

// FUNCTION: SWBF2 0x00514160
template <class ITEM, class PARAM, class PREDICATE>
int RegisterLuaCallback(lua_State*)
{
    Event<ITEM, PARAM>* pEvent =
        (Event<ITEM, PARAM>*)lua_topointer(LuaHelper::mState, lua_upvalueindex(1));
    if (lua_isfunction(LuaHelper::mState, 1))
    {
        lua_pushvalue(LuaHelper::mState, 1);
        int iRef = luaL_ref(LuaHelper::mState, LUA_REGISTRYINDEX);
        PREDICATE predicate = LuaGetItem<PREDICATE>(2);
        typename Event<ITEM, PARAM>::template Filter<PREDICATE>* pFilter =
            pEvent->GetFilter(predicate);
        LuaCallback<ITEM, PARAM>* pCallback = new LuaCallback<ITEM, PARAM>(iRef);
        pEvent->Register(*pCallback, pFilter);
        luaL_unref(LuaHelper::mState, LUA_REGISTRYINDEX, iRef);
        pFilter->Release();
        lua_pushlightuserdata(LuaHelper::mState, pCallback);
        pCallback->Release();
        return 1;
    }
    lua_pushnil(LuaHelper::mState);
    return 1;
}

// TEMPLATE: SWBF2 0x00513e90
// EventManager::RegisterUnregisterLuaCallback<EventManager::LuaObject,EventManager::LuaObject>

// FUNCTION: SWBF2 0x00513f20
template <class ITEM, class PARAM>
void RegisterRegisterLuaCallback(Event<ITEM, PARAM>& event, lua_CFunction pfnClient)
{
    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "On%s", event.GetName());
    acName[sizeof(acName) - 1] = 0;
    lua_pushlightuserdata(LuaHelper::mState, &event);
    if (NetGame::IsEnabled() && NetGame::IsClient())
    {
        LuaHelper::Register(acName, pfnClient, 0);
    }
    else
    {
        LuaHelper::Register(acName, RegisterLuaCallback<ITEM, PARAM>, 1);
    }
}

// FUNCTION: SWBF2 0x005140d0
template <class ITEM, class PARAM, class PREDICATE>
void RegisterRegisterLuaCallback(Event<ITEM, PARAM>& event, lua_CFunction pfnClient)
{
    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "On%s%s", event.GetName(), PREDICATE::GetName());
    acName[sizeof(acName) - 1] = 0;
    lua_pushlightuserdata(LuaHelper::mState, &event);
    if (NetGame::IsEnabled() && NetGame::IsClient())
    {
        LuaHelper::Register(acName, pfnClient, 0);
    }
    else
    {
        LuaHelper::Register(acName, RegisterLuaCallback<ITEM, PARAM, PREDICATE>, 1);
    }
}

template class Event<LuaObject, LuaObject>::Callback;
template class LuaCallback<LuaObject, LuaObject>;
template class Event<LuaObject, LuaObject>::Filter<LuaObjectMatches>;
template void Event<LuaObject, LuaObject>::Unregister(Callback&);
template Event<LuaObject, LuaObject>::Filter<LuaObjectMatches>*
Event<LuaObject, LuaObject>::GetFilter<LuaObjectMatches>(LuaObjectMatches&, Callback*);
template void RegisterUnregisterLuaCallback<LuaObject, LuaObject>(Event<LuaObject, LuaObject>&, lua_CFunction);
template void RegisterRegisterLuaCallback<LuaObject, LuaObject>(Event<LuaObject, LuaObject>&, lua_CFunction);
template void RegisterRegisterLuaCallback<LuaObject, LuaObject, LuaObjectMatches>(Event<LuaObject, LuaObject>&, lua_CFunction);

}

namespace EventManager
{

// TEMPLATE: SWBF2 0x00513c40
// EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Callback::~Callback
template Event<LuaObject, LuaObject>::Callback::~Callback();
}

namespace EventManager
{

int DummyLuaCallback(lua_State* L);
namespace LuaUserEvent
{
int Create(lua_State* L); void Init();
}

// FUNCTION: SWBF2 0x00513c20
void LuaUserEvent::Init()
{
    LuaHelper::Register("CreateUserEvent", Create, 0);
}

}

namespace EventManager
{
// TEMPLATE: SWBF2 0x00515080
// EventManager::Event<GameObject,int>::Callback::operator()
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x00517e60
// EventManager::Event<GameObject,DamageOwner>::Unregister
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x005160b0
// EventManager::RegisterLuaCallback<GameObject,DamageOwner,EventManager::ObjectMatches<DamageOwner> >
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x005162c0
// EventManager::RegisterLuaCallback<GameObject,DamageOwner,EventManager::ObjectMatchesName<DamageOwner> >
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x005164d0
// EventManager::RegisterLuaCallback<GameObject,DamageOwner,EventManager::ObjectMatchesTeam<DamageOwner> >
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x005166e0
// EventManager::RegisterLuaCallback<GameObject,DamageOwner,EventManager::ObjectMatchesClass<DamageOwner> >
}
