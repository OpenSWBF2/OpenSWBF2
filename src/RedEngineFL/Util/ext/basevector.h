#pragma once

#include "basearray.h"
#include "types.h"

namespace util
{

struct AssertBounds_
{
};
template <class TYPE, class ARRAY, class BOUNDS>
class IterArray : public ARRAY
{
public:
    // TEMPLATE: SWBF2 0x006afea0
    // util::IterArray<util::TKeyPair_<RenderItem *,unsigned int>,util::BaseArray<util::TKeyPair_<RenderItem *,unsigned int>,util::TNewArrayAlloc<util::TKeyPair_<RenderItem *,unsigned int> > >,util::AssertBounds_>::~IterArray<util::TKeyPair_<RenderItem *,unsigned int>,util::BaseArray<util::TKeyPair_<RenderItem *,unsigned int>,util::TNewArrayAlloc<util::TKeyPair_<RenderItem *,unsigned int> > >,util::AssertBounds_>
    void Clear()
    {
        this->Reserve_(0, true);
        this->_numElements = 0;
    }

    TYPE& operator[](int i) { return this->_elements[i]; }
    const TYPE& operator[](int i) const { return this->_elements[i]; }

    operator const TYPE*() const { return this->_elements; }
    TYPE* Begin() { return this->_elements; }

    TYPE* End() { return this->_elements + this->_numElements; }
};

template <class TYPE, class ARRAY = BaseArray<TYPE> >
class Vector : public IterArray<TYPE, ARRAY, AssertBounds_>
{
public:
    Vector() {}

    // TEMPLATE: SWBF2 0x0052b020
    // util::Vector<unsigned short,util::BaseArray<unsigned short,util::TypedArrayAllocator<unsigned short,util::NewAllocator> > >::~Vector<unsigned short,util::BaseArray<unsigned short,util::TypedArrayAllocator<unsigned short,util::NewAllocator> > >

    // TEMPLATE: SWBF2 0x0052b050
    // util::Vector<unsigned int,util::BaseArray<unsigned int,util::TypedArrayAllocator<unsigned int,util::NewAllocator> > >::~Vector<unsigned int,util::BaseArray<unsigned int,util::TypedArrayAllocator<unsigned int,util::NewAllocator> > >

    // TEMPLATE: SWBF2 0x006ded90
    // util::Vector<pvs::Sector *,util::BaseArray<pvs::Sector *,util::TNewArrayAlloc<pvs::Sector *> > >::~Vector<pvs::Sector *,util::BaseArray<pvs::Sector *,util::TNewArrayAlloc<pvs::Sector *> > >

    // TEMPLATE: SWBF2 0x006ded60
    // util::Vector<pvs::Portal *,util::BaseArray<pvs::Portal *,util::TNewArrayAlloc<pvs::Portal *> > >::~Vector<pvs::Portal *,util::BaseArray<pvs::Portal *,util::TNewArrayAlloc<pvs::Portal *> > >

    // TEMPLATE: SWBF2 0x006ded30
    // util::Vector<pvs::IDisplayable *,util::BaseArray<pvs::IDisplayable *,util::TNewArrayAlloc<pvs::IDisplayable *> > >::~Vector<pvs::IDisplayable *,util::BaseArray<pvs::IDisplayable *,util::TNewArrayAlloc<pvs::IDisplayable *> > >

    // TEMPLATE: SWBF2 0x006dedc0
    // util::Vector<void *,util::BaseArray<void *,util::TNewArrayAlloc<void *> > >::~Vector<void *,util::BaseArray<void *,util::TNewArrayAlloc<void *> > >

    // TEMPLATE: SWBF2 0x00715c60
    // util::Vector<dx::NVStateID,util::BaseArray<dx::NVStateID,util::TNewArrayAlloc<dx::NVStateID> > >::~Vector<dx::NVStateID,util::BaseArray<dx::NVStateID,util::TNewArrayAlloc<dx::NVStateID> > >


    Vector(Vector const& rOther)
    {
        this->_elements = 0;
        this->_numElements = 0;
        this->_size = 0;

        if (this->Reserve_(rOther._numElements, true))
        {
            this->_numElements = rOther._numElements;
            for (int i = 0; i < this->_numElements; i++)
            {
                this->_elements[i] = rOther._elements[i];
            }
        }
    }

    TYPE* FindFirst(TYPE const& rElem)
    {
        for (int i = 0; i < this->_numElements; i++)
        {
            if (rElem == this->_elements[i])
            {
                return &this->_elements[i];
            }
        }
        return this->End();
    }

    bool FindRemoveLazy(TYPE const& rElem)
    {
        TYPE* pSlot = FindFirst(rElem);
        if (pSlot == this->End())
        {
            return false;
        }
        *pSlot = 0;
        return true;
    }

    bool FindRemove(TYPE const& rElem)
    {
        TYPE* pSlot = FindFirst(rElem);
        if (pSlot == this->End())
        {
            return false;
        }
        *pSlot = this->End()[-1];
        this->_numElements--;
        return true;
    }

    bool Add(TYPE const& rElem)
    {
        if (this->_numElements >= this->_size && !GrowArray(8))
        {
            return false;
        }

        (*this)[this->_numElements++] = rElem;
        return true;
    }

    bool SetNumElements(int iNum)
    {
        if (iNum > this->_size && !GrowArray(iNum))
        {
            return false;
        }
        this->_numElements = iNum;
        return true;
    }

    bool SetSize(int iSize) { return SetSize_(iSize, true); }

    bool SetSizeGrow(int iSize) { return SetSize_(iSize, false); }

    void SetAll(const TYPE& value)
    {
        for (int i = 0; i < this->_numElements; i++)
        {
            this->_elements[i] = value;
        }
    }

    // TEMPLATE: SWBF2 0x0052b0d0
    // util::Vector<unsigned short,util::BaseArray<unsigned short,util::TypedArrayAllocator<unsigned short,util::NewAllocator> > >::SetSize_
    bool SetSize_(int iSize, bool bForce)
    {
        if (iSize == 0 || this->_size == 0)
        {
            if (!this->Reserve_(iSize, bForce))
            {
                return false;
            }
        }
        else if (bForce && iSize < this->_size / 2 - 1)
        {
            if (!this->Reserve_(1 << Int_::ilog(iSize), bForce))
            {
                return false;
            }
        }
        else if (iSize > this->_size)
        {
            const int iMinGranule = 4;
            int iGranule = Max_(1 << (Int_::ilog(iSize) - 2), iMinGranule);

            int iRounded = iSize;
            if (iSize % iGranule != 0)
            {
                iRounded = (iSize / iGranule + 1) * iGranule;
            }

            if (!this->Reserve_(iRounded, bForce))
            {
                return false;
            }
        }

        this->_numElements = iSize;
        return true;
    }

private:
    bool GrowArray(int iSize)
    {
        const int iNewSize = Max_(this->_size * 2, iSize);
        return this->Reserve_(iNewSize, true);
    }

};

}

// TEMPLATE: SWBF2 0x0062b810
// util::Vector<float,util::BaseArray<float,util::TNewArrayAlloc<float> > >::SetSize_
