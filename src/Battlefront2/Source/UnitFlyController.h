#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "UnitController.h"

// VTABLE: SWBF2 0x007afae0 Controller
// VTABLE: SWBF2 0x007afaa0 CommandFsm
// VTABLE: SWBF2 0x007afaf8 PblHandled
class UnitFlyController : public UnitController
{
public:
    UnitFlyController(Controllable* pOwner);

    // SYNTHETIC: SWBF2 0x00667d40
    // UnitFlyController::`scalar deleting destructor'

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashUnitFlyController;
    static const char rttiNameUnitFlyController[];

protected:
    virtual void ApplyNextState();
    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateState(float dt);
};

 

// FUNCTION: SWBF2 0x00667cf0
inline bool UnitFlyController::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitFlyController == uRtti)
    {
        return true;
    }
    return UnitController::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00667d20
inline unsigned int UnitFlyController::GetDerivedRtti() const
{
    return rttiHashUnitFlyController;
}

// FUNCTION: SWBF2 0x00667d30
inline const char* UnitFlyController::GetDerivedRttiName() const
{
    return rttiNameUnitFlyController;
}
