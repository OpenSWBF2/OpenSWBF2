#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "Fsm.h"

class PblHash;

class Agent : public Fsm
{
public:
    virtual bool IsRtti(unsigned int hash) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool Update(float dt, bool b);

    static PblHash rttiHashAgent;
    static const char rttiNameAgent[];
};

inline bool Agent::IsRtti(unsigned int hash) const
{
    return rttiHashAgent == hash;
}

inline unsigned int Agent::GetDerivedRtti() const
{
    return rttiHashAgent;
}

inline const char* Agent::GetDerivedRttiName() const
{
    return rttiNameAgent;
}
