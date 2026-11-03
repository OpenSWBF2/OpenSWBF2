#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "EventInfo.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007afd98
class UnitUnBoardAgent : public UnitAgent
{
public:
    UnitUnBoardAgent(UnitController* pCtrl);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool ShouldQueueCurCmd(const UnitCmdInfo& cmd) const;
    virtual int EventHandler(AI::EventInfo* pInfo);
    virtual const char* NFINAL_GetStateString(int iState) const;

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

public:
    static PblHash rttiHashUnitUnBoardAgent;
    static const char rttiNameUnitUnBoardAgent[];
};
 

// FUNCTION: SWBF2 0x0066b1b0
inline bool UnitUnBoardAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitUnBoardAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0066b1e0
inline unsigned int UnitUnBoardAgent::GetDerivedRtti() const
{
    return rttiHashUnitUnBoardAgent;
}

// FUNCTION: SWBF2 0x0066b1f0
inline const char* UnitUnBoardAgent::GetDerivedRttiName() const
{
    return rttiNameUnitUnBoardAgent;
}
