#pragma once

class PblListSingle
{
public:
    class Node
    {
    public:

        Node()
        {
            mNext = 0;
        }

        // FUNCTION: SWBF2 0x0049e310
        ~Node()
        {
            if (IsAttached())
            {
                Remove();
            }
        }

        bool IsAttached() const { return mNext != 0; }

        void* GetObjectA(const PblListSingle* pList, unsigned int uiOffset) const;

        Node* GetNext() const { return mNext; }

        Node* GetPrevious() const
        {
            const Node* pNext = this;
            const Node* pPrev;
            do
            {
                pPrev = pNext;
                pNext = pNext->mNext;
            } while (this != pNext);
            return const_cast<Node*>(pPrev);
        }

        void Remove()
        {
            GetPrevious()->mNext = mNext;
            mNext = 0;
        }

        Node* mNext;
    };

    PblListSingle()
    {
        mHead.mNext = &mHead;
    }

    // FUNCTION: SWBF2 0x0049e340
    ~PblListSingle()
    {
        mHead.mNext = 0;
    }

    bool IsEmpty() const
    {
        return mHead.mNext == &mHead;
    }

    void Clear()
    {
        mHead.mNext = &mHead;
    }

    void AddAfter(Node* pAt, Node* pNode)
    {
        pNode->mNext = pAt->mNext;
        pAt->mNext = pNode;
    }

    void AddFirst(Node* pNode)
    {
        pNode->mNext = mHead.mNext;
        mHead.mNext = pNode;
    }
    void AddLast(Node* pNode)
    {
        Node* pPrev = mHead.GetPrevious();
        pNode->mNext = pPrev->mNext;
        pPrev->mNext = pNode;
    }

    Node* GetFirst() const { return mHead.mNext; }

    Node* FindByIndex(unsigned int uiIndex) const
    {
        Node* pNode = GetFirst();
        while (pNode != GetTerminator())
        {
            if (uiIndex-- == 0)
            {
                return pNode;
            }
            pNode = pNode->GetNext();
        }
        return 0;
    }

    const Node* GetTerminator() const { return &mHead; }

    class Itor
    {
    public:
        Itor() : mNode(0) {}
        Itor(const Itor& itor) : mNode(itor.mNode) {}
        Itor(Node* pNode) : mNode(pNode) {}
        Itor(const PblListSingle* pList) : mNode(pList->mHead.mNext) {}
        Itor(const PblListSingle& rList) : mNode(rList.mHead.mNext) {}

        Node* GetNode() { return mNode; }
        Itor& operator++()
        {
            mNode = mNode->GetNext();
            return *this;
        }
        Itor& operator--()
        {
            mNode = mNode->GetPrevious();
            return *this;
        }

    private:
        Node* mNode;
    };

    Itor Begin() const { return Itor(mHead.mNext); }
    Itor End() { return Itor(&mHead); }

    Node mHead;
};

inline void* PblListSingle::Node::GetObjectA(const PblListSingle* pList, unsigned int uiOffset) const
{
    return this == pList->GetTerminator() ? 0 : (char*)this - uiOffset;
}
