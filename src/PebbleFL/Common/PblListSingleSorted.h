#pragma once

#include <stddef.h>
#include "PblListSingle.h"

// SYNTHETIC: SWBF2 0x00748bf0
// PblListSingleSorted::~PblListSingleSorted
class PblListSingleSorted : protected PblListSingle
{
public:
    enum Flags
    {
        flags_sortOnAdd = 1
    };

    class Node
    {
    public:
        explicit Node(float fSortValue) : mSortValue(fSortValue) {}

        void* GetObjectA(const PblListSingleSorted* pList, unsigned int uiOffset) const;

        float mSortValue;
        PblListSingle::Node mNode;
    };

    class Itor : public PblListSingle::Itor
    {
    public:
        Itor(const PblListSingleSorted* pList) : PblListSingle::Itor(pList) {}
        Itor(const PblListSingleSorted& rList) : PblListSingle::Itor(rList) {}

        Node* GetNode(const PblListSingleSorted* pList)
        {
            PblListSingle::Node* pNode = PblListSingle::Itor::GetNode();
            return pNode == ((const PblListSingle*)pList)->GetTerminator()
                ? const_cast<Node*>(pList->GetTerminator())
                : (Node*)((char*)pNode - offsetof(Node, mNode));
        }
    };

    explicit PblListSingleSorted(bool bSortOnAdd)
    {
        mFlags = bSortOnAdd ? (unsigned char)flags_sortOnAdd : (unsigned char)0;
    }

    const Node* GetTerminator() const { return (const Node*)this; }

    Node* GetFirst() const
    {
        PblListSingle::Node* pNode = PblListSingle::GetFirst();
        return pNode == PblListSingle::GetTerminator()
            ? (Node*)pNode
            : (Node*)((char*)pNode - offsetof(Node, mNode));
    }

    // FUNCTION: SWBF2 0x00748c00
    void Add(Node* pNode)
    {
        if (mFlags & flags_sortOnAdd)
        {
            for (Itor itor(this); ; ++itor)
            {
                Node* const pCur = itor.GetNode(this);
                if (pCur == GetTerminator())
                {
                    break;
                }
                if (pCur->mSortValue > pNode->mSortValue)
                {
                    PblListSingle::AddAfter(&pCur->mNode, &pNode->mNode);
                    return;
                }
            }
            PblListSingle::AddLast(&pNode->mNode);
        }
        else
        {
            PblListSingle::AddLast(&pNode->mNode);
        }
    }

private:
    unsigned char mFlags;
};

inline void* PblListSingleSorted::Node::GetObjectA(const PblListSingleSorted* pList, unsigned int uiOffset) const
{
    return this == pList->GetTerminator() ? 0 : (char*)this - uiOffset;
}
