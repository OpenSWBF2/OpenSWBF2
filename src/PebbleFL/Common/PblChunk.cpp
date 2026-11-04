#include <string.h>

#include "PblFile.h"
#include "PblChunk.h"
#include "PblVector.h"

inline void PblFileChunk::Realign()
{
    _pFile->SeekAlign(4);
}

inline void PblMemChunk::Realign()
{
    _pData = _pData + ((-(int)_pData) & 3);
}

// FUNCTION: SWBF2 0x0072a8a0
PblFileChunk::PblFileChunk(PblFile* pFile)
{
    _pFile = pFile;
    _ReadHeader();
    _iNext = _pFile->Tell();
}

// FUNCTION: SWBF2 0x0072ab70
void PblFileChunk::_ReadHeader()
{
    Realign();
    _iId = _pFile->Read32();
    _iSize = _pFile->Read32();
    _iEnd = _pFile->Tell() + _iSize;
}

// FUNCTION: SWBF2 0x0072a900
void PblFileChunk::GetChunkData(void* pBuffer)
{
    unsigned int* puiOut = (unsigned int*)pBuffer;
    puiOut[0] = _iId;
    puiOut[1] = _iSize;
    _pFile->ReadData(puiOut + 2, _iSize);
}

// FUNCTION: SWBF2 0x0072a930
unsigned int PblFileChunk::PeekNextChildId() const
{
    int iHere = _pFile->Tell();
    _pFile->Seek(_iNext, PblFile::SeekFromStart);
    _pFile->SeekAlign(4);
    unsigned int uiId = _pFile->Read32();
    _pFile->Seek(iHere, PblFile::SeekFromStart);
    return uiId;
}

// FUNCTION: SWBF2 0x0072aa30
PblFileChunk PblFileChunk::ReadNextChild()
{
    _pFile->Seek(_iNext, PblFile::SeekFromStart);
    Realign();
    PblFileChunk Chunk(_pFile);
    _iNext = _pFile->Tell() + Chunk.GetSize();
    return Chunk;
}

// FUNCTION: SWBF2 0x0072aad0
char* PblFileChunk::ReadString(char* pcBuffer, int iBufSize)
{
    char* pcOut = pcBuffer;
    char cChar = _pFile->Read8();
    while (cChar != '\0')
    {
        if (iBufSize == 0)
        {
            return 0;
        }
        *pcOut++ = cChar;
        cChar = _pFile->Read8();
        --iBufSize;
    }
    if (iBufSize <= 0)
    {
        return 0;
    }
    *pcOut = '\0';
    return pcBuffer;
}

// FUNCTION: SWBF2 0x0072a8d0
PblMemChunk::PblMemChunk(const void* pData)
{
    _pData = (char*)pData;
    _ReadHeader();
    _pNext = _pData;
}

// FUNCTION: SWBF2 0x0072abc0
void PblMemChunk::_ReadHeader()
{
    Realign();
    _pEnd = _pData + 8;
    _iId = Read32();
    _iSize = Read32();
    _pEnd = _pData + _iSize;
}

// FUNCTION: SWBF2 0x0072aa00
char PblMemChunk::Read8()
{
    return *_pData++;
}

// FUNCTION: SWBF2 0x0072a990
short PblMemChunk::Read16()
{
    unsigned short iLow = (unsigned char)*_pData++;
    unsigned short iHigh = (unsigned short)(*_pData++ << 8);
    return iLow | iHigh;
}

// FUNCTION: SWBF2 0x0072a9c0
int PblMemChunk::Read32()
{
    int iByte0 = (unsigned char)*_pData++;
    int iByte1 = (unsigned char)*_pData++ << 8;
    int iByte2 = (unsigned char)*_pData++ << 16;
    int iByte3 = *_pData++ << 24;
    return iByte0 | iByte1 | iByte2 | iByte3;
}

// FUNCTION: SWBF2 0x0072aa10
float PblMemChunk::ReadFloat()
{
    int iValue = Read32();
    return *(float*)&iValue;
}

// FUNCTION: SWBF2 0x0072ab20
char* PblMemChunk::ReadString(char* pcBuffer, int iBufSize)
{
    char* pcOut = pcBuffer;
    char cChar = Read8();
    while (cChar != '\0')
    {
        if (iBufSize == 0)
        {
            return 0;
        }
        *pcOut++ = cChar;
        cChar = Read8();
        --iBufSize;
    }
    if (iBufSize <= 0)
    {
        return 0;
    }
    *pcOut = '\0';
    return pcBuffer;
}

// FUNCTION: SWBF2 0x0072aa90
PblMemChunk PblMemChunk::ReadNextChild()
{
    _pData = _pNext;
    Realign();
    PblMemChunk Chunk(_pData);
    _pNext = _pData + Chunk._iSize + 8;
    return Chunk;
}

int PblMemChunk::GetSize() const
{
    return _iSize;
}

void PblMemChunk::ReadData(void* pBuffer, int iNumBytes)
{
    memcpy(pBuffer, _pData, iNumBytes);
    _pData += iNumBytes;
}
