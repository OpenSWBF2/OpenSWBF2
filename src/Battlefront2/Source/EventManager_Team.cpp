#include "EventManager_Team.h"
#include "LuaHelper.h"
#include "LuaFL/include/lauxlib.h"
#include "EventManager_LuaUser.h"
#include "NetGame.h"

#include "SkyDome.h"
#include "SkyStars.h"
#include "WeatherEffect.h"
#include "PebbleFL/Common/PblRef.h"
#include "RedEngineFL/Graphics/PC/pcRedIndexBuffer.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcInterfaceShader.h"
#include "RedEngineFL/Interface/RedBorderElement.h"
#include "RedEngineFL/Interface/RedHotSpot.h"
#include "RedEngineFL/Interface/RedTextElement.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"
#include "LuaCallbacks_PC.h"
#include "LuaCallbacks_Net.h"
#include "FLInputManager.h"
#include "PlayerProfile.h"
#include "RedEngineFL/input/RedKeyboard.h"
#include "RedEngineFL/Application/pc/pcFLRender.h"
#include "RedEngineFL/Interface/Red3DModelAnimator.h"
#include "RedEngineFL/Interface/Red3DModelElement.h"
#include "RedEngineFL/movie/RedFileAsync.h"
#include "SoundFL/source/sndstreamimaadpcmdecoder.h"
#include "BoundaryManager.h"
#include "CommandPost.h"
#include "EndDisplay.h"
#include "EventManager_CommandPost.h"
#include "EventManager_Object.h"
#include "FLEffect.h"
#include "GameObject.h"
#include "WeaponTowCable.h"
#include "HUDElementBarBitmap.h"
#include "HUDElementMap.h"
#include "HUDElementText.h"
#include "HUDEvent.h"
#include "HUDProceduralBarBitmap.h"
#include "LeafEffect.h"
#include "LoadUtil.h"
#include "LuaCallbacks_Sound.h"
#include "OrdnanceBeam.h"
#include "StatusDisplay.h"
#include "TargetManager.h"
#include "Team.h"
#include "VOHelper.h"
#include "VoiceOverManager.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Interface/RedBitmapElement.h"
#include "SoundFL/source/sndenginebase.h"
#include "SoundFL/source/sndstreambase.h"
#include "SoundFL/source/sndthreadupdate.h"
#include "SoundFL/source/win/sndengine.h"
#include "SoundFL/source/win/sndstream.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "FLJoystick.h"

#include "EventManager_LuaCallback.h"

namespace EventManager
{
namespace TeamEvent
{
extern Event<Team, int> mTicketCountChange;
extern Event<Team, float> mTicketBleedChange;
extern Event<Team, int> mPointsChange;
void Cleanup();
}
}

namespace EventManager
{
namespace TeamEvent
{
extern Event<Team, int> mTicketCountChange;
}
}
namespace EventManager
{
namespace TeamEvent
{
extern Event<Team, float> mTicketBleedChange;
}
}
namespace EventManager
{
namespace TeamEvent
{
extern Event<Team, int> mPointsChange;
}
}

