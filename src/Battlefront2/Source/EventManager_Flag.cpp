#include "EventManager_Flag.h"
#include "LuaHandleArg.h"
#include "LuaHelper.h"
#include "Controllable.h"
#include "Entity.h"
#include "LuaFL/include/lauxlib.h"
#include "EventManager_LuaUser.h"
#include "NetGame.h"

namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, Character> mPickUp;
extern Event<FlagItem, Character> mDrop;
extern Event<FlagItem, Character> mReturn;
extern Event<FlagItem, Character> mCapture;
extern Event<FlagItem, char> mReset;
void Cleanup();
}
}

namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, Character> mPickUp;
}
}
namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, Character> mDrop;
}
}
namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, Character> mReturn;
}
}
namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, Character> mCapture;
}
}
namespace EventManager
{
namespace FlagEvent
{
extern Event<FlagItem, char> mReset;
}
}

namespace EventManager
{

// FUNCTION: SWBF2 0x005122f0
template <>
bool LuaPushItem<FlagItem>(FlagItem& flag)
{
    lua_pushlightuserdata(LuaHelper::mState, &flag);
    return true;
}






// TEMPLATE: SWBF2 0x005133b0
// EventManager::LuaCallback<FlagItem,char>::LuaCallback<FlagItem,char>
template <class ITEM, class PARAM>
LuaCallback<ITEM, PARAM>::LuaCallback(int iRef)
{
    mFunction = -1;
    if (LuaHelper::PushRefProc(iRef))
    {
        mFunction = luaL_ref(LuaHelper::mState, LUA_REGISTRYINDEX);
    }
}



template <class ITEM, class PARAM>
bool LuaCallback<ITEM, PARAM>::operator()(ITEM& item, PARAM& param)
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

// TEMPLATE: SWBF2 0x00513250
// EventManager::LuaCallback<FlagItem,Character>::operator()

// TEMPLATE: SWBF2 0x00513440
// EventManager::LuaCallback<FlagItem,char>::operator()

// TEMPLATE: SWBF2 0x00513320
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesTeam>::operator()

// TEMPLATE: SWBF2 0x005132c0
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesName>::operator()

// TEMPLATE: SWBF2 0x00513350
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesClass>::operator()

// TEMPLATE: SWBF2 0x00513310
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesName>::GetName

// TEMPLATE: SWBF2 0x00513340
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesTeam>::GetName

// TEMPLATE: SWBF2 0x005133a0
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesClass>::GetName

// SYNTHETIC: SWBF2 0x005134a0
// EventManager::LuaCallback<FlagItem,Character>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00513690
// EventManager::LuaCallback<FlagItem,char>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00513540
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesName>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x005135b0
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesTeam>::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00513620
// EventManager::Event<FlagItem,Character>::Filter<EventManager::CharacterMatchesClass>::`scalar deleting destructor'

// TEMPLATE: SWBF2 0x00512490
// EventManager::Event<FlagItem,Character>::Callback::operator()

// TEMPLATE: SWBF2 0x00512500
// EventManager::Event<FlagItem,char>::Callback::operator()

template bool Event<FlagItem, Character>::Callback::operator()(FlagItem&, Character&);
template bool Event<FlagItem, char>::Callback::operator()(FlagItem&, char&);
template class LuaCallback<FlagItem, Character>;
template class LuaCallback<FlagItem, char>;
template class Event<FlagItem, Character>::Filter<CharacterMatchesName>;
template class Event<FlagItem, Character>::Filter<CharacterMatchesTeam>;
template class Event<FlagItem, Character>::Filter<CharacterMatchesClass>;

}

namespace EventManager
{

// FUNCTION: SWBF2 0x005123f0
void FlagEvent::Cleanup()
{
    mPickUp.Cleanup();
    mDrop.Cleanup();
    mReturn.Cleanup();
    mCapture.Cleanup();
    mReset.Cleanup();
}
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x005124a0
// EventManager::Event<FlagItem,char>::Callback::~Callback
template Event<FlagItem, char>::Callback::~Callback();
}

namespace EventManager
{

int DummyLuaCallback(lua_State* L);
namespace FlagEvent
{
void Init();
}

template <class ITEM, class PARAM>
int UnregisterLuaCallback(lua_State*)
{
    Event<ITEM, PARAM>* pEvent =
        (Event<ITEM, PARAM>*)lua_topointer(LuaHelper::mState, lua_upvalueindex(1));
    if (lua_islightuserdata(LuaHelper::mState, 1))
    {
        typename Event<ITEM, PARAM>::Callback* pCallback =
            (typename Event<ITEM, PARAM>::Callback*)LuaHandleArg::Get(LuaHelper::mState, 1);
        if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)
        {
            pEvent->Unregister(*pCallback);
        }
    }
    return 0;
}

