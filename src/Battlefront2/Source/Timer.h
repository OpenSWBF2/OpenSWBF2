#pragma once

#include "Thread.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x007af5c8 Thread
// VTABLE: SWBF2 0x007af5e0 PblHandled
class Timer : public Thread, public PblHandled
{
public:

    Timer(const char* pcName);
    virtual ~Timer();
    virtual bool Update(float fDeltaTime);

    const char* GetName();

    // SYNTHETIC: SWBF2 0x0058aa80
    // Timer::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    // FUNCTION: SWBF2 0x0058aa70
    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }

    // FUNCTION: SWBF2 0x0058aab0
    void SetRate(float fRate) { mRate = fRate; }

    // FUNCTION: SWBF2 0x0058aa90
    void SetTime(float fTime) { mTime = fTime; }

    // FUNCTION: SWBF2 0x0055e320
    float GetTime() const { return mTime; }

    // FUNCTION: SWBF2 0x0058aaa0
    float GetRate() const { return mRate; }

    bool IsRunning() const;

    void Start();
    void Stop();

    static void Clear();

    static Timer* Find(const char* pcName);

    unsigned int mId;
    float mTime;
protected:
    float mRate;
public:
    static PblHashTable<Timer, 64> sMap;
    static MemoryPool sMemoryPool;
};
