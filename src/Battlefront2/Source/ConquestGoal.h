#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"

namespace AI
{

// VTABLE: SWBF2 0x00798a58
class ConquestGoal : public AIGoal
{
public:
    ConquestGoal(unsigned char uTeam, float fWeight);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

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
    static PblHash rttiHashConquestGoal;
    static const char rttiNameConquestGoal[];
};

}

 

// FUNCTION: SWBF2 0x004829c0
inline bool AI::ConquestGoal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashConquestGoal == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004829f0
inline unsigned int AI::ConquestGoal::GetDerivedRtti() const
{
    return rttiHashConquestGoal;
}

// FUNCTION: SWBF2 0x00482a00
inline const char* AI::ConquestGoal::GetDerivedRttiName() const
{
    return rttiNameConquestGoal;
}