// TEMPLATE: SWBF2 0x00513020
// EventManager::RegisterLuaCallback<FlagItem,char>
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
        pCallback->AttachToParent(&pEvent->mRoot);
        luaL_unref(LuaHelper::mState, LUA_REGISTRYINDEX, iRef);
        lua_pushlightuserdata(LuaHelper::mState, pCallback);
        pCallback->Release();
        return 1;
    }
    lua_pushnil(LuaHelper::mState);
    return 1;
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
        pCallback->AttachToParent(pFilter ? (Node*)pFilter : (Node*)&pEvent->mRoot);
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
void RegisterUnregisterLuaCallback(Event<ITEM, PARAM>& event, lua_CFunction pfnClient)
{
    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "Release%s", event.mName);
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

template <class ITEM, class PARAM>
void RegisterRegisterLuaCallback(Event<ITEM, PARAM>& event, lua_CFunction pfnClient)
{
    char acName[64];
    _snprintf(acName, sizeof(acName) - 1, "On%s", event.mName);
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
    _snprintf(acName, sizeof(acName) - 1, "On%s%s", event.mName, PREDICATE::GetName());
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

// FUNCTION: SWBF2 0x00512310
void FlagEvent::Init()
{
    mPickUp.Init();
    RegisterUnregisterLuaCallback(mPickUp, DummyLuaCallback);
    RegisterRegisterLuaCallback(mPickUp, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesName>(mPickUp, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesTeam>(mPickUp, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesClass>(mPickUp, DummyLuaCallback);

    mDrop.Init();
    RegisterUnregisterLuaCallback(mDrop, DummyLuaCallback);
    RegisterRegisterLuaCallback(mDrop, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesName>(mDrop, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesTeam>(mDrop, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesClass>(mDrop, DummyLuaCallback);

    mReturn.Init();
    RegisterUnregisterLuaCallback(mReturn, DummyLuaCallback);
    RegisterRegisterLuaCallback(mReturn, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesName>(mReturn, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesTeam>(mReturn, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesClass>(mReturn, DummyLuaCallback);

    mCapture.Init();
    RegisterUnregisterLuaCallback(mCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback(mCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesName>(mCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesTeam>(mCapture, DummyLuaCallback);
    RegisterRegisterLuaCallback<FlagItem, Character, CharacterMatchesClass>(mCapture, DummyLuaCallback);

    mReset.Init();
    RegisterUnregisterLuaCallback(mReset, DummyLuaCallback);
    RegisterRegisterLuaCallback(mReset, DummyLuaCallback);
}

template int UnregisterLuaCallback<FlagItem, Character>(lua_State*);
template int UnregisterLuaCallback<FlagItem, char>(lua_State*);
template int RegisterLuaCallback<FlagItem, char>(lua_State*);
template int RegisterLuaCallback<FlagItem, Character>(lua_State*);
template int RegisterLuaCallback<FlagItem, Character, CharacterMatchesName>(lua_State*);
template int RegisterLuaCallback<FlagItem, Character, CharacterMatchesTeam>(lua_State*);
template int RegisterLuaCallback<FlagItem, Character, CharacterMatchesClass>(lua_State*);
template void Event<FlagItem, Character>::Unregister(Callback&);
template void Event<FlagItem, char>::Unregister(Callback&);
template Event<FlagItem, Character>::Filter<CharacterMatchesName>*
Event<FlagItem, Character>::GetFilter(CharacterMatchesName&, Callback*);
template Event<FlagItem, Character>::Filter<CharacterMatchesTeam>*
Event<FlagItem, Character>::GetFilter(CharacterMatchesTeam&, Callback*);
template Event<FlagItem, Character>::Filter<CharacterMatchesClass>*
Event<FlagItem, Character>::GetFilter(CharacterMatchesClass&, Callback*);

// TEMPLATE: SWBF2 0x00512430
// EventManager::Node::~Node

// TEMPLATE: SWBF2 0x00512640
// EventManager::RegisterUnregisterLuaCallback<FlagItem,Character>

// TEMPLATE: SWBF2 0x005126d0
// EventManager::RegisterRegisterLuaCallback<FlagItem,Character>

// TEMPLATE: SWBF2 0x00512880
// EventManager::RegisterRegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesName>

// TEMPLATE: SWBF2 0x00512a90
// EventManager::RegisterRegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesTeam>

// TEMPLATE: SWBF2 0x00512ca0
// EventManager::RegisterRegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesClass>

// TEMPLATE: SWBF2 0x00512f00
// EventManager::RegisterUnregisterLuaCallback<FlagItem,char>

// TEMPLATE: SWBF2 0x00512f90
// EventManager::RegisterRegisterLuaCallback<FlagItem,char>

// TEMPLATE: SWBF2 0x005125f0
// EventManager::UnregisterLuaCallback<FlagItem,Character>

// TEMPLATE: SWBF2 0x00512eb0
// EventManager::UnregisterLuaCallback<FlagItem,char>

// TEMPLATE: SWBF2 0x00513140
// EventManager::Event<FlagItem,Character>::Unregister

// TEMPLATE: SWBF2 0x00513180
// EventManager::Event<FlagItem,char>::Unregister

// TEMPLATE: SWBF2 0x005131c0
// EventManager::LuaCallback<FlagItem,Character>::LuaCallback<FlagItem,Character>

// TEMPLATE: SWBF2 0x00513730
// EventManager::Event<FlagItem,Character>::GetFilter<EventManager::CharacterMatchesName>

// TEMPLATE: SWBF2 0x005137e0
// EventManager::Event<FlagItem,Character>::GetFilter<EventManager::CharacterMatchesTeam>

// TEMPLATE: SWBF2 0x00513890
// EventManager::Event<FlagItem,Character>::GetFilter<EventManager::CharacterMatchesClass>

// TEMPLATE: SWBF2 0x00512760
// EventManager::RegisterLuaCallback<FlagItem,Character>

// TEMPLATE: SWBF2 0x00512910
// EventManager::RegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesName>

// TEMPLATE: SWBF2 0x00512b20
// EventManager::RegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesTeam>

// TEMPLATE: SWBF2 0x00512d30
// EventManager::RegisterLuaCallback<FlagItem,Character,EventManager::CharacterMatchesClass>

}

namespace EventManager
{
namespace FlagEvent
{

// SYNTHETIC: SWBF2 0x00403380
// EventManager::FlagEvent::`dynamic initializer for 'mPickUp''
// SYNTHETIC: SWBF2 0x00767ba0
// EventManager::FlagEvent::`dynamic atexit destructor for 'mPickUp''

// GLOBAL: SWBF2 0x007ebed0
// mPickUp
Event<FlagItem, Character> mPickUp("FlagPickUp");

// SYNTHETIC: SWBF2 0x004033b0
// EventManager::FlagEvent::`dynamic initializer for 'mDrop''
// SYNTHETIC: SWBF2 0x00767c20
// EventManager::FlagEvent::`dynamic atexit destructor for 'mDrop''

// GLOBAL: SWBF2 0x007ebcb0
// mDrop
Event<FlagItem, Character> mDrop("FlagDrop");

// SYNTHETIC: SWBF2 0x004033e0
// EventManager::FlagEvent::`dynamic initializer for 'mReturn''
// SYNTHETIC: SWBF2 0x00767ca0
// EventManager::FlagEvent::`dynamic atexit destructor for 'mReturn''

// GLOBAL: SWBF2 0x007ebea0
// mReturn
Event<FlagItem, Character> mReturn("FlagReturn");

// SYNTHETIC: SWBF2 0x00403410
// EventManager::FlagEvent::`dynamic initializer for 'mCapture''
// SYNTHETIC: SWBF2 0x00767d20
// EventManager::FlagEvent::`dynamic atexit destructor for 'mCapture''

// GLOBAL: SWBF2 0x007ebe60
// mCapture
Event<FlagItem, Character> mCapture("FlagCapture");

// SYNTHETIC: SWBF2 0x00403440
// EventManager::FlagEvent::`dynamic initializer for 'mReset''
// SYNTHETIC: SWBF2 0x00767da0
// EventManager::FlagEvent::`dynamic atexit destructor for 'mReset''

// GLOBAL: SWBF2 0x007ebe80
// mReset
Event<FlagItem, char> mReset("FlagReset");

}
}

// SYNTHETIC: SWBF2 0x00512510
// EventManager::Event<FlagItem,Character>::Callback::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00512580
// EventManager::Event<FlagItem,char>::Callback::`scalar deleting destructor'
