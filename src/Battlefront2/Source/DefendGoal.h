#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"

namespace AI
{

// VTABLE: SWBF2 0x00798f34
class DefendGoal : public AIGoal
{
public:
    DefendGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    // FUNCTION: SWBF2 0x0048b4f0
    // AI::DefendGoal::IsRtti
    // FUNCTION: SWBF2 0x0048b520
    // AI::DefendGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x0048b530
    // AI::DefendGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(DefendGoal, AIGoal)

protected:

    virtual void Assign(Character* pChr);

public:

    GoalTarget mTarget;
};

}
