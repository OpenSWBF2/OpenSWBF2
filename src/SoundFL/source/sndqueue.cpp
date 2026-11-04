#include "sndqueue.h"

namespace Snd
{

// FUNCTION: SWBF2 0x00738930
QueueBase::QueueBase()
{
    mFull = false;
    mHead = 0;
    mTail = 0;
}

// FUNCTION: SWBF2 0x00738940
QueueBase::~QueueBase()
{
}

// FUNCTION: SWBF2 0x00738a20
void* QueueBase::Get(unsigned int uiElementSize, void* const pElements,
                      unsigned int uiCapacity)
{
    if (IsEmpty())
    {
        return 0;
    }

    void* const pEntry = (unsigned char*)pElements + mTail * uiElementSize;
    if (++mTail == uiCapacity)
    {
        mTail = 0;
    }
    mFull = false;
    return pEntry;
}

// FUNCTION: SWBF2 0x00738a70
unsigned short QueueBase::GetNumItems(unsigned short usCapacity) const
{
    if (mFull)
    {
        return usCapacity;
    }
    return mHead >= mTail ? mHead - mTail : usCapacity + mHead - mTail;
}

// FUNCTION: SWBF2 0x007389a0
void QueueBase::Empty(unsigned int uiElementSize, void* const pElements,
                      unsigned short usCapacity,
                      bool(__cdecl* const pfnCallback)(void*, void*), void* const pData)
{
    if (!IsEmpty())
    {
        if (pfnCallback)
        {
            bool bContinue = true;
            while ((mFull || mTail != mHead) && bContinue)
            {
                bContinue = pfnCallback((unsigned char*)pElements + mTail * uiElementSize, pData);
                if (++mTail == usCapacity)
                {
                    mTail = 0;
                }
                mFull = false;
            }
        }
        else
        {
            mHead = 0;
            mTail = 0;
            mFull = false;
        }
    }
}

// FUNCTION: SWBF2 0x00738ad0
bool QueueBase::IsFull() const
{
    return mFull;
}

// FUNCTION: SWBF2 0x00738ab0
bool QueueBase::IsEmpty() const
{
    return mHead == mTail && !mFull;
}

// FUNCTION: SWBF2 0x00738ae0
const void* QueueBase::Peek(unsigned int uiElementSize,
                            const void* const pElements,
                            unsigned short usCapacity,
                            unsigned short usIndex) const
{
    unsigned int uiSlot = (unsigned int)mTail + (unsigned int)usIndex;

    if (uiSlot >= (unsigned int)usCapacity)
    {
        uiSlot -= (unsigned int)usCapacity;
    }

    return (const unsigned char*)pElements + uiSlot * uiElementSize;

}

}

