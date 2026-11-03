#include "PebbleFL/Common/PblChunk.h"
#include "ZephyrCommon.h"
#include "ZephyrFile.h"

// FUNCTION: SWBF2 0x0072f610
bool CheckChunkId(PblMemChunk& kChunk, const char* pcID)
{
    if (kChunk.GetId() == ZEPHYR_HEADER_ID(pcID[0], pcID[1], pcID[2], pcID[3]))
    {
        return true;
    }
    return false;
}
