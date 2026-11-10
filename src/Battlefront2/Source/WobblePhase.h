#pragma once

#include "PebbleFL/Common/PblHandle.h"

inline float WobblePhase(const PblHandled* pOwner)
{
    return (float)(pOwner->PblHandled::GetHandleId() & 0xf);
}
