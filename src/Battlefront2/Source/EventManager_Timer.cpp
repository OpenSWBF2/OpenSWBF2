#include "EventManager_Timer.h"
#include "EventManager_LuaUser.h"
#include "LuaHelper.h"
#include "LuaHandleArg.h"
#include "LuaFL/include/lauxlib.h"
#include "NetGame.h"

#include "EventManager_LuaCallback.h"

namespace EventManager
{

// FUNCTION: SWBF2 0x0051c170
template <>
bool LuaPushItem<Timer>(Timer& timer)
{
    lua_pushlightuserdata(LuaHelper::mState, &timer);
    return true;
}


TimerMatches::TimerMatches(const char* pcName)
    : mTimer(Timer::Find(pcName))
{
}

// FUNCTION: SWBF2 0x0051c190
template <>
TimerMatches LuaGetItem<TimerMatches>(int iIndex)
{
    if (lua_islightuserdata(LuaHelper::mState, iIndex))
    {
        const Timer* pTimer = (const Timer*)LuaHandleArg::Get(LuaHelper::mState, iIndex);
        if (!Timer::sMap.HasValue(pTimer))
        {
            pTimer = 0;
        }
        return TimerMatches(pTimer);
    }
    return TimerMatches(Timer::Find(luaL_checkstring(LuaHelper::mState, iIndex)));
}


// TEMPLATE: SWBF2 0x0051c890
// EventManager::Event<Timer,char>::GetFilter<EventManager::TimerMatches>

// FUNCTION: SWBF2 0x0051c490
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

// SYNTHETIC: SWBF2 0x0051c780
// EventManager::LuaCallback<Timer,char>::`scalar deleting destructor'


// SYNTHETIC: SWBF2 0x0051c820
// EventManager::Event<Timer,char>::Filter<EventManager::TimerMatches>::`scalar deleting destructor'

// TEMPLATE: SWBF2 0x0051c2a0
// EventManager::Event<Timer,char>::Callback::operator()

// TEMPLATE: SWBF2 0x0051c770
// EventManager::Event<Timer,char>::Filter<EventManager::TimerMatches>::GetName

// TEMPLATE: SWBF2 0x0051c740
// EventManager::Event<Timer,char>::Filter<EventManager::TimerMatches>::operator()



// TEMPLATE: SWBF2 0x0051c320
// EventManager::UnregisterLuaCallback<Timer,char>

template class LuaCallback<Timer, char>;
template void Event<Timer, char>::Unregister(Callback&);
template int UnregisterLuaCallback<Timer, char>(lua_State*);
template Event<Timer, char>::Filter<TimerMatches>*
Event<Timer, char>::GetFilter<TimerMatches>(TimerMatches&, Callback*);
template int RegisterLuaCallback<Timer, char, TimerMatches>(lua_State*);
template class Event<Timer, char>::Filter<TimerMatches>;
template bool Event<Timer, char>::Callback::operator()(Timer&, char&);

}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0051c240
// EventManager::Event<Timer,char>::Callback::~Callback
template Event<Timer, char>::Callback::~Callback();
}

namespace EventManager
{

int DummyLuaCallback(lua_State* L);
namespace TimerEvent
{
void Init();
}

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

// FUNCTION: SWBF2 0x0051c200
void TimerEvent::Init()
{
    mElapsed.Init();
    RegisterUnregisterLuaCallback(mElapsed, DummyLuaCallback);
    RegisterRegisterLuaCallback<Timer, char, TimerMatches>(mElapsed, DummyLuaCallback);
    Timer::sMemoryPool.Setup(64, Timer::sMemoryPool.GetSize(), Timer::sMemoryPool.GetGrow());
}

// TEMPLATE: SWBF2 0x0051c370
// EventManager::RegisterUnregisterLuaCallback<Timer,char>

// TEMPLATE: SWBF2 0x0051c400
// EventManager::RegisterRegisterLuaCallback<Timer,char,EventManager::TimerMatches>

}

namespace EventManager
{

// FUNCTION: SWBF2 0x0051c230
void TimerEvent::Cleanup()
{
    mElapsed.Cleanup();
    Timer::Clear();
}
}

namespace EventManager
{
namespace TimerEvent
{

// SYNTHETIC: SWBF2 0x004037a0
// EventManager::TimerEvent::`dynamic initializer for 'mElapsed''
// SYNTHETIC: SWBF2 0x007686a0
// EventManager::TimerEvent::`dynamic atexit destructor for 'mElapsed''

// GLOBAL: SWBF2 0x007ec120
// mElapsed
Event<Timer, char> mElapsed("TimerElapse");

}
}

// SYNTHETIC: SWBF2 0x0051c2b0
// EventManager::Event<Timer,char>::Callback::`scalar deleting destructor'
