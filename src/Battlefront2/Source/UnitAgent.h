#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Agent.h"
#include "AttackHelper.h"
#include "Controllable.h"
#include "UnitController.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class AILowLevel;
class GameObject;
class PblHash;
class UnitController;
class UnitCmdInfo;
namespace AI
{
class EventInfo;
}

// VTABLE: SWBF2 0x007af6a4
class UnitAgent : public Agent
{

    friend class CombatHelper;

public:

    static void* operator new(size_t uiSize) { return sMemoryPool.Allocate(uiSize); }
    // SYNTHETIC: SWBF2 0x0065dd30
    // UnitAgent::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }
    static MemoryPool sMemoryPool;

    UnitAgent(UnitController* pCtrl);
    virtual ~UnitAgent();
    virtual void Kill();
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
protected:
    virtual void UpdateState(float dt);
public:

    virtual void UpdateStatePostVision(float dt) = 0;
protected:
    virtual void FaceTargetCommon(ControllableHeight eHeight, bool bFlag, const PblVector3& vTarget);
public:
    virtual void FaceTarget();
    virtual void FaceTarget(ControllableHeight eHeight);
    virtual void FaceTarget(ControllableHeight eHeight, const PblVector3& vTarget);
    virtual bool ShouldQueueCurCmd(const UnitCmdInfo& cmd) const;
    virtual float GetUpdateRate() const;
    virtual const char* NFINAL_GetStateString(int iState) const = 0;
    virtual int EventHandler(AI::EventInfo* pInfo);

    static PblHash rttiHashUnitAgent;
    static const char rttiNameUnitAgent[];

    const Controllable* GetMe() const
    {
        return mCtrl->mOwner;
    }
    Controllable* GetMe()
    {
        return mCtrl->mOwner;
    }
    AILowLevel& GetLowLevel()
    {
        return mCtrl->mLowLevel;
    }

    bool RunForVehicle(const GameObject* pVehicle) const;

protected:
    bool AreNearbyUnitsInCombat();
    void PlayDamageEventVO(AI::EventInfo* pInfo);
    int DamagedEventPlayerCentricness(AI::EventInfo* pInfo);

    int HandleDamagedEvent(AI::EventInfo* pInfo);

private:
    bool ShouldAgentGetInNearbyVehicle(const GameObject* pVehicle) const;
    int HandleSquadCommandEvent(AI::EventInfo* pInfo);
    int Handle_Vision_CanSeeEnemy(AI::EventInfo* pInfo);
    int Handle_Audible_CanHearEnemy(AI::EventInfo* pInfo);

public:

    UnitController* mCtrl;
    AttackHelper mAttackHelper;
    float mCheckNearbyUnitsTime;
    bool mNearbyUnitsInCombat;
};

 

// FUNCTION: SWBF2 0x0065dce0
inline bool UnitAgent::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitAgent == uRtti)
    {
        return true;
    }
    return Agent::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0065dd10
inline unsigned int UnitAgent::GetDerivedRtti() const
{
    return rttiHashUnitAgent;
}

// FUNCTION: SWBF2 0x0065dd20
inline const char* UnitAgent::GetDerivedRttiName() const
{
    return rttiNameUnitAgent;
}


