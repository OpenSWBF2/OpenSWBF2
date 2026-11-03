#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblVector.h"

// VTABLE: SWBF2 0x007afc58
class UnitRandomAgent : public UnitAgent
{
public:
    UnitRandomAgent(UnitController* pCtrl);

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

    bool IsPointValid(const PblVector3& vPoint);
    void PickRandomDest();

public:
    PblVector3 mStartPos;
    float mRandTime;

    static PblHash rttiHashUnitRandomAgent;
    static const char rttiNameUnitRandomAgent[];
};
 
 
 

// FUNCTION: SWBF2 0x00669320
inline bool UnitRandomAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitRandomAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00669350
inline unsigned int UnitRandomAgent::GetDerivedRtti() const
{
    return rttiHashUnitRandomAgent;
}

// FUNCTION: SWBF2 0x00669360
inline const char* UnitRandomAgent::GetDerivedRttiName() const
{
    return rttiNameUnitRandomAgent;
}
