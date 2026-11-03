#pragma once

#include <stddef.h>

namespace Snd
{

class AllocBitmap
{
public:

    AllocBitmap() : mBitmap(0), mBitmapSize(0), mSearchEntry(0) {}

    class Handle
    {
    public:

        Handle() { Reset(); }

        unsigned int GetAllocatedEntryIndex() const { return mStartEntry; }
        unsigned int GetNumAllocatedEntries() const { return mNumEntries; }

        void Reset()
        {
            mStartEntry = 0;
            mNumEntries = 0;
        }

        unsigned int mStartEntry;
        unsigned int mNumEntries;
    };

    bool Alloc(unsigned int uiNumEntries, Handle& handle);
    void Free(Handle& handle);
    bool Resize(unsigned int uiNumEntries, Handle& handle);

    unsigned int GetFreeEntries() const;

    void Initialize(unsigned char* pBitmap, unsigned int uiNumEntries);
    void Reserve(unsigned int uiStartEntry, unsigned int uiNumEntries, bool bReserve);

    unsigned char* mBitmap;
    unsigned int mBitmapSize;

    unsigned int mSearchEntry;
};

class MemoryBitmap : public AllocBitmap
{
public:

    MemoryBitmap();

    class MemoryHandle : public AllocBitmap::Handle
    {
    public:

        uintptr_t GetAddress(MemoryBitmap& bitmap) const
        {
            return mStartEntry * bitmap.GetEntrySize() + bitmap.GetStartAddress();
        }

        unsigned int GetSize(MemoryBitmap& bitmap) const
        {
            return mNumEntries ? mNumEntries * bitmap.GetEntrySize() : 0;
        }
    };

    bool Alloc(unsigned int uiSize, MemoryHandle& handle);
    bool Resize(unsigned int uiSize, MemoryHandle& handle);

    unsigned int GetEntrySize() const;
    uintptr_t GetStartAddress() const;

    void Initialize(uintptr_t uiStartAddress, unsigned int uiMemorySize,
                    unsigned int uiEntrySize, unsigned char* pBitmap);

    uintptr_t mStartAddress;
    unsigned int mMemorySize;
    unsigned int mEntrySize;
};

 
 
 
 

}
