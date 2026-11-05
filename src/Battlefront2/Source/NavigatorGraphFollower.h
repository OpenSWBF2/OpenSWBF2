#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Navigator.h"
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

    static PblHash rttiHashNavigatorGraphFollower;
    static const char rttiNameNavigatorGraphFollower[];

    NavigatorGraphFollower(AILowLevel* pOwner, const PblVector3& globalDest,
                           const Obstacle** ppIgnore, int iIgnoreCnt,
                           bool bRollIntoDest);
    virtual ~NavigatorGraphFollower();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
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

 
 
 
 

// FUNCTION: SWBF2 0x005ac4e0
inline bool NavigatorGraphFollower::IsRtti(unsigned int uRtti) const
{
    if (rttiHashNavigatorGraphFollower == uRtti)
    {
        return true;
    }
    return Navigator::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005ad230
inline unsigned int NavigatorGraphFollower::GetDerivedRtti() const
{
    return rttiHashNavigatorGraphFollower;
}

// FUNCTION: SWBF2 0x005ad240
inline const char* NavigatorGraphFollower::GetDerivedRttiName() const
{
    return rttiNameNavigatorGraphFollower;
}
