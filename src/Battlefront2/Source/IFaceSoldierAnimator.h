#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>
#include "PebbleFL/Common/PblBitVector.h"
#include "RedEngineFL/Interface/Red3DModelAnimator.h"

class ZephyrAnim;

// VTABLE: SWBF2 0x007a38f0
class IFaceSoldierAnimator : public Red3DModelAnimator
{
public:

    IFaceSoldierAnimator(const Red3DModelAnimator* pAnimator);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~IFaceSoldierAnimator() {}
    // SYNTHETIC: SWBF2 0x0056a1d0
    // IFaceSoldierAnimator::`scalar deleting destructor'
    virtual void Open();
    virtual void Close();
    virtual RedPose* UpdatePose();
    virtual void Update(float fElapsed);

    // FUNCTION: SWBF2 0x0056a180
    void SetUpperLowerAnimations(unsigned int uUpperAnimHash, unsigned int uLowerAnimHash,
                                 float fTransitionTime)
    {
        Close();
        mAnimHash = uUpperAnimHash;
        mLowerAnimHash = uLowerAnimHash;
        mQueuedAnim = 0;
        mQueuedLowerAnim = 0;
        mAnimTransitionTimeTotal = fTransitionTime;
        mAnimTransitionTime = 0.0f;
        Open();
    }
    void QueueUpperLowerAnimations(unsigned int uUpperAnimHash, unsigned int uLowerAnimHash);

    static PblHash rttiHashIFaceSoldierAnimator;
    static const char rttiNameIFaceSoldierAnimator[];

    unsigned int mLowerAnimHash;
    unsigned int mQueuedLowerAnim;
    ZephyrAnim* mZephyrAnimationLower;
    PblBitVector<32> mUpperBodyAnimMask;
    PblBitVector<32> mLowerBodyAnimMask;
    ZephyrPoseDyn<32> mZephyrPoseDynLower;
};
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x0056a130
inline bool IFaceSoldierAnimator::IsRtti(unsigned int uRtti) const
{
    if (rttiHashIFaceSoldierAnimator == uRtti)
    {
        return true;
    }
    return Red3DModelAnimator::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0056a160
inline unsigned int IFaceSoldierAnimator::GetDerivedRtti() const
{
    return rttiHashIFaceSoldierAnimator;
}

// FUNCTION: SWBF2 0x0056a170
inline const char* IFaceSoldierAnimator::GetDerivedRttiName() const
{
    return rttiNameIFaceSoldierAnimator;
}
