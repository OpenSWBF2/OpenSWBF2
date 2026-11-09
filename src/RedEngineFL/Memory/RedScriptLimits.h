#pragma once

#include <limits.h>

#include "RedEngineFL/Util/RedLog.h"

namespace RedScriptLimits
{

const unsigned int kPoolBytesMax = 0x1000000;
const unsigned int kTempHeapMin = 4096;
const unsigned int kTempHeapMax = 0x800000;
const unsigned int kArtistStealMin = 32;
const unsigned int kArtistStealMax = 0x2000;
const unsigned int kHeapCapMax = 0x20000000;
const unsigned int kHeapCapTotalMax = 0xC0000000;

struct ScriptLimit
{
    const char* pcEntry;
    unsigned int uMinBytes;
    unsigned int uMaxBytes;
    bool bRefuse;
    bool bKeepWrap;
};

const ScriptLimit kSetMemoryPoolSize = { "SetMemoryPoolSize", 0, kPoolBytesMax, false, true };
const ScriptLimit kAddWalkerType = { "AddWalkerType", 0, kPoolBytesMax, false, true };
const ScriptLimit kSetupTempHeap = { "SetupTempHeap", kTempHeapMin, kTempHeapMax, false, false };
const ScriptLimit kStealArtistHeap = { "StealArtistHeap", kArtistStealMin, kArtistStealMax, true, false };
const ScriptLimit kSetHeapCap = { "SetHeapCap", 0, kHeapCapMax, false, false };

inline bool PoolBytesOk(unsigned int uCount, unsigned int uSize, unsigned int uCeiling)
{
    return uSize != 0 && uCount <= uCeiling / uSize;
}

inline unsigned int ClampScriptSize(const ScriptLimit& rLimit, const char* pcLabel, long long llValue,
                                    unsigned int uUnit)
{
    if (uUnit == 0)
    {
        uUnit = 1;
    }
    const unsigned int uMin = rLimit.uMinBytes / uUnit + (rLimit.uMinBytes % uUnit != 0 ? 1 : 0);
    const unsigned int uMax = rLimit.uMaxBytes / uUnit;
    if (rLimit.bKeepWrap && llValue < 0 && llValue >= INT_MIN)
    {
        return (unsigned int)llValue;
    }
    if (llValue >= (long long)uMin && llValue <= (long long)uMax)
    {
        return (unsigned int)llValue;
    }
    const char* pcName = pcLabel ? pcLabel : "";
    if (rLimit.bRefuse)
    {
        LOG(LogMemory, Warning, "%s(%s): script value %lld refused", rLimit.pcEntry, pcName, llValue);
        return 0;
    }
    const unsigned int uResult = llValue < (long long)uMin ? uMin : uMax;
    LOG(LogMemory, Warning, "%s(%s): script value %lld clamped to %u", rLimit.pcEntry, pcName, llValue,
        uResult);
    return uResult;
}

}
