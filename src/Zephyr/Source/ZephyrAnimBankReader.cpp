#include "ZephyrAnimBankReader.h"
#include "PebbleFL/Common/PblChunk.h"
#include "Battlefront2/Source/GameMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include <string.h>
#include "ZephyrAnimBank.h"
#include "ZephyrCommon.h"

// FUNCTION: SWBF2 0x0072f0b0
bool ZephyrAnimBankReader::ReadFile(PblMemChunk& kChunk)
{
    const char* pcData = kChunk.GetData();
    int iSize = kChunk._iSize;
    ZephyrAnimFileHeaderD kHeader = *(const ZephyrAnimFileHeaderD*)pcData;
    pcData += sizeof(ZephyrAnimFileHeaderD);
    const ZephyrAnimFileHeaderD* pkHeader = &kHeader;

    if (pkHeader->kHeader.uiID != ZEPHYR_HEADER_ID('A', 'N', 'M', 'S'))
    {
        return false;
    }
    if (ZEPHYR_FILE_MAJOR_VERSION != pkHeader->u16MajorVersion || ZEPHYR_FILE_MINOR_VERSION < pkHeader->u16MinorVersion)
    {
        return false;
    }

    // The bank object, then the runtime data (uRuntimeSize bytes), then the file's data after the descriptions.
    const unsigned int uBankSize = ZEPHYR_ALIGN(sizeof(ZephyrAnimBank), 8);
    unsigned int uDescSize, uRuntimeSize;
    unsigned int uMemSize = ZEPHYR_ALIGN(uBankSize + ZephyrAnimBank::GetRequiredMemSize(pkHeader, uDescSize, uRuntimeSize), 8);
    unsigned int uAllocSize = ZEPHYR_ALIGN(uBankSize + uRuntimeSize + (iSize - uDescSize - sizeof(ZephyrAnimFileHeaderD)), 8);
    m_pBank = (ZephyrAnimBank*)RedAllocFromHeap(GameMemory::RunTimeHeap, uAllocSize, 0);
    if (!m_pBank)
    {
        return false;
    }

    // The descriptions are only needed while the bank opens: read them into a temporary buffer.
    int iTempHeap = GameMemory::TempLoadHeap;
    if (iTempHeap <= 0)
    {
        iTempHeap = GameMemory::RunTimeHeap;
    }
    char* pcDesc = (char*)RedAllocFromHeap(iTempHeap, uDescSize, 0);
    if (!pcDesc)
    {
        return false;
    }
    memcpy(pcDesc, pcData, uDescSize);

    char* pcMem = (char*)m_pBank + uBankSize;
    unsigned int uDataSize = iSize - uDescSize - sizeof(ZephyrAnimFileHeaderD);
    memcpy(pcMem + uRuntimeSize, pcData + uDescSize, uDataSize);
    if (!m_pBank->Open(pcMem, uMemSize - uBankSize, pcDesc, pkHeader))
    {
        return false;
    }

    RedFreeToHeap(iTempHeap, pcDesc, 0);
    return true;
}
