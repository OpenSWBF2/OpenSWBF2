#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "AIGoal.h"
#include "PebbleFL/Common/PblHandle.h"

class GameObject;

namespace AI
{

struct GoalTarget;

// VTABLE: SWBF2 0x00798d78
class CTFGoalD : public AIGoal
{
public:

    CTFGoalD(unsigned char uTeam, float fWeight, const GoalTarget& target);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

protected:

    virtual void Assign(Character* pChr);

public:
    static PblHash rttiHashCTFGoalD;
    static const char rttiNameCTFGoalD[];

    PblHandle<const GameObject> mFlag;
};

}

 
 

// FUNCTION: SWBF2 0x00488040
inline bool AI::CTFGoalD::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCTFGoalD == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00488070
inline unsigned int AI::CTFGoalD::GetDerivedRtti() const
{
    return rttiHashCTFGoalD;
}

// FUNCTION: SWBF2 0x00488080
inline const char* AI::CTFGoalD::GetDerivedRttiName() const
{
    return rttiNameCTFGoalD;
}
