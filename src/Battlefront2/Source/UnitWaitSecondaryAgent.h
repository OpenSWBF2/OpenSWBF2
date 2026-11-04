#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "PebbleFL/Common/PblHashTable.h"

class GameObject;

// VTABLE: SWBF2 0x007afe84
class UnitWaitSecondaryAgent : public UnitAgent
{
public:
    UnitWaitSecondaryAgent(UnitController* pCtrl, float fWaitTime, bool bArg);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool ShouldQueueCurCmd(const UnitCmdInfo& cmd) const;
    virtual int EventHandler(AI::EventInfo* pInfo);
    virtual const char* NFINAL_GetStateString(int iState) const;

    void TakeoffNow() { mWaitUntilMissionTime = 0.0f; }

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

    GameObject* IsPlayerLookingAtVehicle();

public:
    float mWaitUntilMissionTime;
    float mPlayerWaitTimeout;
    float mImpatientSoundTime;

    static PblHash rttiHashUnitWaitSecondaryAgent;
    static const char rttiNameUnitWaitSecondaryAgent[];
};
 
 
 

// FUNCTION: SWBF2 0x0066b670
inline bool UnitWaitSecondaryAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitWaitSecondaryAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0066b6a0
inline unsigned int UnitWaitSecondaryAgent::GetDerivedRtti() const
{
    return rttiHashUnitWaitSecondaryAgent;
}

// FUNCTION: SWBF2 0x0066b6b0
inline const char* UnitWaitSecondaryAgent::GetDerivedRttiName() const
{
    return rttiNameUnitWaitSecondaryAgent;
}
