#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"
#include "PebbleFL/Common/PblRtti.h"

namespace AI
{

// VTABLE: SWBF2 0x00798a58
class ConquestGoal : public AIGoal
{
public:
    ConquestGoal(unsigned char uTeam, float fWeight);

    // FUNCTION: SWBF2 0x004829c0
    // AI::ConquestGoal::IsRtti
    // FUNCTION: SWBF2 0x004829f0
    // AI::ConquestGoal::GetDerivedRtti
    // FUNCTION: SWBF2 0x00482a00
    // AI::ConquestGoal::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(ConquestGoal, AIGoal)

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

    static void sAssign(Character* pChr, const CommandPost* pPost);
    static void sIssueCmd(Character* pChr, const CommandPost* pPost);
    static const CommandPost* sGenerateCP(Character* pChr, const CommandPost* pPost, bool bSecondPass);
    static bool sIsPilotingVehicle(Character* pChr);
    static bool sIsPassenger(Character* pChr);
    static float sGetImportance(const CommandPost* pPost, int iTeam);

public:
};

}
