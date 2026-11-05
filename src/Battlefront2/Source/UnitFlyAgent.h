#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "UnitAgent.h"
#include "PebbleFL/Common/PblHashTable.h"

class BaseHint;
class EntityFlyer;

// VTABLE: SWBF2 0x007afa34
class UnitFlyAgent : public UnitAgent
{
public:
    UnitFlyAgent(UnitController* pCtrl);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual const char* NFINAL_GetStateString(int iState) const;
    virtual float GetUpdateRate() const;
    virtual int EventHandler(AI::EventInfo* pInfo);

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateStatePostVision(float dt);

    const EntityFlyer* GetFlyer() const;
    EntityFlyer* GetFlyer();
    bool IsTransport() const;
    bool IsNearLandingPt();

    bool IsRoofOverhead() const;
    bool IsNearATAT();

public:
    const BaseHint* mHint;
    float mNextHintCheckTime;
    bool mGotoNonOwnedCP;
    float mWaitTime;

    static PblHash rttiHashUnitFlyAgent;
    static const char rttiNameUnitFlyAgent[];
};
 
 
 

// FUNCTION: SWBF2 0x006666f0
inline bool UnitFlyAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitFlyAgent == uRtti)
    {
        return true;
    }
    return UnitAgent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00666720
inline unsigned int UnitFlyAgent::GetDerivedRtti() const
{
    return rttiHashUnitFlyAgent;
}

// FUNCTION: SWBF2 0x00666730
inline const char* UnitFlyAgent::GetDerivedRttiName() const
{
    return rttiNameUnitFlyAgent;
}
