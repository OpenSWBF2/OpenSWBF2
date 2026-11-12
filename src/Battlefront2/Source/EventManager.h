#pragma once

#include "PebbleFL/Common/PblRef.h"
#include "NetGame.h"

namespace EventManager
{

extern char noContext;

// VTABLE: SWBF2 0x0079e17c
class Node : public PblRef
{
public:

    virtual ~Node()
    {
        Clear();
        DetachFromParent();
    }

    virtual const char* GetName();

    Node() : mParent(0), mChild(0), mSibling(0)
    {
    }

    void AttachToParent(Node* pParent);
    void DetachFromParent();

    void Clear();

    bool Contains(const Node* pNode) const
    {
        for (const Node* pChild = mChild; pChild; pChild = pChild->mSibling)
        {
            if (pChild == pNode || pChild->Contains(pNode))
            {
                return true;
            }
        }
        return false;
    }

    Node* mParent;
    Node* mChild;
    Node* mSibling;
};

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

private:
    const char* mName;
    Callback mRoot;

public:
    Event(const char* pcName) : mName(pcName) {}

    void Init() {}
    void Cleanup() { mRoot.Clear(); }

    const char* GetName() { return mName; }

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

    void Register(Callback& aCallback, Callback* aParent)
    {
        aCallback.AttachToParent(aParent ? aParent : &mRoot);
    }

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
        if (!NetGame::IsEnabled() || !NetGame::OnClient())
        {
            mRoot.Trigger(item, param);
        }
    }
};

void Init();
void Cleanup();
}
