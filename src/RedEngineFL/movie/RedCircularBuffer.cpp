#include "RedCircularBuffer.h"
#include <string.h>

// FUNCTION: SWBF2 0x006f86c0
RedCircularBuffer::RedCircularBuffer()
{
    mBuffer = 0;
    mPos = 0;
    mUsed = 0;
    mSize = 0;
}

// FUNCTION: SWBF2 0x006f8730
void* RedCircularBuffer::GetBuffer() const
{
    return mBuffer;
}

// FUNCTION: SWBF2 0x006f87d0
unsigned int RedCircularBuffer::GetUsed() const
{
    return mUsed;
}

// FUNCTION: SWBF2 0x006f88e0
void RedCircularBuffer::UpdateReadPos(unsigned int uiSize)
{
    mUsed -= uiSize;
    mPos += uiSize;
    if (mPos >= mSize)
    {
        mPos -= mSize;
    }
}

// FUNCTION: SWBF2 0x006f8800
unsigned int RedCircularBuffer::ReadData(void* pDest, unsigned int uiSize, bool bUpdate)
{
    unsigned int total = 0;
    unsigned int used = GetUsed();
    if (uiSize > used)
    {
        uiSize = used;
    }
    unsigned int pos = mPos;
    unsigned int first = mSize - pos;
    if (first > uiSize)
    {
        first = uiSize;
    }
    if (first)
    {
        memcpy(pDest, mBuffer + pos, first);
        if (bUpdate)
        {
            UpdateReadPos(first);
        }
        pos += first;
        if (pos == mSize)
        {
            pos = 0;
        }
        uiSize -= first;
        total = first;
    }
    unsigned int second = mSize - pos;
    if (second > uiSize)
    {
        second = uiSize;
    }
    if (second)
    {
        memcpy((unsigned char*)pDest + first, mBuffer + pos, second);
        if (bUpdate)
        {
            UpdateReadPos(second);
        }
    }
    return total + second;
}

// FUNCTION: SWBF2 0x006f86e0
RedCircularBuffer::~RedCircularBuffer()
{
}

// FUNCTION: SWBF2 0x006f8720
void RedCircularBuffer::Flush()
{
    mPos = 0;
    mUsed = 0;
}

// FUNCTION: SWBF2 0x006f88c0
void RedCircularBuffer::SetBuffer(void* pBuffer, unsigned int uiSize)
{
    mBuffer = (unsigned char*)pBuffer;
    mSize = uiSize;
    mUsed = 0;
    mPos = 0;
}

// FUNCTION: SWBF2 0x006f86f0
RedCircularBuffer& RedCircularBuffer::operator=(const RedCircularBuffer& other)
{
    mBuffer = other.mBuffer;
    mPos = other.mPos;
    mUsed = other.mUsed;
    mSize = other.mSize;
    return *this;
}

// FUNCTION: SWBF2 0x006f8740
unsigned int RedCircularBuffer::GetFree() const
{
    return mSize - mUsed;
}

// FUNCTION: SWBF2 0x006f8750
unsigned int RedCircularBuffer::GetFreeAligned(unsigned int uiAlign) const
{
    unsigned int uiRead = uiAlign * (mPos / uiAlign);
    unsigned int uiWrite = uiAlign * ((uiAlign + GetWriteOffset() - 1) / uiAlign);
    unsigned int uiDistance = uiWrite >= uiRead ? uiWrite - uiRead : uiWrite + (mSize - uiRead);
    if (mSize > uiDistance)
    {
        return mSize - uiDistance;
    }
    return 0;
}

// FUNCTION: SWBF2 0x006f87a0
unsigned int RedCircularBuffer::GetFreeBlock() const
{
    unsigned int uiWrite = GetWriteOffset();
    if (!GetFree())
    {
        return 0;
    }
    return (mPos <= uiWrite ? mSize : mPos) - uiWrite;
}

// FUNCTION: SWBF2 0x006f87e0
unsigned int RedCircularBuffer::GetWriteOffset() const
{
    unsigned int uiWrite = mPos + mUsed;
    if (uiWrite >= mSize)
    {
        uiWrite -= mSize;
    }
    return uiWrite;
}

// FUNCTION: SWBF2 0x006f87f0
void* RedCircularBuffer::GetWritePos() const
{
    return mBuffer + GetWriteOffset();
}

// FUNCTION: SWBF2 0x006f8900
void RedCircularBuffer::UpdateWritePos(unsigned int uiSize)
{
    mUsed += uiSize;
}

