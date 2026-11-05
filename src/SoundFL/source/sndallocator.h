#pragma once

#include "sndglobals.h"

namespace Snd
{

template <class T>
class Allocator
{
public:
    enum AllocType
    {
        allocType_heap = 0,
        allocType_tempHeap = 1,
        allocType_custom = 2
    };

    Allocator() {}
    ~Allocator() {}

    AllocType GetAllocType() const { return (AllocType)mAllocType; }

    static void* operator new(size_t uiSize, void* pMemory, bool bTemp)
    {
        void* p;
        AllocType eType;
        if (pMemory)
        {
            p = pMemory;
            eType = allocType_custom;
        }
        else if (bTemp)
        {
            p = AllocTemp(uiSize);
            eType = allocType_tempHeap;
        }
        else
        {
            p = Alloc(uiSize);
            eType = allocType_heap;
        }
        if (p)
        {
            ((T*)p)->mAllocType = (unsigned char)eType;
        }
        return p;
    }

    static void operator delete(void* p, size_t  )
    {
        switch (((T*)p)->GetAllocType())
        {
        case allocType_heap:
            Free(p);
            break;
        case allocType_tempHeap:
            FreeTemp(p);
            break;
        default:
            break;
        }
    }

    static void operator delete(void* p, void* pMemory, bool bTemp)
    {
        (void)pMemory;
        (void)bTemp;
        if (!p)
        {
            return;
        }
        switch (((T*)p)->GetAllocType())
        {
        case allocType_heap:
            Free(p);
            break;
        case allocType_tempHeap:
            FreeTemp(p);
            break;
        default:
            break;
        }
    }

    unsigned char mAllocType;
};

}
