#include "ZephyrSkinReader.h"
#include "PebbleFL/Common/PblChunk.h"
#include "ZephyrSkinShared.h"
#include <new>
// FUNCTION: SWBF2 0x0072efe0
bool ZephyrSkinReader::ReadFile(PblMemChunk& kChunk)
{
    const char* pcData = kChunk.GetData();
    PblMemChunk kZafChunk(pcData);
    ZephyrSkinShared::DataOffsets kOffsets;

    unsigned int uMemSize = ZephyrSkinShared::GetRequiredMemSize(kZafChunk, kOffsets);
    if (uMemSize == 0)
        return false;

    m_pSkin = (ZephyrSkinShared*)new char[uMemSize];
    kZafChunk = PblMemChunk(pcData);
    if (m_pSkin->Open(kZafChunk, kOffsets, uMemSize))
        return true;
    return false;
}
