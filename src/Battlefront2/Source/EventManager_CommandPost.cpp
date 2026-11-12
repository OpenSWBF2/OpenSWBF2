#include "EventManager_CommandPost.h"
#include "LuaHandleArg.h"
#include "LuaHelper.h"
#include "LuaFL/include/lauxlib.h"
#include "EventManager_LuaUser.h"
#include "NetGame.h"

#include "EventManager_Flag.h"
#include "Controllable.h"
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
#include "EventManager_Object.h"
#include "EventManager_Team.h"
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
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mBeginNeutralize;
extern Event<CommandPost, unsigned __int64> mAbortNeutralize;
extern Event<CommandPost, unsigned __int64> mFinishNeutralize;
extern Event<CommandPost, unsigned __int64> mBeginCapture;
extern Event<CommandPost, unsigned __int64> mAbortCapture;
extern Event<CommandPost, unsigned __int64> mFinishCapture;
extern Event<CommandPost, DamageOwner> mKill;
extern Event<CommandPost, char> mRespawn;
void Cleanup();
}
}

namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mBeginNeutralize;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mAbortNeutralize;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mFinishNeutralize;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mBeginCapture;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mAbortCapture;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, unsigned __int64> mFinishCapture;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, DamageOwner> mKill;
}
}
namespace EventManager
{
namespace CommandPostEvent
{
extern Event<CommandPost, char> mRespawn;
}
}

namespace EventManager
{

// FUNCTION: SWBF2 0x0050f980
template <>
bool LuaPushItem<CommandPost>(CommandPost& post)
{
    if (!post.mName)
    {
        return false;
    }
    lua_pushstring(LuaHelper::mState, post.mName);
    return true;
}

// FUNCTION: SWBF2 0x0050f9a0
template <>
bool LuaPushItem<unsigned __int64>(unsigned __int64& mask)
{
    lua_newtable(LuaHelper::mState);
    int iCount = 0;
    for (int i = 0; i < 64; ++i)
    {
        if (mask & ((unsigned __int64)1 << i))
        {
            lua_pushnumber(LuaHelper::mState, (lua_Number)++iCount);
            lua_pushnumber(LuaHelper::mState, (lua_Number)i);
            lua_rawset(LuaHelper::mState, -3);
        }
    }
    return true;
}



// TEMPLATE: SWBF2 0x00511770
// EventManager::LuaCallback<CommandPost,char>::LuaCallback<CommandPost,char>

// TEMPLATE: SWBF2 0x00511410
// EventManager::LuaCallback<CommandPost,unsigned __int64>::LuaCallback<CommandPost,unsigned __int64>

// TEMPLATE: SWBF2 0x00510750
// EventManager::RegisterLuaCallback<CommandPost,DamageOwner>

// TEMPLATE: SWBF2 0x00510e00
// EventManager::RegisterLuaCallback<CommandPost,char>



// TEMPLATE: SWBF2 0x0050fcf0
// EventManager::Event<CommandPost,unsigned __int64>::Callback::operator()

// TEMPLATE: SWBF2 0x0050fd60
// EventManager::Event<CommandPost,DamageOwner>::Callback::operator()

// TEMPLATE: SWBF2 0x005114a0
// EventManager::LuaCallback<CommandPost,unsigned __int64>::operator()

// TEMPLATE: SWBF2 0x00511650
// EventManager::LuaCallback<CommandPost,DamageOwner>::operator()

// TEMPLATE: SWBF2 0x00511800
// EventManager::LuaCallback<CommandPost,char>::operator()

// TEMPLATE: SWBF2 0x00511510
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesName<unsigned __int64> >::operator()

// TEMPLATE: SWBF2 0x00511540
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesTeam<unsigned __int64> >::operator()

// TEMPLATE: SWBF2 0x005116c0
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesName<DamageOwner> >::operator()

// TEMPLATE: SWBF2 0x005116f0
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesTeam<DamageOwner> >::operator()

// TEMPLATE: SWBF2 0x00511860
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesName<char> >::operator()

// TEMPLATE: SWBF2 0x00511890
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesTeam<char> >::operator()

// TEMPLATE: SWBF2 0x00511530
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesName<unsigned __int64> >::GetName

// TEMPLATE: SWBF2 0x005115b0
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesTeam<unsigned __int64> >::GetName

// TEMPLATE: SWBF2 0x005116e0
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesName<DamageOwner> >::GetName

// TEMPLATE: SWBF2 0x00511760
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesTeam<DamageOwner> >::GetName

// TEMPLATE: SWBF2 0x00511880
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesName<char> >::GetName

// TEMPLATE: SWBF2 0x00511900
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesTeam<char> >::GetName

// SYNTHETIC: SWBF2 0x00511910
// EventManager::LuaCallback<CommandPost,unsigned __int64>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511a90
// EventManager::LuaCallback<CommandPost,DamageOwner>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511c10
// EventManager::LuaCallback<CommandPost,char>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x005119b0
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesName<unsigned __int64> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511a20
// EventManager::Event<CommandPost,unsigned __int64>::Filter<EventManager::PostMatchesTeam<unsigned __int64> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511b30
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesName<DamageOwner> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511ba0
// EventManager::Event<CommandPost,DamageOwner>::Filter<EventManager::PostMatchesTeam<DamageOwner> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511cb0
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesName<char> >::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00511d20
// EventManager::Event<CommandPost,char>::Filter<EventManager::PostMatchesTeam<char> >::`scalar deleting destructor'

template class LuaCallback<CommandPost, unsigned __int64>;
template class LuaCallback<CommandPost, DamageOwner>;
template class LuaCallback<CommandPost, char>;
template bool Event<CommandPost, unsigned __int64>::Callback::operator()(CommandPost&, unsigned __int64&);
template bool Event<CommandPost, DamageOwner>::Callback::operator()(CommandPost&, DamageOwner&);
template class Event<CommandPost, unsigned __int64>::Filter<PostMatchesName<unsigned __int64> >;
template class Event<CommandPost, unsigned __int64>::Filter<PostMatchesTeam<unsigned __int64> >;
template class Event<CommandPost, DamageOwner>::Filter<PostMatchesName<DamageOwner> >;
template class Event<CommandPost, DamageOwner>::Filter<PostMatchesTeam<DamageOwner> >;
template class Event<CommandPost, char>::Filter<PostMatchesName<char> >;
template class Event<CommandPost, char>::Filter<PostMatchesTeam<char> >;
template int RegisterLuaCallback<CommandPost, unsigned __int64>(lua_State*);
template int RegisterLuaCallback<CommandPost, DamageOwner>(lua_State*);
template int RegisterLuaCallback<CommandPost, char>(lua_State*);

}

