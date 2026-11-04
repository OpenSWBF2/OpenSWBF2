#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "AIGoal.h"

namespace AI
{

// VTABLE: SWBF2 0x00798ebc
class DeathmatchGoal : public AIGoal
{
public:
    DeathmatchGoal(unsigned char uTeam, float fWeight);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

protected:

    virtual void Assign(Character* pChr);

public:
    static PblHash rttiHashDeathmatchGoal;
    static const char rttiNameDeathmatchGoal[];
};

}

 

// FUNCTION: SWBF2 0x0048a980
inline bool AI::DeathmatchGoal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashDeathmatchGoal == uRtti)
    {
        return true;
    }
    return AIGoal::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0048a9b0
inline unsigned int AI::DeathmatchGoal::GetDerivedRtti() const
{
    return rttiHashDeathmatchGoal;
}

// FUNCTION: SWBF2 0x0048a9c0
inline const char* AI::DeathmatchGoal::GetDerivedRttiName() const
{
    return rttiNameDeathmatchGoal;
}
