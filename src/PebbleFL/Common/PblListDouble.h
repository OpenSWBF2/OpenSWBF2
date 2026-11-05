#pragma once

class PblListDouble
{
public:
    class Node
    {
    public:

        Node()
        {
            mNext = 0;
            mPrev = 0;
        }

        // FUNCTION: SWBF2 0x004d0d00
        ~Node()
        {
            if (IsAttached())
            {
                Remove();
            }
        }

        bool IsAttached() const
        {
            return mNext != 0 && mPrev != 0;
        }

        void* GetObjectA(const PblListDouble* pList, unsigned int uiOffset) const;

        // FUNCTION: SWBF2 0x004d0d30
        void Remove()
        {
            mNext->mPrev = mPrev;
            mPrev->mNext = mNext;
            mPrev = 0;
            mNext = 0;
        }

        Node* mNext;
        Node* mPrev;
    };

    class Itor
    {
    public:
        Itor(Node* pNode) : mNode(pNode) {}
        Itor(const Itor& itor) : mNode(itor.mNode) {}
        Itor(const PblListDouble* pList) : mNode(pList->mTerminator.mNext) {}
        Itor(const PblListDouble& list) : mNode(list.mTerminator.mNext) {}
        Itor() : mNode(0) {}

        const Itor& operator=(Node* pNode)
        {
            mNode = pNode;
            return *this;
        }

        Itor& operator++()
        {
            mNode = mNode->mNext;
            return *this;
        }

        Itor& operator--()
        {
            mNode = mNode->mPrev;
            return *this;
        }

        Node* GetNode() { return mNode; }

    private:
        Node* mNode;
    };

    PblListDouble()
    {
        mTerminator.mNext = &mTerminator;
        mTerminator.mPrev = &mTerminator;
    }

    // FUNCTION: SWBF2 0x004d0d50
    ~PblListDouble()
    {
        mTerminator.mNext = 0;
        mTerminator.mPrev = 0;
    }

    void AddBefore(Node* pAt, Node* pNode)
    {
        pNode->mNext = pAt;
        pNode->mPrev = pAt->mPrev;
        pAt->mPrev->mNext = pNode;
        pAt->mPrev = pNode;
    }

    void AddAfter(Node* oldNode, Node* newNode)
    {
        AddBefore(oldNode->mNext, newNode);
    }

    void AddFirst(Node* pNode)
    {
        Node* pAt = mTerminator.mNext;
        pNode->mNext = pAt;
        pNode->mPrev = pAt->mPrev;
        pAt->mPrev->mNext = pNode;
        pAt->mPrev = pNode;
    }

    // FUNCTION: SWBF2 0x004d0d60
    void Clear()
    {
        while (mTerminator.mNext != &mTerminator)
        {
            mTerminator.mNext->Remove();
        }
    }

    unsigned int GetCount() const
    {
        unsigned int uCount = 0;
        for (const Node* pNode = mTerminator.mNext; pNode != &mTerminator; pNode = pNode->mNext)
        {
            ++uCount;
        }
        return uCount;
    }

    void AddLast(Node* pNode)
    {
        pNode->mNext = &mTerminator;
        pNode->mPrev = mTerminator.mPrev;
        mTerminator.mPrev->mNext = pNode;
        mTerminator.mPrev = pNode;
    }

    const Node* GetTerminator() const { return &mTerminator; }

    Node mTerminator;
};

inline void* PblListDouble::Node::GetObjectA(const PblListDouble* pList, unsigned int uiOffset) const
{
    return this == pList->GetTerminator() ? 0 : (char*)this - uiOffset;
}
