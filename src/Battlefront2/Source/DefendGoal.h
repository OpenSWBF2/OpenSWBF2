#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"

namespace AI
{

// VTABLE: SWBF2 0x00798f34
class DefendGoal : public AIGoal
{
public:
    DefendGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

protected:

    virtual void Assign(Character* pChr);

public:
    static PblHash rttiHashDefendGoal;
    static const char rttiNameDefendGoal[];

    GoalTarget mTarget;
};

}

 
 

// FUNCTION: SWBF2 0x0048b4f0
inline bool AI::DefendGoal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashDefendGoal == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0048b520
inline unsigned int AI::DefendGoal::GetDerivedRtti() const
{
    return rttiHashDefendGoal;
}

// FUNCTION: SWBF2 0x0048b530
inline const char* AI::DefendGoal::GetDerivedRttiName() const
{
    return rttiNameDefendGoal;
}
