#pragma once

#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblSimOrder.h"

inline float WobblePhase(const PblHandled* pOwner, const void* pThis)
{
    if (PblGetSimOrder() == PBL_SIMORDER_SERIAL)
    {
        return (float)(pOwner->PblHandled::GetHandleId() & 0xf);
    }
    return (float)(((unsigned int)pThis >> 2) & 0xf);
}