namespace EventManager
{

// FUNCTION: SWBF2 0x0051af40
template <>
bool LuaPushItem<Team>(Team& team)
{
    lua_pushnumber(LuaHelper::mState, (lua_Number)team.GetNumber());
    return true;
}




// TEMPLATE: SWBF2 0x0051b0e0
// EventManager::Event<Team,float>::Callback::operator()

// TEMPLATE: SWBF2 0x0051bb90
// EventManager::LuaCallback<Team,int>::LuaCallback<Team,int>

// TEMPLATE: SWBF2 0x0051bcc0
// EventManager::LuaCallback<Team,float>::LuaCallback<Team,float>

// TEMPLATE: SWBF2 0x0051bc20
// EventManager::LuaCallback<Team,int>::operator()

// TEMPLATE: SWBF2 0x0051bd50
// EventManager::LuaCallback<Team,float>::operator()

// SYNTHETIC: SWBF2 0x0051bdf0
// EventManager::LuaCallback<Team,int>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x0051bf00
// EventManager::LuaCallback<Team,float>::`scalar deleting destructor'

// TEMPLATE: SWBF2 0x0051bc90
// EventManager::Event<Team,int>::Filter<EventManager::TeamMatches<int> >::operator()

// TEMPLATE: SWBF2 0x0051bdc0
// EventManager::Event<Team,float>::Filter<EventManager::TeamMatches<float> >::operator()

// TEMPLATE: SWBF2 0x0051bcb0
// EventManager::Event<Team,int>::Filter<EventManager::TeamMatches<int> >::GetName

// TEMPLATE: SWBF2 0x0051bde0
// EventManager::Event<Team,float>::Filter<EventManager::TeamMatches<float> >::GetName

// SYNTHETIC: SWBF2 0x0051be90
// EventManager::Event<Team,int>::Filter<EventManager::TeamMatches<int> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x0051bfa0
// EventManager::Event<Team,float>::Filter<EventManager::TeamMatches<float> >::`scalar deleting destructor'

template class LuaCallback<Team, int>;
template class LuaCallback<Team, float>;
template class Event<Team, int>::Filter<TeamMatches<int> >;
template class Event<Team, float>::Filter<TeamMatches<float> >;
template bool Event<Team, float>::Callback::operator()(Team&, float&);
template int RegisterLuaCallback<Team, int>(lua_State*);

}

namespace EventManager
{

// FUNCTION: SWBF2 0x0051aff0
void TeamEvent::Cleanup()
{
    mTicketCountChange.Cleanup();
    mTicketBleedChange.Cleanup();
    mPointsChange.Cleanup();
}
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0051b010
// EventManager::Event<Team,int>::Callback::~Callback
template Event<Team, int>::Callback::~Callback();
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0051b080
// EventManager::Event<Team,float>::Callback::~Callback
template Event<Team, float>::Callback::~Callback();
}

