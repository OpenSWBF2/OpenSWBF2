#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"

namespace AI
{

// VTABLE: SWBF2 0x00798ebc
class DeathmatchGoal : public AIGoal
{
public:
    DeathmatchGoal(unsigned char uTeam, float fWeight);

    // FUNCTION: SWBF2 0x0048a980
    // AI::DeathmatchGoal::IsRtti
    // FUNCTION: SWBF2 0x0048a9b0
    // AI::DeathmatchGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x0048a9c0
    // AI::DeathmatchGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(DeathmatchGoal, AIGoal)

protected:

    virtual void Assign(Character* pChr);

public:
};

}
