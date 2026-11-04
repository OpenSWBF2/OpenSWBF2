#pragma once

namespace Snd
{

class QueueBase
{
public:
    QueueBase();
    ~QueueBase();

    void* Get(unsigned int uiElementSize, void* const pElements, unsigned int uiCapacity);

    bool IsFull() const;

    bool IsEmpty() const;

    const void* Peek(unsigned int uiElementSize, const void* const pElements,
                     unsigned short usCapacity,
                     unsigned short usIndex) const;

    void Empty(unsigned int uiElementSize, void* const pElements, unsigned short usCapacity,
               bool(__cdecl* const pfnCallback)(void*, void*), void* const pData);
    unsigned short GetNumItems(unsigned short usCapacity) const;
    void* Add(unsigned int uiElementSize, void* const pElements,
              unsigned short usCapacity);

    unsigned short mHead;
    unsigned short mTail;
    bool mFull : 1;
};
 

template <class T, int N>
class Queue : public QueueBase
{
public:

    Queue()
    {
    }

    // TEMPLATE: SWBF2 0x0073d940
    // Snd::Queue<Snd::StreamPacket *,4>::~Queue<Snd::StreamPacket *,4>
    ~Queue()
    {
    }
    // TEMPLATE: SWBF2 0x0073da80
    // Snd::Queue<Snd::StreamPacket *,4>::Add
    bool Add(const T& rElement)
    {
        if (IsFull())
        {
            return false;
        }

        mElements[mHead] = rElement;
        ++mHead;

        if (mHead == N)
        {
            mHead = 0;
        }

        mFull = (mHead == mTail);
        return true;
    }

    const T* Peek(unsigned short usIndex) const
    {
        return (const T*)QueueBase::Peek((unsigned int)sizeof(T), mElements,
                                         (unsigned short)N, usIndex);
    }

    void Get(T& rElement)
    {
        rElement = mElements[mTail];
        ++mTail;

        if (mTail == N)
        {
            mTail = 0;
        }

        mFull = false;
    }

    void Empty(bool(__cdecl* const pfnCallback)(void*, void*), void* const pData)
    {
        QueueBase::Empty((unsigned int)sizeof(T), mElements, (unsigned short)N, pfnCallback,
                         pData);
    }
    unsigned short GetNumItems() const { return QueueBase::GetNumItems((unsigned short)N); }

    T mElements[N];
};

class QueueLockBase
{
public:

    QueueLockBase();

    ~QueueLockBase();

    void Empty(QueueBase* const pQueue, unsigned int uiElementSize, void* const pElements,
               unsigned short usCapacity, bool(__cdecl* const pfnCallback)(void*, void*),
               void* const pData);
    void* Get(QueueBase* const pQueue, unsigned int uiElementSize,
              void* const pElements, unsigned short usCapacity);
    void Unlock(QueueBase* const pQueue, const void* const pElement,
                unsigned int uiElementSize, void* const pElements,
                unsigned short usCapacity);

    unsigned short mLock;
};
 

template <class T, int N>
class QueueLock : public Queue<T, N>, public QueueLockBase
{
public:

    void Empty(bool(__cdecl* const pfnCallback)(void*, void*), void* const pData)
    {
        QueueLockBase::Empty(this, (unsigned int)sizeof(T), this->mElements, (unsigned short)N,
                             pfnCallback, pData);
    }

    T* Lock();

    void Unlock(const T* const pElement);
};

template <class T, int N>
T* QueueLock<T, N>::Lock()
{
    return (T*)QueueLockBase::Get(this, (unsigned int)sizeof(T), this->mElements,
                                  (unsigned short)N);
}

template <class T, int N>
void QueueLock<T, N>::Unlock(const T* const pElement)
{
    QueueLockBase::Unlock(this, pElement, (unsigned int)sizeof(T), this->mElements,
                          (unsigned short)N);
}

// TEMPLATE: SWBF2 0x006f4f90
// Snd::QueueCircular<RedMovie::Properties const *,11>::~QueueCircular<RedMovie::Properties const *,11>
// TEMPLATE: SWBF2 0x006f4fa0
// Snd::QueueCircular<RedMovie::SegmentInfo const *,11>::~QueueCircular<RedMovie::SegmentInfo const *,11>
template <class T, int N>
class QueueCircular : public Queue<T, N>
{
public:

    bool Add(const T& rElement)
    {
        if (Queue<T, N>::Add(rElement))
        {
            return false;
        }

        this->mElements[this->mHead] = rElement;
        ++this->mHead;
        if (this->mHead == N)
        {
            this->mHead = 0;
        }
        this->mTail = this->mHead;
        return true;
    }
};

}
