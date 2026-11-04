#pragma once

#include "Battlefront2/Source/EventManager.h"
#include "PebbleFL/Common/PblHandle.h"
#include "Timer.h"
#include "LuaHelper.h"
#include "NetGame.h"

namespace EventManager
{

template <class ITEM, class PARAM>
class Event
{
public:

    class Callback : public Node
    {
    public:

        virtual ~Callback() {}
        // TEMPLATE: SWBF2 0x00519d00
        // EventManager::Event<RedRegion,Character>::Callback::operator()
        virtual bool operator()(ITEM& item, PARAM& param)
        {
            return true;
        }

        void Trigger(ITEM& item, PARAM& param);
    };

    template <class PREDICATE>
    class Filter : public Callback
    {
    public:

        Filter(PREDICATE& predicate)
            : mPredicate(predicate)
        {
        }

        virtual const char* GetName()
        {
            return PREDICATE::GetName();
        }
        virtual bool operator()(ITEM& item, PARAM& param)
        {
            return mPredicate(item, param);
        }

        PREDICATE mPredicate;
    };

    // TEMPLATE: SWBF2 0x0050e0c0
    // EventManager::Event<Character,DamageOwner>::Unregister

    // TEMPLATE: SWBF2 0x00511350
    // EventManager::Event<CommandPost,unsigned __int64>::Unregister

    // TEMPLATE: SWBF2 0x005142f0
    // EventManager::Event<EventManager::LuaObject,EventManager::LuaObject>::Unregister

    // TEMPLATE: SWBF2 0x0051c610
    // EventManager::Event<Timer,char>::Unregister

    void Unregister(Callback& callback)
    {
        Node* pParent = callback.mParent;
        callback.DetachFromParent();
        while (pParent && pParent->mParent && !pParent->mChild)
        {
            Node* pNode = pParent;
            pParent = pParent->mParent;
            pNode->DetachFromParent();
        }
    }
    template <class PREDICATE>
    Filter<PREDICATE>* GetFilter(PREDICATE& predicate, Callback* pParent = 0)
    {
        if (!pParent)
        {
            pParent = &mRoot;
        }

        for (Node* pNode = pParent->mChild; pNode; pNode = pNode->mSibling)
        {
            if (pNode->GetName() == PREDICATE::GetName() &&
                ((Filter<PREDICATE>*)pNode)->mPredicate == predicate)
            {
                pNode->AddRef();
                return (Filter<PREDICATE>*)pNode;
            }
        }

        Filter<PREDICATE>* pFilter = new Filter<PREDICATE>(predicate);
        pFilter->AttachToParent(pParent);
        return pFilter;
    }

    Event(char* pcName) : mName(pcName) {}

    void Init() {}
    void Cleanup() { mRoot.Clear(); }

    // TEMPLATE: SWBF2 0x0042cd60
    // EventManager::Event<Character,char>::Trigger
    // TEMPLATE: SWBF2 0x00489c50
    // EventManager::Event<GameObject,DamageOwner>::Trigger
    // TEMPLATE: SWBF2 0x004b97e0
    // EventManager::Event<Character,GameObject>::Trigger
    // TEMPLATE: SWBF2 0x00525c30
    // EventManager::Event<FlagItem,Character>::Trigger
    void Trigger(ITEM& item, PARAM& param)
    {
        if (!NetGame::IsEnabled() || !NetGame::IsClient())
        {
            mRoot.Trigger(item, param);
        }
    }

    char* mName;
    Callback mRoot;
};

// VTABLE: SWBF2 0x0079e250
// class LuaCallback<CommandPost, struct DamageOwner>
template <class ITEM, class PARAM>
class LuaCallback : public Event<ITEM, PARAM>::Callback
{
public:

    LuaCallback(int iRef);
    virtual ~LuaCallback();
    virtual bool operator()(ITEM& item, PARAM& param);

    int mFunction;
};

template <class ITEM>
bool LuaPushItem(ITEM& item);

template <>
bool LuaPushItem<char>(char& item);

class TimerMatches
{
public:

    TimerMatches(const Timer* pTimer) : mTimer(pTimer) {}
    TimerMatches(const char* pcName);

    static const char* GetName() { return ""; }

    bool operator==(const TimerMatches& other) const
    {
        return mTimer == other.mTimer;
    }

    bool operator()(Timer& timer, char&) const
    {
        return mTimer == &timer;
    }

    PblHandle<Timer const> mTimer;
};

namespace TimerEvent
{
extern Event<Timer, char> mElapsed;
void Cleanup();
}

}

namespace EventManager
{
template <class ITEM, class PARAM>
inline LuaCallback<ITEM, PARAM>::~LuaCallback()
{
    if (mFunction != -1)
    {
        lua_unref(LuaHelper::mState, mFunction);
    }
}
}
