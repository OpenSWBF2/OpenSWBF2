#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblRef.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x007ad36c
class ParticleEmitterInfoData : public PblRef
{
public:

    ParticleEmitterInfoData()
        : mNext(0), mPrev(0)
    {
    }

    static void* operator new(size_t size)
    {
        return sMemoryPool.Allocate((unsigned int)size);
    }

    static void operator delete(void* p)
    {
        sMemoryPool.Free(p);
    }

    // FUNCTION: SWBF2 0x0060b910
    static ParticleEmitterInfoData* Create()
    {
        if (sMemoryPool.IsFull())
        {
            return 0;
        }
        return new ParticleEmitterInfoData;
    }

    void LinkNext(ParticleEmitterInfoData* newNext)
    {
        if (mNext)
        {
            mNext->mPrev = newNext;
        }
        newNext->mNext = mNext;
        newNext->mPrev = this;
        mNext = newNext;
    }

    PblMatrix mMatrix;
    PblVector3 mVelocity;
    PblVector3 mInitialVelocity;
    float mStartTime;
    float mLifeTime;
    ParticleEmitterInfoData* mNext;
    ParticleEmitterInfoData* mPrev;

    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }
    static MemoryPool sMemoryPool;
};
 
 
 
 
 
