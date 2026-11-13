#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"
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

    // FUNCTION: SWBF2 0x00488040
    // AI::CTFGoalD::IsRtti
    // FUNCTION: SWBF2 0x00488070
    // AI::CTFGoalD::GetDerivedRtti
    // FUNCTION: SWBF2 0x00488080
    // AI::CTFGoalD::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(CTFGoalD, AIGoal)

protected:

    virtual void Assign(Character* pChr);

public:

    PblHandle<const GameObject> mFlag;
};

}
