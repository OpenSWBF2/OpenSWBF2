#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "CmdInfo.h"
#include "CombatHelper.h"
#include "DefendHelper.h"
#include "PebbleFL/Common/PblHandle.h"

class FlagItem;
int GetFormationType(const UnitController* pCtrl);

// VTABLE: SWBF2 0x007af88c
class UnitCTFOffenseAgent : public UnitAgent
{
public:
    static PblHash rttiHashUnitCTFOffenseAgent;
    static const char rttiNameUnitCTFOffenseAgent[];

    UnitCTFOffenseAgent(UnitController* pCtrl, const FlagItem* pFlag, const PblVector3& vPos);
    // SYNTHETIC: SWBF2 0x00664630
    // UnitCTFOffenseAgent::`scalar deleting destructor'
    // SYNTHETIC: SWBF2 0x00664660
    // UnitCTFOffenseAgent::~UnitCTFOffenseAgent
    virtual void Kill();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual float GetUpdateRate() const;

    virtual const char* NFINAL_GetStateString(int iState) const;
    virtual int EventHandler(AI::EventInfo* pInfo);

    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

    bool IsFlagNeutral() const;
    bool IsFlagInMyHands() const;
    bool IsFlagInAlliedHandsButNotMine() const;
    bool IsFlagInEnemyHands() const;
    void FaceTargetIfClose(ControllableHeight eHeight);

    CmdInfo mCombatCmd;
    CombatHelper mCombatHelper;
    DefendHelper mDefendHelper;
    PblHandle<const GameObject> mFlag;
    PblVector3 mCapturePos;
    bool mPathing;
    float mPathTime;
    float mDefendCarrierOffset;
};

 
 
 
 
 

// FUNCTION: SWBF2 0x006645e0
inline bool UnitCTFOffenseAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitCTFOffenseAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::rttiHashUnitAgent == uRtti ||
           Agent::rttiHashAgent == uRtti;
}

// FUNCTION: SWBF2 0x00664610
inline unsigned int UnitCTFOffenseAgent::GetDerivedRtti() const
{
    return rttiHashUnitCTFOffenseAgent;
}

// FUNCTION: SWBF2 0x00664620
inline const char* UnitCTFOffenseAgent::GetDerivedRttiName() const
{
    return rttiNameUnitCTFOffenseAgent;
}
