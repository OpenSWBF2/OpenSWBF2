#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblRtti.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "PebbleFL/Common/PblVector.h"

class Controllable;

namespace AI
{

    enum TargetBubbleIncreaseType
    {
        TBI_ROLL = 0,
        TBI_SPRINT = 1,
        TBI_JUMP = 2,
        TBI_JETJUMP = 3,
        TBI_FLYERTRICK = 4
    };
}

// VTABLE: SWBF2 0x00798cf8
class Controller
{
public:
    Controller(Controllable* pOwner);

    // FUNCTION: SWBF2 0x00485100
    // Controller::IsRtti
    // FUNCTION: SWBF2 0x00485120
    // Controller::GetDerivedRtti
    // FUNCTION: SWBF2 0x00485130
    // Controller::GetDerivedRttiName
    PBL_DECLARE_ROOT_RTTI(Controller)
    // SYNTHETIC: SWBF2 0x00485140
    // Controller::~Controller
    // SYNTHETIC: SWBF2 0x00485160
    // Controller::`scalar deleting destructor'
    virtual ~Controller() {}
    virtual void KillController();


    PblVector3 GetTargetBubbleCircle() const;
    void IncreaseTargetBubbleSize(float fIncrease);
    void IncreaseTargetBubbleSize(AI::TargetBubbleIncreaseType eType);

    void UpdateTargetBubbleSize(float dt);
    void UpdateTotalVisFactor();

    bool GetCrouchedForVis() const;
    float GetFoliageVisFactor() const;
    float GetInvisibilityVisFactor() const;
    float GetRegionVisFactor() const;
    void UpdateFreeSpace();

    void SetRegionVisFactor(float fCrouch, float fStand);

    void SetFoliageVisFactor(float fCrouch, float fStand);

    void SetForceAudible(bool bForce) { mForceAudible = bForce; }

    const Controllable* GetMe() const { return mOwner; }
    Controllable* GetMe() { return mOwner; }

    bool IsInCrowd() const;

protected:
    Controller();
    float _GetTargetBubbleSize() const;

public:
    Controllable* mOwner;
    PblVector3 mTargetBubbleLastVel;
    float mTargetBubbleSize;
    float mAvailableBubbleCharge;
    float mTargetBubbleSize_Cached;
    PblVector3 mTargetBubbleAxis;
    PblQuaternion mTargetBubbleRotation;
    PblVector3 mTargetBubbleOffset;
    float mOpenRadius;
    float mOpenRadiusTime;
    float mFoliageVisCrouch;
    float mFoliageVisStand;
    float mRegionVisCrouch;
    float mRegionVisStand;
    float mRegionVisTime;
    float mTotalVisFactor;
    bool mForceAudible;
    float mCrowdTestTimer;
    int mCrowdEnemies;
    int mCrowdAllies;
};

 
 
 

