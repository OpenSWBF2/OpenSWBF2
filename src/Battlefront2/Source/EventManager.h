#pragma once

#include "PebbleFL/Common/PblRef.h"

namespace EventManager
{

class Node : public PblRef
{
public:

    virtual ~Node();

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

void Init();
void Cleanup();
}
