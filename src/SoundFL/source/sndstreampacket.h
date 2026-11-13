#pragma once

namespace Snd
{

class StreamPacket
{
public:
    enum Flags
    {
        flags_lastPacket = 1,
        flags_discontinuity = 2
    };
    typedef void (__cdecl* CompleteCallback)(StreamPacket* const, void* const);

    // FUNCTION: SWBF2 0x0053c610
    StreamPacket()
        : mPos(0),
          mBuffer(0),
          mBufferUsed(0),
          mRefCount(0),
          mFlags(0),
          mCallback(0),
          mCallbackData(0)
    {
    }

    // FUNCTION: SWBF2 0x0053c640
    ~StreamPacket() {}

    void Lock() { ++mRefCount; }

    // FUNCTION: SWBF2 0x0053d330
    void Unlock()
    {
        if (mRefCount != 0)
        {
            --mRefCount;
        }

        if (0 == mRefCount)
        {
            if (mCallback != 0)
            {
                mCallback(this, mCallbackData);
            }

            mBufferUsed = 0;
            mFlags = 0;
        }
    }

    bool IsLocked() const { return mRefCount > 0; }

    void SetUsed(unsigned int uiUsed) { mBufferUsed = uiUsed; }
    unsigned int GetUsed() const { return mBufferUsed; }
    void SetPosition(unsigned int uiPosition) { mPos = uiPosition; }
    unsigned int GetPosition() const { return mPos; }
    void SetBuffer(void* pBuffer) { mBuffer = pBuffer; }
    void* GetBuffer() const { return mBuffer; }

    void SetLast(bool bLast)
    {
        if (bLast)
        {
            mFlags |= 1;
        }
        else
        {
            mFlags &= 0xFFFE;
        }
    }

    bool GetLast() const { return (mFlags & 1) != 0; }

    // FUNCTION: SWBF2 0x0073caf0
    void SetDiscontinuity(bool bDiscontinuity)
    {
        if (bDiscontinuity)
        {
            mFlags |= 2;
        }
        else
        {
            mFlags &= 0xFFFD;
        }
    }

    bool GetDiscontinuity() const { return ((mFlags >> 1) & 1) != 0; }

    void SetCallback(CompleteCallback pCallback, void* pCallbackData)
    {
        mCallback = pCallback;
        mCallbackData = pCallbackData;
    }

    CompleteCallback GetCallback() const { return mCallback; }

    void* GetCallbackData() const { return mCallbackData; }

private:
    unsigned int mPos;
    void* mBuffer;
    unsigned int mBufferUsed;
    unsigned short mRefCount;
    unsigned short mFlags;
    CompleteCallback mCallback;
    void* mCallbackData;
};

 

}
