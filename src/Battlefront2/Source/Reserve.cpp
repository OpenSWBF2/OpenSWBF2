#include "Reserve.h"

#include "GameLoop.h"
#include "GameObject.h"

// FUNCTION: SWBF2 0x0062f440
ReserveObject::ReserveObject() : mReserveTime(-10.0f)
{
}

// FUNCTION: SWBF2 0x0062f460
bool ReserveObject::IsReserved(int iTeam, float fDelay) const
{
    const float fNow = GameLoop::GetMissionTime();
    float fWait = 2.0f;
    if (fDelay >= 0.0f)
    {
        fWait = fDelay;
    }
    return GetReserveTime(iTeam) + fWait > fNow;
}

// FUNCTION: SWBF2 0x0062f4b0
float ReserveObject::GetReserveTime(int  ) const
{
    return mReserveTime;
}

// FUNCTION: SWBF2 0x0062f4c0
void ReserveObject::SetReserveDelay(int  , float fDelay)
{
    mReserveTime = GameLoop::GetMissionTime() + fDelay;
}

// FUNCTION: SWBF2 0x0062f4f0
void ReserveObject::Reserve(const GameObject* pObj)
{
    mReserver = pObj;
    mReserveTime = GameLoop::GetMissionTime();
}

// FUNCTION: SWBF2 0x0062f530
void ReserveObject::Unreserve(const GameObject*  )
{
    mReserver = PblHandle<const GameObject>();
    mReserveTime = -10.0f;
}

