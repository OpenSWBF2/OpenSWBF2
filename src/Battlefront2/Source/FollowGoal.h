#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"

namespace AI
{

// VTABLE: SWBF2 0x0079f2cc
class FollowGoal : public AIGoal
{
public:
    FollowGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    // FUNCTION: SWBF2 0x0052d1c0
    // AI::FollowGoal::IsRtti
    // FUNCTION: SWBF2 0x0052d1f0
    // AI::FollowGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x0052d200
    // AI::FollowGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(FollowGoal, AIGoal)

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

public:

    GoalTarget mTarget;
};

}
