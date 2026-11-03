#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "UnitCmdInfo.h"
#include "CombatHelper.h"

namespace AI
{
class EventInfo;
}

// VTABLE: SWBF2 0x007af7b0
class UnitCombatAgent : public UnitAgent
{
public:
    static PblHash rttiHashUnitCombatAgent;
    static const char rttiNameUnitCombatAgent[];

    enum
    {
        STATE_START = 1,
        STATE_Combat = 2,
        STATE_FINAL = 3
    };

    UnitCombatAgent(UnitController* pCtrl, const UnitCombatInfo& info);

    int EventHandler(AI::EventInfo* pInfo);

    virtual void Kill();
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool ShouldQueueCurCmd(const UnitCmdInfo& cmd) const;
    virtual float GetUpdateRate() const;

    virtual const char* NFINAL_GetStateString(int iState) const;
protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);
public:

    UnitCombatInfo mCombatInfo;
    CombatHelper mCombatHelper;
};

 
 

// FUNCTION: SWBF2 0x00662e10
inline bool UnitCombatAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitCombatAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::rttiHashUnitAgent == uRtti ||
           Agent::rttiHashAgent == uRtti;
}

// FUNCTION: SWBF2 0x00662e40
inline unsigned int UnitCombatAgent::GetDerivedRtti() const
{
    return rttiHashUnitCombatAgent;
}

// FUNCTION: SWBF2 0x00662e50
inline const char* UnitCombatAgent::GetDerivedRttiName() const
{
    return rttiNameUnitCombatAgent;
}
