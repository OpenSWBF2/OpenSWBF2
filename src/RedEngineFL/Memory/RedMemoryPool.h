#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblAsan.h"
#include "PebbleFL/Common/PblList.h"

class MemoryPool
{
public:

    MemoryPool(char const* pcLabel, int iSize);

    void Create(unsigned int uiCount, unsigned int uiSize, unsigned int uiGrow);

    void Reset();

    void ClearStats();

    void Setup(unsigned int uiCount, unsigned int uiSize, unsigned int uiGrow);

    bool IsPresent(void* pBlock) const
    {
        const char* pcPool = (const char*)mPool;
        if (pcPool == 0 || (const char*)pBlock < pcPool)
        {
            return false;
        }
        return (const char*)pBlock < pcPool + mCount * mSize;
    }

    // FUNCTION: SWBF2 0x004374e0
    unsigned int GetSize() const { return mSize; }
    // FUNCTION: SWBF2 0x004374f0
    unsigned int GetGrow() const { return mGrow; }

    PBL_ASAN_NO_SANITIZE void* Allocate(unsigned int uiSize);

    PBL_ASAN_NO_SANITIZE void Free(void* pBlock);

    PBL_ASAN_NO_SANITIZE void* AllocateAt(void* pBlock, unsigned int uiSize);

    PBL_ASAN_NO_SANITIZE void* AllocateRun(unsigned int uiCount, unsigned int uiSize);

    PBL_ASAN_NO_SANITIZE bool IsFreeBlock(const void* pBlock) const;

    ~MemoryPool();

    void Destroy();
    void SetCount(unsigned int uiCount);

    // FUNCTION: SWBF2 0x0044c530
    unsigned int GetCount() const { return mCount; }

    unsigned int GetPeak() const { return mPeak; }
    unsigned int GetGrown() const { return mGrown; }

    int GetIndex(void* pBlock) const
    {
        return ((char*)pBlock - (char*)mPool) / mSize;
    }

    bool IsFull() const
    {
        if (mUsed >= mCount)
        {
            ReportFull(mUsed + 1);
            return true;
        }
        return false;
    }

    bool IsSufficientSpace(unsigned int uCount) const
    {
        if (mUsed + uCount <= mCount)
        {
            return true;
        }
        ReportFull(mUsed + uCount);
        return false;
    }

    void ReportFull(unsigned int uWanted) const;
    void SetSize(unsigned int uiSize);

    static void CreateAll();
    static void DestroyAll();
    static MemoryPool* Find(char const* pcLabel);

    void Set(MemoryPool const& rPool);

    static void PoolErrorPostLoad();

    static bool sErrorOnExpansionPostLoad;

    static bool sErrorOnExpansion;

    PblList<MemoryPool>::Node mNode;
    char mLabel[32];
    unsigned int mSize;
    unsigned int mCount;
    unsigned int mGrow;
    unsigned int mUsed;
    int mHeap;
    void* mPool;
    bool mPoolOwner;
    void* mFree;
    unsigned int mPeak;
    unsigned int mGrown;
    mutable bool mFullReported;
    mutable bool mFullReportedStrict;
};
 
 
 
 
 
 
 
 
 
 

PblList<MemoryPool>* GetMemoryPoolList();
