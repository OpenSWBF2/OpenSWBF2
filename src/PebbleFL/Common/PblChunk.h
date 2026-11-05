#pragma once

#include <stddef.h>
#include "PblFile.h"

class PblFileChunk
{
public:

    PblFileChunk(PblFile* pFile);

    PblFileChunk ReadNextChild();

    unsigned int PeekNextChildId() const;

    void GetChunkData(void* pBuffer);

    void ReadData(void* pBuffer, int iNumBytes) { _pFile->ReadData(pBuffer, iNumBytes); }
    char Read8() { return _pFile->Read8(); }
    short Read16() { return _pFile->Read16(); }
    int Read32() { return _pFile->Read32(); }
    float ReadFloat() { return _pFile->ReadFloat(); }

    char* ReadString(char* pcBuffer, int iBufSize);

    void Realign();

    bool IsDone() const
    {
        return _iNext + ((-_iNext) & 3) >= _iEnd;
    }

    int GetSize() const { return _iSize; }

    unsigned int _iId;
    int _iSize;
    int _iEnd;
    int _iNext;
    PblFile* _pFile;

private:
    void _ReadHeader();
};
 
 

// VTABLE: SWBF2 0x00772ba4
class PblMemChunk
{
public:
    PblMemChunk(const void* pData);
    PblMemChunk(const PblMemChunk& other);
    virtual ~PblMemChunk();

    // SYNTHETIC: SWBF2 0x00578fb0
    // PblMemChunk::`scalar deleting destructor'

    char Read8();
    short Read16();
    int Read32();
    float ReadFloat();
    void ReadData(void* pBuffer, int iNumBytes);
    char* ReadString(char* pcBuffer, int iBufSize);
    PblMemChunk ReadNextChild();
    int GetSize() const;
    void Realign();

    unsigned int GetId() const { return _iId; }
    const char* GetData() const { return _pData; }
    bool IsDone() const { return _pNext + ((-(int)_pNext) & 3) >= _pEnd; }

    unsigned int _iId;
    int _iSize;
    char* _pNext;
    char* _pEnd;
    char* _pData;

private:
    void _ReadHeader();
};
 
 

// FUNCTION: SWBF2 0x00578fa0
inline PblMemChunk::~PblMemChunk()
{
}

inline PblMemChunk::PblMemChunk(const PblMemChunk& other)
{
    _iId = other._iId;
    _iSize = other._iSize;
    _pNext = other._pNext;
    _pEnd = other._pEnd;
    _pData = other._pData;
}
