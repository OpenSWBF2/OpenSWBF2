#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "UnitController.h"

// VTABLE: SWBF2 0x007afbe8 CommandFsm
// VTABLE: SWBF2 0x007afc28 PblHandled
// VTABLE: SWBF2 0x007afc30 Controller
class UnitMobileController : public UnitController
{
public:

    UnitMobileController(Controllable* pOwner);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashUnitMobileController;
    static const char rttiNameUnitMobileController[];

protected:

    virtual void ApplyNextState();
};

// FUNCTION: SWBF2 0x006691b0
inline unsigned int UnitMobileController::GetDerivedRtti() const
{
    return rttiHashUnitMobileController;
}

// FUNCTION: SWBF2 0x006691c0
inline const char* UnitMobileController::GetDerivedRttiName() const
{
    return rttiNameUnitMobileController;
}

// FUNCTION: SWBF2 0x00669180
inline bool UnitMobileController::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitMobileController == uRtti)
    {
        return true;
    }
    return UnitController::IsRtti(uRtti);
}
