#include "ListAllocator.h"

namespace util
{

// VTABLE: SWBF2 0x007839a0
class ListAllocatorImpl : public ListAllocator
{
public:
    // FUNCTION: SWBF2 0x006e9210
    ListAllocatorImpl(unsigned int uBaseMem, unsigned int uMaxMem, IAllocator* pPages)
        : freelist_(0)
    {
        baseAlloc_ = pPages ? pPages : &valloc_;
        pageSize_ = 0x2000;
        basemem_ = uBaseMem;
        maxmem_ = uMaxMem;
        usedmem_ = 0;
        freemem_ = 0;
        pages_.Reserve(16);
        roundsize_ = 32;
        ExternalAlloc_(uBaseMem);
    }
    // FUNCTION: SWBF2 0x006e9320
    virtual ~ListAllocatorImpl()
    {
        if (baseAlloc_)
        {
            for (int i = 0; i < pages_.GetSize(); i++)
            {
#if defined(__SANITIZE_ADDRESS__)
                PblAsanUnpoison(pages_[i].data_, pages_[i].size_);
#endif
                baseAlloc_->Deallocate(pages_[i]);
            }
        }
    }

    // FUNCTION: SWBF2 0x006e94a0
    PBL_ASAN_NO_SANITIZE virtual void Allocate(Buffer& rBuf)
    {
#if defined(__SANITIZE_ADDRESS__)
        unsigned int uiRequest = rBuf.size_;
#endif
        if (rBuf.size_ % roundsize_)
        {
            rBuf.size_ = (rBuf.size_ / roundsize_ + 1) * roundsize_;
        }

        DLNode<Buffer>* pNode = FindFreeBlock(rBuf);
        if (!pNode)
        {
            if (baseAlloc_)
            {
                int iSize = rBuf.size_;
                if (ExternalAlloc_(Max_(pageSize_, iSize + 2 * (int)sizeof(DLNode<Buffer>))))
                {
                    Allocate(rBuf);
#if defined(__SANITIZE_ADDRESS__)
                    if (rBuf.data_)
                    {
                        PblAsanPoison(rBuf.data_, rBuf.size_);
                        PblAsanUnpoison(rBuf.data_, uiRequest);
                    }
#endif
                }
                else
                {
                    rBuf.data_ = 0;
                }
            }
            return;
        }

        int iRemain = pNode->elem_.size_ - rBuf.size_;
        if (iRemain > 32)
        {
            freemem_ -= pNode->elem_.size_;
            rBuf.data_ = pNode->elem_.data_;
            pNode->elem_.size_ = rBuf.size_;

            DLNode<Buffer>* pRest = (DLNode<Buffer>*)((char*)rBuf.data_ + rBuf.size_);
            pRest->elem_.data_ = pRest + 1;
            pRest->elem_.size_ = iRemain - sizeof(DLNode<Buffer>);
            pRest->next_ = 0;
            pRest->prev_ = 0;

            pRest->prev_ = pNode;
            pRest->next_ = pNode->next_;
            pNode->next_ = pRest;
            if (pRest->next_)
            {
                pRest->next_->prev_ = pRest;
            }
            else
            {
                freelist_.last_ = pRest;
            }
            freelist_.size_++;

            freemem_ += pRest->elem_.size_;
            freelist_.Remove_(pNode);
        }
        else
        {
            rBuf = pNode->elem_;
            freelist_.Remove_(pNode);
            freemem_ -= rBuf.size_;
        }
#if defined(__SANITIZE_ADDRESS__)
        PblAsanPoison(pNode, sizeof(DLNode<Buffer>) + pNode->elem_.size_);
        PblAsanUnpoison(rBuf.data_, uiRequest);
#endif
    }
    // FUNCTION: SWBF2 0x006e9670
    PBL_ASAN_NO_SANITIZE virtual void Deallocate(Buffer& rBuf)
    {
        if (!rBuf.data_)
        {
            return;
        }

        DLNode<Buffer>* pNode = Convert_(rBuf);
        pNode->next_ = 0;
        pNode->prev_ = 0;

        DLNode<Buffer>* pLast = 0;
        DLNode<Buffer>* pAfter = 0;
        const unsigned long long uKey = PageOrder_(rBuf.data_);
        for (DLNode<Buffer>* p = freelist_.first_; p; p = p->next_)
        {
            pLast = p;
            if (PageOrder_(p->elem_.data_) > uKey)
            {
                pAfter = p;
                break;
            }
        }

        if (freelist_.size_ > 0 && !pAfter)
        {

            pNode->prev_ = pLast;
            pNode->next_ = 0;
            if (pLast)
            {
                pLast->next_ = pNode;
            }
            else
            {
                freelist_.first_ = pNode;
            }
            freelist_.last_ = pNode;
        }
        else
        {

            pNode->prev_ = pAfter ? pAfter->prev_ : 0;
            pNode->next_ = pAfter;
            if (pNode->prev_)
            {
                pNode->prev_->next_ = pNode;
            }
            else
            {
                freelist_.first_ = pNode;
            }
            if (pAfter)
            {
                pAfter->prev_ = pNode;
            }
            else
            {
                freelist_.last_ = pNode;
            }
        }
        freelist_.size_++;
        freemem_ += rBuf.size_;
#if defined(__SANITIZE_ADDRESS__)
        PblAsanPoison(pNode, sizeof(DLNode<Buffer>) + rBuf.size_);
#endif

        DLNode<Buffer>* pPrev = pNode->prev_;
        DLNode<Buffer>* pNext = pNode->next_;
        if (pPrev && (!pNext || pPrev->elem_.size_ < pNext->elem_.size_))
        {
            MergeBlocks_(pPrev, pNode);
        }
        else if (pNext)
        {
            MergeBlocks_(pNode, pNext);
        }
    }
    // FUNCTION: SWBF2 0x006e9a60
    virtual void SetMaxMem(int iMax)
    {
        maxmem_ = iMax;
    }
    // FUNCTION: SWBF2 0x006e9a70
    virtual void UnitTest_()
    {
    }

