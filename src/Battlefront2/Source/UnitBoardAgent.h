#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblHashTable.h"

class GameObject;

// VTABLE: SWBF2 0x007af724
class UnitBoardAgent : public UnitAgent
{
public:
    UnitBoardAgent(UnitController* pCtrl, GameObject* pVehicle, int iVOResponseHandle);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool ShouldQueueCurCmd(const UnitCmdInfo& cmd) const;
    virtual const char* NFINAL_GetStateString(int iState) const;
    virtual int EventHandler(AI::EventInfo* pInfo);

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

public:
    PblHandle<GameObject> mVehicle;
    int mVOResponseHandle;
    float mStateEnterTime;

    static PblHash rttiHashUnitBoardAgent;
    static const char rttiNameUnitBoardAgent[];
};
 
 
 
 

// FUNCTION: SWBF2 0x00661f80
inline bool UnitBoardAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitBoardAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00661fb0
inline unsigned int UnitBoardAgent::GetDerivedRtti() const
{
    return rttiHashUnitBoardAgent;
}

// FUNCTION: SWBF2 0x00661fc0
inline const char* UnitBoardAgent::GetDerivedRttiName() const
{
    return rttiNameUnitBoardAgent;
}
