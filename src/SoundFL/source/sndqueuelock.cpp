#include "sndqueue.h"

namespace Snd
{

// FUNCTION: SWBF2 0x00745050
void QueueLockBase::Empty(QueueBase* const pQueue, unsigned int uiElementSize,
                          void* const pElements, unsigned short usCapacity,
                          bool(__cdecl* const pfnCallback)(void*, void*), void* const pData)
{
    pQueue->Empty(uiElementSize, pElements, usCapacity, pfnCallback, pData);
    mLock = pQueue->mTail;
}

// FUNCTION: SWBF2 0x00745080
void* QueueLockBase::Get(QueueBase* const pQueue, unsigned int uiElementSize,
                         void* const pElements, unsigned short usCapacity)
{
    if (mLock == pQueue->mHead)
    {
        return 0;
    }
    unsigned short i = mLock++;
    void* pResult = (char*)pElements + i * uiElementSize;
    if (mLock == usCapacity)
    {
        mLock = 0;
    }
    return pResult;
}

// FUNCTION: SWBF2 0x007450c0
void QueueLockBase::Unlock(QueueBase* const pQueue, const void* const pElement,
                           unsigned int uiElementSize, void* const pElements,
                           unsigned short usCapacity)
{
    (void)pElement;
    (void)uiElementSize;
    (void)pElements;
    ++pQueue->mTail;
    if (pQueue->mTail == usCapacity)
    {
        pQueue->mTail = 0;
    }
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x00745030
QueueLockBase::QueueLockBase()
{
    mLock = 0;
}

// FUNCTION: SWBF2 0x00745040
QueueLockBase::~QueueLockBase()
{
}
}