    PBL_ASAN_NO_SANITIZE DLNode<Buffer>* Convert_(Buffer& rBuf)
    {
        DLNode<Buffer>* pNode = (DLNode<Buffer>*)rBuf.data_ - 1;
        rBuf = pNode->elem_;
        return pNode;
    }

    // FUNCTION: SWBF2 0x006e9760
    DLNode<Buffer>* ExternalAlloc_(unsigned int uSize)
    {
        if (usedmem_ + uSize <= maxmem_)
        {
            Buffer buf(0, uSize);
            baseAlloc_->Allocate(buf);
            if (buf.data_)
            {
                pages_.Add(buf);

                DLNode<Buffer>* pNode = (DLNode<Buffer>*)buf.data_;
                buf.size_ -= sizeof(DLNode<Buffer>);
                pNode->elem_.data_ = pNode + 1;
                pNode->elem_.size_ = buf.size_;
                pNode->next_ = 0;
                pNode->prev_ = 0;
                buf.data_ = pNode->elem_.data_;
                Deallocate(buf);

                usedmem_ += uSize;
                return pNode;
            }
        }
        return 0;
    }

    // FUNCTION: SWBF2 0x006e97f0
    PBL_ASAN_NO_SANITIZE DLNode<Buffer>* FindFreeBlock(const Buffer& rBuf)
    {
        for (DLNode<Buffer>* p = freelist_.first_; p; p = p->next_)
        {
            if (p->next_)
            {
                MergeBlocks_(p, p->next_);
            }
            if (p->elem_.size_ >= rBuf.size_ + sizeof(DLNode<Buffer>))
            {
                return p;
            }
        }
        return 0;
    }
    // FUNCTION: SWBF2 0x006e98a0
    PBL_ASAN_NO_SANITIZE bool MergeBlocks_(DLNode<Buffer>* pFirst, DLNode<Buffer>* pSecond)
    {
        if ((char*)pSecond->elem_.data_ - (char*)pFirst->elem_.data_ - pFirst->elem_.size_
            - sizeof(DLNode<Buffer>) != 0)
        {
            return false;
        }
        if ((PageOrder_(pFirst) >> 32) != (PageOrder_(pSecond) >> 32))
        {
            return false;
        }

        pFirst->elem_.size_ += pSecond->elem_.size_ + sizeof(DLNode<Buffer>);
        freemem_ += sizeof(DLNode<Buffer>);
        freelist_.Remove_(pSecond);
        return true;
    }

protected:
    unsigned long long PageOrder_(const void* p)
    {
        for (int i = 0; i < pages_.GetSize(); i++)
        {
            const char* pStart = (const char*)pages_[i].data_;
            if ((const char*)p >= pStart && (const char*)p < pStart + pages_[i].size_)
            {
                return ((unsigned long long)i << 32) | (unsigned int)((const char*)p - pStart);
            }
        }
        return ~0ull;
    }

    VNewAllocator valloc_;
    DummyNonAllocator dalloc_;
    IAllocator* baseAlloc_;
    int pageSize_;
    int roundsize_;
    unsigned int basemem_;
    unsigned int usedmem_;
    unsigned int maxmem_;
    int freemem_;
    List<Buffer, DLNode<Buffer> > freelist_;
    Vector<Buffer> pages_;
};







// SYNTHETIC: SWBF2 0x006e93c0
// util::ListAllocator::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x006e93f0
// util::ListAllocatorImpl::`scalar deleting destructor'

// TEMPLATE: SWBF2 0x006e9420
// util::Vector<util::Buffer,util::BaseArray<util::Buffer,util::TNewArrayAlloc<util::Buffer> > >::Add
template bool Vector<Buffer>::Add(const Buffer&);

// FUNCTION: SWBF2 0x006e9490
void DummyNonAllocator::Allocate(Buffer&)
{
}



// TEMPLATE: SWBF2 0x006e95a0
// util::TypedArrayAllocator<util::Buffer,util::NewAllocator>::Allocate_
template Buffer* TypedArrayAllocator<Buffer>::Allocate_(int&);

// FUNCTION: SWBF2 0x006e95f0
ListAllocator* ListAllocator::Create(unsigned int uBaseMem, unsigned int uMaxMem, IAllocator* pPages)
{
    return new ListAllocatorImpl(uBaseMem, uMaxMem, pPages);
}

// FUNCTION: SWBF2 0x006e9660
void DummyNonAllocator::Deallocate(Buffer&)
{
}









// TEMPLATE: SWBF2 0x006e9920
// util::List<util::Buffer,util::DLNode<util::Buffer> >::Remove_
template bool List<Buffer, DLNode<Buffer> >::Remove_(DLNode<Buffer>*);

// TEMPLATE: SWBF2 0x006e9980
// util::BaseArray<util::Buffer,util::TNewArrayAlloc<util::Buffer> >::Reserve_
template bool BaseArray<Buffer>::Reserve_(int, bool);





}
