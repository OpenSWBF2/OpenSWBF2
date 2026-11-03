#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "UnitController.h"

// VTABLE: SWBF2 0x007afccc Controller
// VTABLE: SWBF2 0x007afce4 PblHandled
// VTABLE: SWBF2 0x007afcec CommandFsm
class UnitStationaryController : public UnitController
{
public:
    UnitStationaryController(Controllable* pOwner);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashUnitStationaryController;
    static const char rttiNameUnitStationaryController[];

protected:
    virtual void ApplyNextState();
};

 

// FUNCTION: SWBF2 0x00669720
inline bool UnitStationaryController::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitStationaryController == uRtti)
    {
        return true;
    }
    return UnitController::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00669750
inline unsigned int UnitStationaryController::GetDerivedRtti() const
{
    return rttiHashUnitStationaryController;
}

// FUNCTION: SWBF2 0x00669760
inline const char* UnitStationaryController::GetDerivedRttiName() const
{
    return rttiNameUnitStationaryController;
}
