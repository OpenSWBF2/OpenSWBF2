#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "Navigator.h"
#include "PebbleFL/Common/PblRtti.h"

class Obstacle;

// VTABLE: SWBF2 0x007aa88c
class NavigatorPathFollower : public Navigator
{
public:

    enum
    {
        STATE_WAITFORPATH = 2,
        STATE_FOLLOWPATH = 3,
        STATE_FINAL = 4
    };

    // FUNCTION: SWBF2 0x005adad0
    // NavigatorPathFollower::IsRtti
    // FUNCTION: SWBF2 0x005adb00
    // NavigatorPathFollower::GetDerivedRtti
    // FUNCTION: SWBF2 0x005adb10
    // NavigatorPathFollower::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(NavigatorPathFollower, Navigator)

    NavigatorPathFollower(AILowLevel* pOwner, bool bDoCallbacksOnPts,
                          const Obstacle** ppIgnore, int iIgnoreCnt,
                          bool bRollIntoDest);
    NavigatorPathFollower(AILowLevel* pOwner, bool bDoCallbacksOnPts,
                          const Obstacle** ppIgnore, int iIgnoreCnt,
                          bool bRollIntoDest, AIPath* pPath);
    virtual ~NavigatorPathFollower();

    virtual void ReceivePathResult(AIPath* pPath);

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateState(float dt);

public:
    int mIgnoreCnt;
    const Obstacle* mIgnore[2];
    bool mRollIntoDest;
    bool mDoCallbacksOnPts;
};

 