namespace EventManager
{

// FUNCTION: SWBF2 0x0050f970
template <>
bool LuaPushItem<char>(char&)
{
    return true;
}
}

namespace EventManager
{

// FUNCTION: SWBF2 0x0050fc40
void CommandPostEvent::Cleanup()
{
    mBeginNeutralize.Cleanup();
    mAbortNeutralize.Cleanup();
    mFinishNeutralize.Cleanup();
    mBeginCapture.Cleanup();
    mAbortCapture.Cleanup();
    mFinishCapture.Cleanup();
    mKill.Cleanup();
    mRespawn.Cleanup();
}
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0050fc90
// EventManager::Event<CommandPost,unsigned __int64>::Callback::~Callback
template Event<CommandPost, unsigned __int64>::Callback::~Callback();
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0050fd00
// EventManager::Event<CommandPost,DamageOwner>::Callback::~Callback
template Event<CommandPost, DamageOwner>::Callback::~Callback();
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0050fd70
// EventManager::Event<CommandPost,char>::Callback::~Callback
template Event<CommandPost, char>::Callback::~Callback();
}

namespace EventManager
{

int DummyLuaCallback(lua_State* L);
namespace CommandPostEvent
{
void Init();
}





// FUNCTION: SWBF2 0x0050fa30
template <>
PostMatchesName<unsigned __int64> LuaGetItem<PostMatchesName<unsigned __int64> >(int)
{
    return PostMatchesName<unsigned __int64>(luaL_checklstring(LuaHelper::mState, 2, 0));
}

// FUNCTION: SWBF2 0x0050fa60
template <>
PostMatchesName<DamageOwner> LuaGetItem<PostMatchesName<DamageOwner> >(int)
{
    return PostMatchesName<DamageOwner>(luaL_checklstring(LuaHelper::mState, 2, 0));
}

// FUNCTION: SWBF2 0x0050fa90
template <>
PostMatchesName<char> LuaGetItem<PostMatchesName<char> >(int)
{
    return PostMatchesName<char>(luaL_checklstring(LuaHelper::mState, 2, 0));
}

// FUNCTION: SWBF2 0x0050fac0
template <>
PostMatchesTeam<unsigned __int64> LuaGetItem<PostMatchesTeam<unsigned __int64> >(int)
{
    return PostMatchesTeam<unsigned __int64>((int)luaL_checknumber(LuaHelper::mState, 2));
}

// FUNCTION: SWBF2 0x0050fae0
template <>
PostMatchesTeam<DamageOwner> LuaGetItem<PostMatchesTeam<DamageOwner> >(int)
{
    return PostMatchesTeam<DamageOwner>((int)luaL_checknumber(LuaHelper::mState, 2));
}

// FUNCTION: SWBF2 0x0050fb00
template <>
PostMatchesTeam<char> LuaGetItem<PostMatchesTeam<char> >(int)
{
    return PostMatchesTeam<char>((int)luaL_checknumber(LuaHelper::mState, 2));
}

// TEMPLATE: SWBF2 0x0050ff30
// EventManager::UnregisterLuaCallback<CommandPost,unsigned __int64>
// TEMPLATE: SWBF2 0x00510250
// EventManager::RegisterLuaCallback<CommandPost,unsigned __int64,EventManager::PostMatchesName<unsigned __int64> >
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



// TEMPLATE: SWBF2 0x00511d90
// EventManager::Event<CommandPost,unsigned __int64>::GetFilter<EventManager::PostMatchesName<unsigned __int64> >

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

// FUNCTION: SWBF2 0x0050fb20
void CommandPostEvent::Init()
{
    mBeginNeutralize.Init();
    RegisterUnregisterLuaCallback(mBeginNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback(mBeginNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mBeginNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mBeginNeutralize, DummyLuaCallback);

    mAbortNeutralize.Init();
    RegisterUnregisterLuaCallback(mAbortNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback(mAbortNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mAbortNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mAbortNeutralize, DummyLuaCallback);

    mFinishNeutralize.Init();
    RegisterUnregisterLuaCallback(mFinishNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback(mFinishNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mFinishNeutralize, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mFinishNeutralize, DummyLuaCallback);

    mBeginCapture.Init();
    RegisterUnregisterLuaCallback(mBeginCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback(mBeginCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mBeginCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mBeginCapture, DummyLuaCallback);

    mAbortCapture.Init();
    RegisterUnregisterLuaCallback(mAbortCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback(mAbortCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mAbortCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mAbortCapture, DummyLuaCallback);

    mFinishCapture.Init();
    RegisterUnregisterLuaCallback(mFinishCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback(mFinishCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(mFinishCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(mFinishCapture, DummyLuaCallback);

    mKill.Init();
    RegisterUnregisterLuaCallback(mKill, DummyLuaCallback);
    RegisterRegisterLuaCallback(mKill, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, DamageOwner, PostMatchesName<DamageOwner> >(mKill, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, DamageOwner, PostMatchesTeam<DamageOwner> >(mKill, DummyLuaCallback);

    mRespawn.Init();
    RegisterUnregisterLuaCallback(mRespawn, DummyLuaCallback);
    RegisterRegisterLuaCallback(mRespawn, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, char, PostMatchesName<char> >(mRespawn, DummyLuaCallback);
    RegisterRegisterLuaCallback<CommandPost, char, PostMatchesTeam<char> >(mRespawn, DummyLuaCallback);
}

template int UnregisterLuaCallback<CommandPost, unsigned __int64>(lua_State*);
template int UnregisterLuaCallback<CommandPost, char>(lua_State*);
template int RegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesName<unsigned __int64> >(lua_State*);
template int RegisterLuaCallback<CommandPost, unsigned __int64, PostMatchesTeam<unsigned __int64> >(lua_State*);
template int RegisterLuaCallback<CommandPost, char, PostMatchesName<char> >(lua_State*);
template int RegisterLuaCallback<CommandPost, char, PostMatchesTeam<char> >(lua_State*);
template void Event<CommandPost, unsigned __int64>::Unregister(Callback&);
template void Event<CommandPost, char>::Unregister(Callback&);
template Event<CommandPost, unsigned __int64>::Filter<PostMatchesName<unsigned __int64> >*
Event<CommandPost, unsigned __int64>::GetFilter(PostMatchesName<unsigned __int64>&, Callback*);
template Event<CommandPost, unsigned __int64>::Filter<PostMatchesTeam<unsigned __int64> >*
Event<CommandPost, unsigned __int64>::GetFilter(PostMatchesTeam<unsigned __int64>&, Callback*);
template Event<CommandPost, char>::Filter<PostMatchesName<char> >*
Event<CommandPost, char>::GetFilter(PostMatchesName<char>&, Callback*);
template Event<CommandPost, char>::Filter<PostMatchesTeam<char> >*
Event<CommandPost, char>::GetFilter(PostMatchesTeam<char>&, Callback*);

// TEMPLATE: SWBF2 0x0050ff80
// EventManager::RegisterUnregisterLuaCallback<CommandPost,unsigned __int64>

// TEMPLATE: SWBF2 0x00510010
// EventManager::RegisterRegisterLuaCallback<CommandPost,unsigned __int64>

// TEMPLATE: SWBF2 0x005101c0
// EventManager::RegisterRegisterLuaCallback<CommandPost,unsigned __int64,EventManager::PostMatchesName<unsigned __int64> >

// TEMPLATE: SWBF2 0x005103d0
// EventManager::RegisterRegisterLuaCallback<CommandPost,unsigned __int64,EventManager::PostMatchesTeam<unsigned __int64> >

// TEMPLATE: SWBF2 0x00510630
// EventManager::RegisterUnregisterLuaCallback<CommandPost,DamageOwner>

// TEMPLATE: SWBF2 0x005106c0
// EventManager::RegisterRegisterLuaCallback<CommandPost,DamageOwner>

// TEMPLATE: SWBF2 0x00510870
// EventManager::RegisterRegisterLuaCallback<CommandPost,DamageOwner,EventManager::PostMatchesName<DamageOwner> >

// TEMPLATE: SWBF2 0x00510a80
// EventManager::RegisterRegisterLuaCallback<CommandPost,DamageOwner,EventManager::PostMatchesTeam<DamageOwner> >

// TEMPLATE: SWBF2 0x00510ce0
// EventManager::RegisterUnregisterLuaCallback<CommandPost,char>

// TEMPLATE: SWBF2 0x00510d70
// EventManager::RegisterRegisterLuaCallback<CommandPost,char>

// TEMPLATE: SWBF2 0x00510f20
// EventManager::RegisterRegisterLuaCallback<CommandPost,char,EventManager::PostMatchesName<char> >

// TEMPLATE: SWBF2 0x00511140
// EventManager::RegisterRegisterLuaCallback<CommandPost,char,EventManager::PostMatchesTeam<char> >

// TEMPLATE: SWBF2 0x00510460
// EventManager::RegisterLuaCallback<CommandPost,unsigned __int64,EventManager::PostMatchesTeam<unsigned __int64> >

// TEMPLATE: SWBF2 0x00510c90
// EventManager::UnregisterLuaCallback<CommandPost,char>

// TEMPLATE: SWBF2 0x00510fc0
// EventManager::RegisterLuaCallback<CommandPost,char,EventManager::PostMatchesName<char> >

// TEMPLATE: SWBF2 0x005111d0
// EventManager::RegisterLuaCallback<CommandPost,char,EventManager::PostMatchesTeam<char> >

// TEMPLATE: SWBF2 0x005113d0
// EventManager::Event<CommandPost,char>::Unregister

// TEMPLATE: SWBF2 0x00511e40
// EventManager::Event<CommandPost,unsigned __int64>::GetFilter<EventManager::PostMatchesTeam<unsigned __int64> >

// TEMPLATE: SWBF2 0x00512060
// EventManager::Event<CommandPost,char>::GetFilter<EventManager::PostMatchesName<char> >

// TEMPLATE: SWBF2 0x00512110
// EventManager::Event<CommandPost,char>::GetFilter<EventManager::PostMatchesTeam<char> >

}

namespace EventManager
{
// SYNTHETIC: SWBF2 0x0050fde0
// EventManager::Event<CommandPost,unsigned __int64>::Callback::`scalar deleting destructor'
}

namespace EventManager
{
// SYNTHETIC: SWBF2 0x0050fe50
// EventManager::Event<CommandPost,DamageOwner>::Callback::`scalar deleting destructor'
}

namespace EventManager
{
// SYNTHETIC: SWBF2 0x0050fec0
// EventManager::Event<CommandPost,char>::Callback::`scalar deleting destructor'
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x00511ef0
// EventManager::Event<CommandPost,DamageOwner>::GetFilter<EventManager::PostMatchesName<DamageOwner> >
}

namespace EventManager
{
// TEMPLATE: SWBF2 0x00511fb0
// EventManager::Event<CommandPost,DamageOwner>::GetFilter<EventManager::PostMatchesTeam<DamageOwner> >
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x0050fdd0
// EventManager::Event<CommandPost,char>::Callback::operator()
template bool Event<CommandPost, char>::Callback::operator()(CommandPost&, char&);
}

namespace EventManager
{
namespace CommandPostEvent
{

// SYNTHETIC: SWBF2 0x00403140
// EventManager::CommandPostEvent::`dynamic initializer for 'mBeginNeutralize''
// SYNTHETIC: SWBF2 0x007675a0
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mBeginNeutralize''

// GLOBAL: SWBF2 0x007ebb90
// mBeginNeutralize
Event<CommandPost, unsigned __int64> mBeginNeutralize("BeginNeutralize");

// SYNTHETIC: SWBF2 0x00403170
// EventManager::CommandPostEvent::`dynamic initializer for 'mAbortNeutralize''
// SYNTHETIC: SWBF2 0x00767620
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mAbortNeutralize''

// GLOBAL: SWBF2 0x007eba60
// mAbortNeutralize
Event<CommandPost, unsigned __int64> mAbortNeutralize("AbortNeutralize");

// SYNTHETIC: SWBF2 0x004031a0
// EventManager::CommandPostEvent::`dynamic initializer for 'mFinishNeutralize''
// SYNTHETIC: SWBF2 0x007676a0
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mFinishNeutralize''

// GLOBAL: SWBF2 0x007ebb50
// mFinishNeutralize
Event<CommandPost, unsigned __int64> mFinishNeutralize("FinishNeutralize");

// SYNTHETIC: SWBF2 0x004031d0
// EventManager::CommandPostEvent::`dynamic initializer for 'mBeginCapture''
// SYNTHETIC: SWBF2 0x00767720
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mBeginCapture''

// GLOBAL: SWBF2 0x007ebb20
// mBeginCapture
Event<CommandPost, unsigned __int64> mBeginCapture("BeginCapture");

// SYNTHETIC: SWBF2 0x00403200
// EventManager::CommandPostEvent::`dynamic initializer for 'mAbortCapture''
// SYNTHETIC: SWBF2 0x007677a0
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mAbortCapture''

// GLOBAL: SWBF2 0x007ebaa0
// mAbortCapture
Event<CommandPost, unsigned __int64> mAbortCapture("AbortCapture");

// SYNTHETIC: SWBF2 0x00403230
// EventManager::CommandPostEvent::`dynamic initializer for 'mFinishCapture''
// SYNTHETIC: SWBF2 0x00767820
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mFinishCapture''

// GLOBAL: SWBF2 0x007ebac0
// mFinishCapture
Event<CommandPost, unsigned __int64> mFinishCapture("FinishCapture");

// SYNTHETIC: SWBF2 0x00403260
// EventManager::CommandPostEvent::`dynamic initializer for 'mKill''
// SYNTHETIC: SWBF2 0x007678a0
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mKill''

// GLOBAL: SWBF2 0x007eba80
// mKill
Event<CommandPost, DamageOwner> mKill("CommandPostKill");

// SYNTHETIC: SWBF2 0x00403290
// EventManager::CommandPostEvent::`dynamic initializer for 'mRespawn''
// SYNTHETIC: SWBF2 0x00767920
// EventManager::CommandPostEvent::`dynamic atexit destructor for 'mRespawn''

// GLOBAL: SWBF2 0x007ebb00
// mRespawn
Event<CommandPost, char> mRespawn("CommandPostRespawn");

}
}
