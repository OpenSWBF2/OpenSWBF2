#include <string.h>
#ifdef OPENSWBF2_TEST
#include <stdlib.h>
#endif

#include "RedMemoryPool.h"
#include "RedMemory.h"
#include "RedMemoryReport.h"
#include "RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

#ifdef OPENSWBF2_TEST
static PBL_ASAN_NO_SANITIZE void* PerturbShuffleChunk(void* pChunk, unsigned int uiCount, unsigned int uiSize)
{
    if (uiCount < 2 || (RedPerturbModes() & kRedPerturbPool) == 0)
    {
        return pChunk;
    }
    unsigned int* puOrder = (unsigned int*)malloc(uiCount * sizeof(unsigned int));
    if (puOrder == 0)
    {
        return pChunk;
    }
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        puOrder[i] = i;
    }
    for (unsigned int i = uiCount - 1; i > 0; --i)
    {
        unsigned int uiDraw = 0;
        if (!RedPerturbDraw(kRedPerturbPool, &uiDraw))
        {
            free(puOrder);
            return pChunk;
        }
        unsigned int j = uiDraw % (i + 1);
        unsigned int uiSwap = puOrder[i];
        puOrder[i] = puOrder[j];
        puOrder[j] = uiSwap;
    }
    char* pcChunk = (char*)pChunk;
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        char* pcSlot = pcChunk + puOrder[i] * uiSize;
        *(char**)pcSlot = i + 1 < uiCount ? pcChunk + puOrder[i + 1] * uiSize : 0;
    }
    void* pHead = pcChunk + puOrder[0] * uiSize;
    free(puOrder);
    return pHead;
}
#endif

// FUNCTION: SWBF2 0x006dc640
PblList<MemoryPool>* GetMemoryPoolList()
{
    // GLOBAL: SWBF2 0x0099e7d0
    static PblList<MemoryPool> sPoolList;
    return &sPoolList;
}

// FUNCTION: SWBF2 0x006dc2a0
MemoryPool::MemoryPool(char const* pcLabel, int iSize)
{
    mPoolOwner = true;
    mSize = ((unsigned int)iSize + 3) & ~3u;
    mGrow = 0;
    Reset();
    ClearStats();
    strncpy(mLabel, pcLabel, sizeof(mLabel) - 1);
    mLabel[sizeof(mLabel) - 1] = '\0';

    PblList<MemoryPool>* pList = GetMemoryPoolList();
    pList->AddLast(&mNode, this);
}

// FUNCTION: SWBF2 0x006dc440
void MemoryPool::Create(unsigned int uiCount, unsigned int uiSize, unsigned int uiGrow)
{
    if (uiSize == 0)
    {
        return;
    }

    uiSize = (uiSize + 3) & ~3u;
    if (uiCount != 0 && !RedScriptLimits::PoolBytesOk(uiCount, uiSize, RedScriptLimits::kPoolBytesMax))
    {
        LOG(LogMemory, Warning, "Memory pool \"%s\" refused count %u size %u", mLabel, uiCount, uiSize);
        uiCount = 0;
    }
    mSize = uiSize;
    mCount = uiCount;
    if (uiSize > 0 && uiGrow == 0)
    {
        uiGrow = (uiSize + 63) / uiSize;
    }
    mGrow = uiGrow;
    mUsed = 0;
    mHeap = __RedCurrHeap;
    if (uiCount == 0)
    {
        return;
    }

    mPoolOwner = true;
    void* pChunk = RedAllocFromHeap(__RedCurrHeap, mCount * mSize, 0);
    if (pChunk == 0)
    {
        LOG(LogMemory, Warning, "Memory pool \"%s\" chunk of %u bytes not allocated", mLabel, mCount * mSize);
        mCount = 0;
        mPool = 0;
        mFree = 0;
        return;
    }
    mPool = pChunk;

    unsigned int i;
    for (i = 0; i < mCount - 1; ++i)
    {
        *(char**)((char*)mPool + i * mSize) = (char*)mPool + (i + 1) * mSize;
    }
    *(char**)((char*)mPool + (mCount - 1) * mSize) = 0;
    mFree = mPool;
    PblAsanPoison(mPool, mSize * mCount);
#ifdef OPENSWBF2_TEST
    mFree = PerturbShuffleChunk(mPool, mCount, mSize);
#endif
}

// GLOBAL: SWBF2 0x0099e7c8
bool MemoryPool::sErrorOnExpansion;

