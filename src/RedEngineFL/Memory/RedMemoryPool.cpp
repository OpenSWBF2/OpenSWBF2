#include <stdlib.h>
#include <string.h>

#include "RedMemoryPool.h"
#include "RedMemory.h"
#include "RedMemoryReport.h"
#include "RedScriptLimits.h"
#include "PebbleFL/Common/PblCrtHeap.h"
#include "RedEngineFL/Util/RedLog.h"

#ifdef OPENSWBF2_TEST
static PBL_ASAN_NO_SANITIZE void* PerturbShuffleChunk(void* pChunk, unsigned int uiCount, unsigned int uiSize)
{
    if (uiCount < 2 || (RedPerturbModes() & kRedPerturbPool) == 0)
    {
        return pChunk;
    }
    unsigned int* puOrder = (unsigned int*)PblCrtAlloc(uiCount * sizeof(unsigned int));
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
            PblCrtFree(puOrder);
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
    PblCrtFree(puOrder);
    return pHead;
}

static PBL_ASAN_NO_SANITIZE bool PerturbFreeInsert(void** ppFree, void* pBlock)
{
    unsigned int uiDraw = 0;
    if ((RedPerturbModes() & kRedPerturbPool) == 0 || !RedPerturbDraw(kRedPerturbPool, &uiDraw))
    {
        return false;
    }
    unsigned int uDepth = uiDraw % 4;
    if (uDepth == 0)
    {
        return false;
    }
    void** ppLink = ppFree;
    for (unsigned int i = 0; i < uDepth && *ppLink != 0; ++i)
    {
        ppLink = (void**)*ppLink;
    }
    *(void**)pBlock = *ppLink;
    *ppLink = pBlock;
    return true;
}
#endif

MemoryPool::GrowthChunk* MemoryPool::sChunkRecords;

static void ForgetChunkRecord(MemoryPool::GrowthChunk* pRecord)
{
    for (MemoryPool::GrowthChunk** ppNode = &MemoryPool::sChunkRecords; *ppNode != 0; ppNode = &(*ppNode)->pAllNext)
    {
        if (*ppNode == pRecord)
        {
            *ppNode = pRecord->pAllNext;
            return;
        }
    }
}

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
    mChunks = 0;
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
    mFirstCount = mCount;

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

        if (mPool != 0)
        {
            GrowthChunk* pRecord = (GrowthChunk*)PblCrtAlloc(sizeof(GrowthChunk));
            if (pRecord == 0)
            {
                RedFreeToHeap(__RedCurrHeap, mFree, 0);
                mFree = 0;
                return 0;
            }
            pRecord->pNext = mChunks;
            pRecord->pChunk = mFree;
            pRecord->uCount = mGrow;
            pRecord->pAllNext = sChunkRecords;
            sChunkRecords = pRecord;
            mChunks = pRecord;
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
            mFirstCount = mGrow;
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

PBL_ASAN_NO_SANITIZE void MemoryPool::SortFreeList()
{
    for (unsigned int uWidth = 1; ; uWidth *= 2)
    {
        char* pcTail = 0;
        char* pcRest = (char*)mFree;
        char* pcHead = 0;
        unsigned int uMerges = 0;
        while (pcRest != 0)
        {
            ++uMerges;
            char* pcA = pcRest;
            char* pcB = pcRest;
            unsigned int uA = 0;
            while (uA < uWidth && pcB != 0)
            {
                ++uA;
                pcB = *(char**)pcB;
            }
            unsigned int uB = uWidth;
            while (uA > 0 || (uB > 0 && pcB != 0))
            {
                char* pcTake;
                if (uA == 0 || (uB > 0 && pcB != 0 && pcB < pcA))
                {
                    pcTake = pcB;
                    pcB = *(char**)pcB;
                    --uB;
                }
                else
                {
                    pcTake = pcA;
                    pcA = *(char**)pcA;
                    --uA;
                }
                if (pcTail != 0)
                {
                    *(char**)pcTail = pcTake;
                }
                else
                {
                    pcHead = pcTake;
                }
                pcTail = pcTake;
            }
            pcRest = pcB;
        }
        if (pcTail != 0)
        {
            *(char**)pcTail = 0;
        }
        mFree = pcHead;
        if (uMerges <= 1)
        {
            return;
        }
    }
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
    bool bSorted = false;
    for (int iPass = 0; iPass < 2 && !bSorted; ++iPass)
    {
        if (iPass != 0)
        {
            SortFreeList();
        }
        pcBest = 0;
        char* pcRun = 0;
        char* pcLast = 0;
        unsigned int uiRun = 0;
        bSorted = true;
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
            if (uiRun == uiCount && pcBest == 0)
            {
                pcBest = pcRun;
            }
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

#ifdef OPENSWBF2_TEST
    if (PerturbFreeInsert(&mFree, pBlock))
    {
        --mUsed;
        PblAsanPoison(pBlock, mSize);
        return;
    }
#endif
    *(void**)pBlock = mFree;
    --mUsed;
    mFree = pBlock;
    PblAsanPoison(pBlock, mSize);
}

// FUNCTION: SWBF2 0x006dc6a0
void MemoryPool::Reset()
{
    if (mPoolOwner)
    {
        GrowthChunk* pNode = mChunks;
        while (pNode != 0)
        {
            GrowthChunk* pNext = pNode->pNext;
            ForgetChunkRecord(pNode);
            PblCrtFree(pNode);
            pNode = pNext;
        }
    }
    mChunks = 0;
    mFirstCount = 0;
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
        FreeChunks();
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
        FreeChunks();
    }
    Reset();
}

void MemoryPool::FreeChunks()
{
    for (GrowthChunk* pNode = mChunks; pNode != 0; pNode = pNode->pNext)
    {
        RedFreeToHeap(__RedCurrHeap, pNode->pChunk, 0);
    }
    RedFreeToHeap(__RedCurrHeap, mPool, 0);
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
    while (sChunkRecords != 0)
    {
        GrowthChunk* pRecord = sChunkRecords;
        sChunkRecords = pRecord->pAllNext;
        PblCrtFree(pRecord);
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
    mChunks = rPool.mChunks;
    mFirstCount = rPool.mFirstCount;
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
