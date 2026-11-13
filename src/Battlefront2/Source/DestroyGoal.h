#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"

namespace AI
{

// VTABLE: SWBF2 0x00798f88
class DestroyGoal : public AIGoal
{
public:
    DestroyGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    // FUNCTION: SWBF2 0x0048c6b0
    // AI::DestroyGoal::IsRtti
    // FUNCTION: SWBF2 0x0048c6e0
    // AI::DestroyGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x0048c6f0
    // AI::DestroyGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(DestroyGoal, AIGoal)

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

public:

    GoalTarget mTarget;
};

}
