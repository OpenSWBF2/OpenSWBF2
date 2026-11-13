#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Navigator.h"
#include "PebbleFL/Common/PblRtti.h"
#include "PebbleFL/Common/PblVector.h"

class ConnectivityGraphFollower;
class Obstacle;

// VTABLE: SWBF2 0x007aa820
class NavigatorGraphFollower : public Navigator
{
public:

    enum
    {
        STATE_PATHING = 2,
        STATE_FINAL = 3
    };

    // FUNCTION: SWBF2 0x005ac4e0
    // NavigatorGraphFollower::IsRtti
    // FUNCTION: SWBF2 0x005ad230
    // NavigatorGraphFollower::GetDerivedRtti
    // FUNCTION: SWBF2 0x005ad240
    // NavigatorGraphFollower::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(NavigatorGraphFollower, Navigator)

    NavigatorGraphFollower(AILowLevel* pOwner, const PblVector3& globalDest,
                           const Obstacle** ppIgnore, int iIgnoreCnt,
                           bool bRollIntoDest);
    virtual ~NavigatorGraphFollower();

    virtual void ReceivePathResult(AIPath* pPath);
    virtual const PblVector3& GetGlobalDest() const;

protected:
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateState(float dt);

public:
    PblVector3 mGlobalDest;
    int mIgnoreCnt;
    const Obstacle* mIgnore[2];
    ConnectivityGraphFollower* mGraphFollower;
    bool mRollIntoDest;
};

 
 
 
 