// FUNCTION: SWBF2 0x006dc370
PBL_ASAN_NO_SANITIZE void* MemoryPool::Allocate(unsigned int uiSize)
{
    if (mPool == 0)
    {
        Create(mCount, mSize, mGrow);
    }

    if (mFree == 0)
    {
        sErrorOnExpansion = true;
        ReportFull(mUsed + 1);
        if (mHeap == -1)
        {
            mHeap = __RedCurrHeap;
        }

        if (!RedScriptLimits::PoolBytesOk(mGrow, mSize, RedScriptLimits::kPoolBytesMax) || mGrow == 0)
        {
            LOG(LogMemory, Warning, "Memory pool \"%s\" refused growth %u size %u", mLabel, mGrow, mSize);
            return 0;
        }

        int iOldHeap = RedSetCurrentHeap(mHeap);
        mFree = RedAllocFromHeap(__RedCurrHeap, mSize * mGrow, 0);
        RedSetCurrentHeap(iOldHeap);
        if (mFree == 0)
        {
            return 0;
        }

        mCount += mGrow;
        ++mGrown;
        unsigned int i;
        for (i = 0; i < mGrow - 1; ++i)
        {
            *(char**)((char*)mFree + i * mSize) = (char*)mFree + (i + 1) * mSize;
        }
        *(char**)((char*)mFree + (mGrow - 1) * mSize) = 0;
        PblAsanPoison(mFree, mSize * mGrow);

        if (mPool == 0)
        {
            mPool = mFree;
        }
#ifdef OPENSWBF2_TEST
        mFree = PerturbShuffleChunk(mFree, mGrow, mSize);
#endif
    }

    void* pBlock = mFree;
    mFree = *(void**)pBlock;
    ++mUsed;
    if (mUsed > mPeak)
    {
        mPeak = mUsed;
    }
#ifdef OPENSWBF2_TEST
    if ((RedPerturbModes() & kRedPerturbFill) != 0)
    {
        PblAsanUnpoison(pBlock, mSize);
        memset(pBlock, 0xA5, mSize);
        PblAsanPoison(pBlock, mSize);
    }
#endif
    PblAsanUnpoison(pBlock, uiSize < mSize ? uiSize : mSize);
    return pBlock;
}

PBL_ASAN_NO_SANITIZE bool MemoryPool::IsFreeBlock(const void* pBlock) const
{
    for (const char* pcNode = (const char*)mFree; pcNode != 0; pcNode = *(const char* const*)pcNode)
    {
        if (pcNode == pBlock)
        {
            return true;
        }
    }
    return false;
}

PBL_ASAN_NO_SANITIZE void* MemoryPool::AllocateAt(void* pBlock, unsigned int uiSize)
{
    char* pcPrev = 0;
    for (char* pcNode = (char*)mFree; pcNode != 0; pcPrev = pcNode, pcNode = *(char**)pcNode)
    {
        if (pcNode != pBlock)
        {
            continue;
        }
        if (pcPrev != 0)
        {
            *(void**)pcPrev = *(void**)pcNode;
        }
        else
        {
            mFree = *(void**)pcNode;
        }
        ++mUsed;
        if (mUsed > mPeak)
        {
            mPeak = mUsed;
        }
#ifdef OPENSWBF2_TEST
        if ((RedPerturbModes() & kRedPerturbFill) != 0)
        {
            PblAsanUnpoison(pcNode, mSize);
            memset(pcNode, 0xA5, mSize);
            PblAsanPoison(pcNode, mSize);
        }
#endif
        PblAsanUnpoison(pcNode, uiSize < mSize ? uiSize : mSize);
        return pcNode;
    }
    return 0;
}

PBL_ASAN_NO_SANITIZE void* MemoryPool::AllocateRun(unsigned int uiCount, unsigned int uiSize)
{
    if (mPool == 0)
    {
        Create(mCount, mSize, mGrow);
    }
    if (uiCount == 0 || mUsed + uiCount > mCount)
    {
        return 0;
    }
    char* pcBest = 0;
    char* pcRun = 0;
    char* pcLast = 0;
    unsigned int uiRun = 0;
    bool bSorted = true;
    for (char* pcNode = (char*)mFree; pcNode != 0; pcLast = pcNode, pcNode = *(char**)pcNode)
    {
        if (pcLast != 0 && pcNode <= pcLast)
        {
            bSorted = false;
            break;
        }
        if (pcLast != 0 && pcNode == pcLast + mSize)
        {
            ++uiRun;
        }
        else
        {
            pcRun = pcNode;
            uiRun = 1;
        }
        if (uiRun == uiCount)
        {
            pcBest = pcRun;
            break;
        }
    }
    for (char* pcNode = bSorted ? 0 : (char*)mFree; pcNode != 0; pcNode = *(char**)pcNode)
    {
        if (pcBest != 0 && pcNode >= pcBest)
        {
            continue;
        }
        unsigned int i = 1;
        while (i < uiCount && IsFreeBlock(pcNode + i * mSize))
        {
            ++i;
        }
        if (i == uiCount)
        {
            pcBest = pcNode;
        }
    }
    if (pcBest == 0)
    {
        return 0;
    }
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        AllocateAt(pcBest + i * mSize, uiSize);
    }
    return pcBest;
}

void MemoryPool::ReportFull(unsigned int uWanted) const
{
    if (mFullReported)
    {
        if (mFullReportedStrict || !sErrorOnExpansionPostLoad)
        {
            return;
        }
    }
    mFullReported = true;
    mFullReportedStrict = sErrorOnExpansionPostLoad;
    unsigned int uPeak = uWanted > GetPeak() ? uWanted : GetPeak();
    char acLine[128];
    RedMemoryReport::FormatPoolFull(mLabel, uPeak, acLine, sizeof(acLine));
    if (sErrorOnExpansionPostLoad)
    {
        LOG(LogMemory, Error, "%s", acLine);
    }
    else
    {
        LOG(LogMemory, Warning, "%s", acLine);
    }
}