namespace EventManager
{

int DummyLuaCallback(lua_State* L);
namespace TeamEvent
{
void Init();
}



// FUNCTION: SWBF2 0x0051af60
template <>
TeamMatches<int> LuaGetItem<TeamMatches<int> >(int)
{
    return TeamMatches<int>((int)luaL_checknumber(LuaHelper::mState, 2));
}

// FUNCTION: SWBF2 0x0051af80
template <>
TeamMatches<float> LuaGetItem<TeamMatches<float> >(int)
{
    return TeamMatches<float>((int)luaL_checknumber(LuaHelper::mState, 2));
}

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

// FUNCTION: SWBF2 0x0051afa0
void TeamEvent::Init()
{
    mTicketCountChange.Init();
    RegisterUnregisterLuaCallback(mTicketCountChange, DummyLuaCallback);
    RegisterRegisterLuaCallback(mTicketCountChange, DummyLuaCallback);
    RegisterRegisterLuaCallback<Team, int, TeamMatches<int> >(mTicketCountChange, DummyLuaCallback);

    mTicketBleedChange.Init();
    RegisterUnregisterLuaCallback(mTicketBleedChange, DummyLuaCallback);
    RegisterRegisterLuaCallback(mTicketBleedChange, DummyLuaCallback);
    RegisterRegisterLuaCallback<Team, float, TeamMatches<float> >(mTicketBleedChange, DummyLuaCallback);

    mPointsChange.Init();
    RegisterUnregisterLuaCallback(mPointsChange, DummyLuaCallback);
    RegisterRegisterLuaCallback(mPointsChange, DummyLuaCallback);
    RegisterRegisterLuaCallback<Team, int, TeamMatches<int> >(mPointsChange, DummyLuaCallback);
}

template int UnregisterLuaCallback<Team, int>(lua_State*);
template int RegisterLuaCallback<Team, float>(lua_State*);
template int RegisterLuaCallback<Team, int, TeamMatches<int> >(lua_State*);
template int UnregisterLuaCallback<Team, float>(lua_State*);
template int RegisterLuaCallback<Team, float, TeamMatches<float> >(lua_State*);
template void Event<Team, int>::Unregister(Callback&);
template void Event<Team, float>::Unregister(Callback&);
template Event<Team, int>::Filter<TeamMatches<int> >*
Event<Team, int>::GetFilter(TeamMatches<int>&, Callback*);
template Event<Team, float>::Filter<TeamMatches<float> >*
Event<Team, float>::GetFilter(TeamMatches<float>&, Callback*);

// TEMPLATE: SWBF2 0x0051b220
// EventManager::RegisterUnregisterLuaCallback<Team,int>

// TEMPLATE: SWBF2 0x0051b2b0
// EventManager::RegisterRegisterLuaCallback<Team,int>

// TEMPLATE: SWBF2 0x0051b460
// EventManager::RegisterRegisterLuaCallback<Team,int,EventManager::TeamMatches<int> >

// TEMPLATE: SWBF2 0x0051b6c0
// EventManager::RegisterUnregisterLuaCallback<Team,float>

// TEMPLATE: SWBF2 0x0051b750
// EventManager::RegisterRegisterLuaCallback<Team,float>

// TEMPLATE: SWBF2 0x0051b900
// EventManager::RegisterRegisterLuaCallback<Team,float,EventManager::TeamMatches<float> >

// TEMPLATE: SWBF2 0x0051b1d0
// EventManager::UnregisterLuaCallback<Team,int>

// TEMPLATE: SWBF2 0x0051b4f0
// EventManager::RegisterLuaCallback<Team,int,EventManager::TeamMatches<int> >

// TEMPLATE: SWBF2 0x0051b7e0
// EventManager::RegisterLuaCallback<Team,float>

// TEMPLATE: SWBF2 0x0051bb10
// EventManager::Event<Team,int>::Unregister

// TEMPLATE: SWBF2 0x0051bb50
// EventManager::Event<Team,float>::Unregister

// TEMPLATE: SWBF2 0x0051c010
// EventManager::Event<Team,int>::GetFilter<EventManager::TeamMatches<int> >

// TEMPLATE: SWBF2 0x0051c0c0
// EventManager::Event<Team,float>::GetFilter<EventManager::TeamMatches<float> >

// TEMPLATE: SWBF2 0x0051b670
// EventManager::UnregisterLuaCallback<Team,float>

// TEMPLATE: SWBF2 0x0051b990
// EventManager::RegisterLuaCallback<Team,float,EventManager::TeamMatches<float> >

}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0051b070
// EventManager::Event<Team,int>::Callback::operator()
template bool Event<Team, int>::Callback::operator()(Team&, int&);
}

namespace EventManager
{
// SYNTHETIC: SWBF2 0x0051b0f0
// EventManager::Event<Team,int>::Callback::`scalar deleting destructor'
}

namespace EventManager
{
// SYNTHETIC: SWBF2 0x0051b160
// EventManager::Event<Team,float>::Callback::`scalar deleting destructor'
}

namespace EventManager
{
namespace TeamEvent
{

// SYNTHETIC: SWBF2 0x00403710
// EventManager::TeamEvent::`dynamic initializer for 'mTicketCountChange''
// SYNTHETIC: SWBF2 0x00768520
// EventManager::TeamEvent::`dynamic atexit destructor for 'mTicketCountChange''

// GLOBAL: SWBF2 0x007ec0c0
// mTicketCountChange
Event<Team, int> mTicketCountChange("TicketCountChange");

// SYNTHETIC: SWBF2 0x00403740
// EventManager::TeamEvent::`dynamic initializer for 'mTicketBleedChange''
// SYNTHETIC: SWBF2 0x007685a0
// EventManager::TeamEvent::`dynamic atexit destructor for 'mTicketBleedChange''

// GLOBAL: SWBF2 0x007ec0e0
// mTicketBleedChange
Event<Team, float> mTicketBleedChange("TicketBleedChange");

// SYNTHETIC: SWBF2 0x00403770
// EventManager::TeamEvent::`dynamic initializer for 'mPointsChange''
// SYNTHETIC: SWBF2 0x00768620
// EventManager::TeamEvent::`dynamic atexit destructor for 'mPointsChange''

// GLOBAL: SWBF2 0x007ec100
// mPointsChange
Event<Team, int> mPointsChange("TeamPointsChange");

}
}
