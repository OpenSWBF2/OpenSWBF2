#pragma once

#include <stddef.h>
#include <string.h>

class PblFile
{
public:

    enum SeekType
    {
        SeekFromHere = 0,
        SeekFromStart,
        SeekFromEnd
    };

    static void __cdecl SetBlockCallback(void(__cdecl* pFunc)(void));

    PblFile();
    ~PblFile();

    bool Open(const char* pcFileName, bool bAsynchronous, bool bOSBuffer, bool bBuffer);

    bool OpenCreate(const char* pcFileName, bool bAsynchronous, bool bOSBuffer, bool bBuffer);
    void Close();

    bool IsOpen() const { return _bIsOpen; }

    static void __cdecl Delete(const char* pcFileName);
    static bool __cdecl Exists(const char* pcFileName);

    int GetSize();
    int Tell() const;
    void Seek(int iOffset, SeekType eFrom);

    void SeekAlign(int iAlign)
    {
        int iPad = -Tell() & (iAlign - 1);
        if (iPad)
        {
            Seek(iPad, SeekFromHere);
        }
    }

    char Read8();
    short Read16();
    int Read32();
    float ReadFloat();
    void ReadData(void* pBuffer, int iNumBytes);

    void WriteData(void* pBuffer, int iNumBytes);
    void WriteString(const char* pcText);

protected:

    unsigned int MemGetSize() const;
    void MemRead(void* pBuffer, int iNumBytes);
    void MemWrite(void* pBuffer, int iNumBytes);
    unsigned int MemTell() const;
    void MemSeek(int iOffset, SeekType eFrom);

    bool MemOpenFile(void* pBuffer, unsigned int uiSize);
    void MemCloseFile();

public:

    bool OpenReadWrite(const char* pcFileName, bool bAsynchronous, bool bOSBuffer, bool bBuffer);
    void Write8(char cValue);
    void Write16(short sValue);
    void Write32(int iValue);
    void WriteFloat(float fValue);
    void Flush();
    void Suspend();
    void Resume(const char* pcFileName);
    bool OpenMemFile(void* pBuffer, unsigned int uiSize);
    void CloseMemFile();
    static int __cdecl GetSize(const char* pcFileName);

    char* _Buffer;
    unsigned int _iBufferSize;
    unsigned int _iPos;
    bool _bIsOpen;
    bool _bAsynchronous;
    bool _bOSBuffer;
    bool _bBuffer;
    bool _bIsMemFile;

    static void(__cdecl* _blockCallbackFunc)(void);
};

inline unsigned int PblFile::MemGetSize() const
{
    return _iBufferSize;
}

inline void PblFile::MemRead(void* pBuffer, int iNumBytes)
{
    if (_iPos + iNumBytes > _iBufferSize)
    {
        iNumBytes = _iBufferSize - _iPos;
    }
    if (iNumBytes != 0)
    {
        memcpy(pBuffer, _Buffer + _iPos, iNumBytes);
        _iPos += iNumBytes;
    }
}

inline void PblFile::MemWrite(void* pBuffer, int iNumBytes)
{
    if (_iPos + iNumBytes > _iBufferSize)
    {
        iNumBytes = _iBufferSize - _iPos;
    }
    if (iNumBytes != 0)
    {
        memcpy(_Buffer + _iPos, pBuffer, iNumBytes);
        _iPos += iNumBytes;
    }
}

inline unsigned int PblFile::MemTell() const
{
    return _iPos;
}

inline void PblFile::MemSeek(int iOffset, SeekType eFrom)
{
    switch (eFrom)
    {
    case SeekFromHere:
        _iPos += iOffset;
        break;
    case SeekFromStart:
        _iPos = iOffset;
        break;
    case SeekFromEnd:
        _iPos = _iBufferSize - iOffset;
        break;
    }
}
