#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"

namespace AI
{

// VTABLE: SWBF2 0x00798f88
class DestroyGoal : public AIGoal
{
public:
    DestroyGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

public:
    static PblHash rttiHashDestroyGoal;
    static const char rttiNameDestroyGoal[];

    GoalTarget mTarget;
};

}

 
 

// FUNCTION: SWBF2 0x0048c6b0
inline bool AI::DestroyGoal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashDestroyGoal == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0048c6e0
inline unsigned int AI::DestroyGoal::GetDerivedRtti() const
{
    return rttiHashDestroyGoal;
}

// FUNCTION: SWBF2 0x0048c6f0
inline const char* AI::DestroyGoal::GetDerivedRttiName() const
{
    return rttiNameDestroyGoal;
}