// FUNCTION: SWBF2 0x006dc5f0
PBL_ASAN_NO_SANITIZE void MemoryPool::Free(void* pBlock)
{
    if (pBlock == 0)
    {
        return;
    }

    char* pcBlock = (char*)pBlock;
    char* pcNode = (char*)mFree;
#ifdef OPENSWBF2_TEST
    if (pcNode == 0 || pcBlock <= pcNode || (RedPerturbModes() & kRedPerturbPool) != 0)
#else
    if (pcNode == 0 || pcBlock <= pcNode)
#endif
    {
        *(void**)pBlock = mFree;
        --mUsed;
        mFree = pBlock;
        PblAsanPoison(pBlock, mSize);
        return;
    }

    while (*(char**)pcNode != 0 && pcBlock > *(char**)pcNode)
    {
        pcNode = *(char**)pcNode;
    }
    *(void**)pBlock = *(void**)pcNode;
    *(void**)pcNode = pBlock;
    --mUsed;
    PblAsanPoison(pBlock, mSize);
}

// FUNCTION: SWBF2 0x006dc6a0
void MemoryPool::Reset()
{
    mCount = 0;
    mUsed = 0;
    mHeap = -1;
    mPool = 0;
    mFree = 0;
}

void MemoryPool::ClearStats()
{
    mPeak = 0;
    mGrown = 0;
    mFullReported = false;
    mFullReportedStrict = false;
}

// FUNCTION: SWBF2 0x006dc750
void MemoryPool::Setup(unsigned int uiCount, unsigned int uiSize,
                       unsigned int uiGrow)
{
    if (mPool == 0)
    {
        mCount = uiCount;
        mSize = uiSize;
        mGrow = uiGrow;
    }
}

// FUNCTION: SWBF2 0x006dc320
MemoryPool::~MemoryPool()
{
    GetMemoryPoolList()->Remove(&mNode);
    if (mPool != 0 && mPoolOwner)
    {
        RedFreeToHeap(__RedCurrHeap, mPool, 0);
    }
    Reset();
}

// FUNCTION: SWBF2 0x006dc4f0
void MemoryPool::CreateAll()
{
    for (PblList<MemoryPool>::Itor itor(*GetMemoryPoolList()); itor; ++itor)
    {
        if (itor->mPool == 0)
        {
            itor->Create(itor->mCount, itor->mSize, itor->mGrow);
        }
    }
}

// FUNCTION: SWBF2 0x006dc530
void MemoryPool::Destroy()
{
    if (mPool != 0 && mPoolOwner)
    {
        RedFreeToHeap(__RedCurrHeap, mPool, 0);
    }
    Reset();
}

// FUNCTION: SWBF2 0x006dc560
void MemoryPool::DestroyAll()
{
    for (PblList<MemoryPool>::Itor itor(*GetMemoryPoolList()); itor; ++itor)
    {
        if (itor->mPool != 0)
        {
            itor->Destroy();
        }
        itor->ClearStats();
    }
    sErrorOnExpansion = false;
    sErrorOnExpansionPostLoad = false;
}

// FUNCTION: SWBF2 0x006dc5a0
MemoryPool* MemoryPool::Find(char const* pcLabel)
{
    for (PblList<MemoryPool>::Itor itor(*GetMemoryPoolList()); itor; ++itor)
    {
        if (_strnicmp(itor->mLabel, pcLabel, sizeof(itor->mLabel) - 1) == 0)
        {
            return itor;
        }
    }
    return 0;
}

// GLOBAL: SWBF2 0x0099e7c9
bool MemoryPool::sErrorOnExpansionPostLoad;

// FUNCTION: SWBF2 0x006dc690
void MemoryPool::PoolErrorPostLoad()
{
    sErrorOnExpansionPostLoad = true;
}

// FUNCTION: SWBF2 0x006dc6d0
void MemoryPool::Set(MemoryPool const& rPool)
{
    mPoolOwner = false;
    mSize = rPool.mSize;
    mCount = rPool.mCount;
    mGrow = rPool.mGrow;
    mUsed = rPool.mUsed;
    mHeap = rPool.mHeap;
    mPool = rPool.mPool;
    mFree = rPool.mFree;
    mPeak = rPool.mPeak;
    mGrown = rPool.mGrown;
    mFullReported = rPool.mFullReported;
    mFullReportedStrict = rPool.mFullReportedStrict;
}

// FUNCTION: SWBF2 0x006dc710
void MemoryPool::SetCount(unsigned int uiCount)
{
    if (mPool == 0)
    {
        mCount = uiCount;
    }
}

// FUNCTION: SWBF2 0x006dc730
void MemoryPool::SetSize(unsigned int uiSize)
{
    if (mPool == 0)
    {
        mSize = uiSize;
    }
}
