#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"

class GameObject;

namespace AI
{

// VTABLE: SWBF2 0x00798d48
class CTFGoal : public AIGoal
{
public:

    CTFGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    // FUNCTION: SWBF2 0x00487c20
    // AI::CTFGoal::IsRtti
    // FUNCTION: SWBF2 0x00487c50
    // AI::CTFGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x00487c60
    // AI::CTFGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(CTFGoal, AIGoal)

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

public:

    PblHandle<const GameObject> mFlag;
    PblVector3 mCapturePos;
};

}
