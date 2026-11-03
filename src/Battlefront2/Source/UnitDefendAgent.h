#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "AIGoal.h"
#include "CmdInfo.h"
#include "CombatHelper.h"
#include "DefendHelper.h"

// VTABLE: SWBF2 0x007af960
class UnitDefendAgent : public UnitAgent
{
public:

    UnitDefendAgent(UnitController* pCtrl, const AI::GoalTarget& target);
    // SYNTHETIC: SWBF2 0x00665bb0
    // UnitDefendAgent::`scalar deleting destructor'
    // SYNTHETIC: SWBF2 0x00665be0
    // UnitDefendAgent::~UnitDefendAgent
    virtual void Kill();
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual float GetUpdateRate() const;
    virtual const char* NFINAL_GetStateString(int iState) const;
    virtual int EventHandler(AI::EventInfo* pInfo);

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

public:
    DefendHelper mDefendHelper;
    CmdInfo mCombatCmd;
    CombatHelper mCombatHelper;
    AI::GoalTarget mTarget;
    bool mMaintainVisualContact;

    static PblHash rttiHashUnitDefendAgent;
    static const char rttiNameUnitDefendAgent[];
};

 
 
 
 
 

// FUNCTION: SWBF2 0x00665b60
inline bool UnitDefendAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitDefendAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00665b90
inline unsigned int UnitDefendAgent::GetDerivedRtti() const
{
    return rttiHashUnitDefendAgent;
}

// FUNCTION: SWBF2 0x00665ba0
inline const char* UnitDefendAgent::GetDerivedRttiName() const
{
    return rttiNameUnitDefendAgent;
}
