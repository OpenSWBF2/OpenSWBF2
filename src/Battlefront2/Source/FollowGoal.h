#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"

namespace AI
{

// VTABLE: SWBF2 0x0079f2cc
class FollowGoal : public AIGoal
{
public:
    FollowGoal(unsigned char uTeam, float fWeight, const GoalTarget& target);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    virtual void Update();

protected:

    virtual void Assign(Character* pChr);

public:
    static PblHash rttiHashFollowGoal;
    static const char rttiNameFollowGoal[];

    GoalTarget mTarget;
};

}

 
 

// FUNCTION: SWBF2 0x0052d1c0
inline bool AI::FollowGoal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashFollowGoal == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0052d1f0
inline unsigned int AI::FollowGoal::GetDerivedRtti() const
{
    return rttiHashFollowGoal;
}

// FUNCTION: SWBF2 0x0052d200
inline const char* AI::FollowGoal::GetDerivedRttiName() const
{
    return rttiNameFollowGoal;
}
