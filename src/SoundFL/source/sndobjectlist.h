#pragma once

#include "PebbleFL/Common/PblListSingle.h"
#include "sndfreelist.h"

namespace Snd
{

class ObjectList : public PblListSingle
{
public:

    class Node
    {
    public:

        static FreeList smFreeList;

        static FreeList& GetFreeList() { return smFreeList; }

        static void operator delete(void* p)
        {
            GetFreeList().Free(p);
        }

        void* GetObjectA() const { return mObject; }

        PblListSingle::Node mNode;
        void* mObject;
    };

    class Itor : public PblListSingle::Itor
    {
    public:
        Itor() {}
        Itor(Node* pNode) : PblListSingle::Itor(&pNode->mNode) {}
        Itor(const ObjectList* pList) : PblListSingle::Itor(pList) {}
        Itor(const ObjectList& rList) : PblListSingle::Itor(rList) {}

        Node* GetNode()
        {
            return reinterpret_cast<Node*>(PblListSingle::Itor::GetNode());
        }
        void* GetObjectA(const ObjectList* pList)
        {
            if (PblListSingle::Itor::GetNode() != pList->GetTerminator())
            {
                return GetNode()->GetObjectA();
            }
            return 0;
        }
    };

    void Remove(void* pObject);

    Node* FindByObject(void* pObject);

    // FUNCTION: SWBF2 0x0052c740
    void Add(void* pObject)
    {
        Node* const pNode = (Node*)Node::smFreeList.Alloc();
        if (pNode)
        {
            pNode->mNode.mNext = 0;
            pNode->mObject = pObject;
            AddLast(&pNode->mNode);
        }
    }

    void AddBefore(Node* pBefore, void* pObject)
    {
        Node* pNode = (Node*)Node::smFreeList.Alloc();
        if (pNode)
        {
            pNode->mNode.mNext = 0;
            pNode->mObject = pObject;
        }
        PblListSingle::Node* pPrev = &pBefore->mNode;
        while (pPrev->mNext != &pBefore->mNode)
        {
            pPrev = pPrev->mNext;
        }
        pNode->mNode.mNext = pPrev->mNext;
        pPrev->mNext = &pNode->mNode;
    }

    ObjectList() {}

    // FUNCTION: SWBF2 0x0052c6c0
    ~ObjectList()
    {
        for (;;)
        {
            Node* const pNode = (Node*)mHead.mNext;

            if (pNode == (Node*)&mHead)
            {
                break;
            }

            if (pNode == 0)
            {
                continue;
            }

            if (pNode->mNode.mNext != 0)
            {
                pNode->mNode.Remove();
            }

            Node::smFreeList.Free(pNode);
        }

    }

    Node* FindByIndex(unsigned int uiIndex) const;
    unsigned int GetCount() const;
};

 
 

}
