#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "AIGoal.h"

// VTABLE: SWBF2 0x007af9cc
class UnitDestroyAgent : public UnitAgent
{
public:
    UnitDestroyAgent(UnitController* pCtrl, const AI::GoalTarget& target);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual const char* NFINAL_GetStateString(int iState) const;
    virtual int EventHandler(AI::EventInfo* pInfo);

protected:

    enum
    {
        STATE_START = 1,
        STATE_Goto = 2,
        STATE_WaitForSpawn = 3,
        STATE_FINAL = 4
    };

    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

    void CheckTargetPosition(const PblVector3& vPos);

public:
    AI::GoalTarget mTarget;
    PblVector3 mLastSeekPos;

    static PblHash rttiHashUnitDestroyAgent;
    static const char rttiNameUnitDestroyAgent[];
};

 
 
 

// FUNCTION: SWBF2 0x00666000
inline bool UnitDestroyAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitDestroyAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00666030
inline unsigned int UnitDestroyAgent::GetDerivedRtti() const
{
    return rttiHashUnitDestroyAgent;
}

// FUNCTION: SWBF2 0x00666040
inline const char* UnitDestroyAgent::GetDerivedRttiName() const
{
    return rttiNameUnitDestroyAgent;
}
