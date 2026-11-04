#pragma once

template <class T, class KEY>
class PblHeap
{
public:
    struct RECORD
    {
        KEY mKey;
        T mObj;
    };

    PblHeap(int iMaxCount, KEY maxKey, void* pMem)
    {
        if (pMem)
        {
            mPool = (RECORD*)pMem;
            mAllocated = false;
        }
        else
        {
            mPool = new RECORD[iMaxCount + 1];
            mAllocated = true;
        }
        mMaxCount = iMaxCount;
        mPool[0].mKey = maxKey;
        mCount = 0;
    }

    void Flush() { mCount = 0; }

    KEY Value(int i) const { return mPool[i + 1].mKey; }
    bool IsEmpty() { return mCount == 0; }

    bool IsFull() { return mCount == GetMaxSize(); }

    T Get(int i) { return mPool[i + 1].mObj; }

    // TEMPLATE: SWBF2 0x0072b890
    // PblHeap<int,int>::Pop
    void Pop()
    {
        mPool[1] = mPool[mCount--];
        RECORD rec;
        rec = mPool[1];
        int i = 1;
        while (i <= (mCount >> 1))
        {
            int iChild = i + i;
            if (iChild < mCount && mPool[iChild].mKey < mPool[iChild + 1].mKey)
            {
                ++iChild;
            }
            if (rec.mKey >= mPool[iChild].mKey)
            {
                break;
            }
            mPool[i] = mPool[iChild];
            i = iChild;
        }
        mPool[i] = rec;
    }

    // TEMPLATE: SWBF2 0x00671340
    // PblHeap<PblHandle<GameObject>,int>::~PblHeap<PblHandle<GameObject>,int>
    ~PblHeap()
    {
        if (mAllocated)
        {
            delete[] mPool;
        }
    }

    void Push(T& obj, KEY key) { SortDown(++mCount, obj, key); }

    int GetCount() const { return mCount; }
    int GetMaxSize() const { return mMaxCount; }

    int FindLowest()
    {
        int lowest = mCount >> 1;
        int iEnd = mCount + 1;
        for (int i = lowest + 1; i < iEnd; ++i)
        {
            if (mPool[i].mKey < mPool[lowest].mKey)
            {
                lowest = i;
            }
        }
        return lowest - 1;
    }

    void ReplaceLowest(T& obj, KEY key, int iLowest) { SortDown(iLowest + 1, obj, key); }

private:
    void SortDown(int i, T& obj, KEY key)
    {
        int iParent = i >> 1;
        while (mPool[iParent].mKey < key)
        {
            mPool[i] = mPool[iParent];
            i = iParent;
            iParent >>= 1;
        }
        mPool[i].mKey = key;
        mPool[i].mObj = obj;
    }

public:

    int mCount;
    int mMaxCount;
    RECORD* mPool;
    bool mAllocated;
};
