#pragma once

#include "sndallocator.h"
#include "sndallocbitmap.h"
#include "PebbleFL/Common/PblListSingle.h"

namespace Snd
{

class FreeList
{
public:

    class Block : public Allocator<Block>
    {
    public:

        Block(void* pAllocAddress)
        {
            mAllocAddress = pAllocAddress;
        }

        // FUNCTION: SWBF2 0x00738720
        void* AllocEntry(unsigned int uiEntrySize, unsigned int uiEntriesPerBlock)
        {
            void* const pEntries = (char*)this - uiEntriesPerBlock * uiEntrySize;
            void* pEntry = 0;
            AllocBitmap::Handle handle;
            if (mBitmap.Alloc(1, handle))
            {
                pEntry = (char*)pEntries + handle.GetAllocatedEntryIndex() * uiEntrySize;
                handle.Reset();
            }
            return pEntry;
        }

        void FreeEntry(void* pEntry, unsigned int uiEntrySize, unsigned int uiEntriesPerBlock)
        {
            void* const pEntries = (char*)this - uiEntriesPerBlock * uiEntrySize;
            const unsigned int uiEntryIndex =
                (unsigned int)((char*)pEntry - (char*)pEntries) / uiEntrySize;
            mBitmap.Reserve(uiEntryIndex, 1, false);
        }

        bool IsValid(void* pEntry, unsigned int uiEntrySize, unsigned int uiEntriesPerBlock)
        {
            void* const pEntries = (char*)this - uiEntriesPerBlock * uiEntrySize;
            return pEntry >= pEntries &&
                pEntry < (char*)pEntries + uiEntriesPerBlock * uiEntrySize;
        }

        const AllocBitmap& GetAllocator() const { return mBitmap; }
        void* GetAllocationAddress() const { return mAllocAddress; }

        PblListSingle::Node mNode;
        AllocBitmap mBitmap;
        void* mAllocAddress;
    };

    FreeList(unsigned int uiEntrySize, unsigned int uiAlignment,
             bool bPurgeOnFree, unsigned int uiEntriesPerBlock,
             void* (__cdecl* pAllocFunc)(unsigned int) = Globals::smAlloc,
             void (__cdecl* pFreeFunc)(void*) = Globals::smFree)
        : mEntrySize(uiEntrySize),
          mAlignment(uiAlignment),
          mEntriesPerBlock(uiEntriesPerBlock),
          mAllocFunc(pAllocFunc),
          mFreeFunc(pFreeFunc),
          mFlags(bPurgeOnFree ? flags_purgeOnFree : 0)
    {
    }

    enum Flags
    {
        flags_purgeOnFree = 1,
        flags_locked = 2
    };

    void* Alloc();
    bool AllocBlocks(unsigned int uiNumBlocks,
                     Block** ppFirstBlock);
    void Free(void* p);
    void PurgeBlocks();

    void CheckAllocator()
    {
        if (mAllocFunc == 0 || mFreeFunc == 0)
        {
            mAllocFunc = Globals::smAlloc;
            mFreeFunc = Globals::smFree;
        }
    }

    unsigned int mEntrySize;
    unsigned int mAlignment;
    unsigned int mEntriesPerBlock;
    PblListSingle mBlockList;
    void* (__cdecl* mAllocFunc)(unsigned int);
    void (__cdecl* mFreeFunc)(void*);
    unsigned char mFlags;
};

 
 

}
