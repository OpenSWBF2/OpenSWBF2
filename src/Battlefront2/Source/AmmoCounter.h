#pragma once

#include <float.h>
#include <stddef.h>
#include <float.h>
#include "RedEngineFL/Memory/RedMemoryPool.h"

class NetPktGroup;

#include "RedEngineFL/Memory/RedMemoryPool.h"

class NetPktGroup;

class AmmoCounterClass
{
public:
    AmmoCounterClass()
    {
        mRoundsPerClip = 1;
        mAmmoPerRound = 1.0f;
    }

    bool SetProperty(unsigned int uiHash, const char* pcValue);

    int mRoundsPerClip;
    float mAmmoPerRound;
};

class AmmoCounter
{
public:
    AmmoCounter(const AmmoCounterClass* pClass, int iNumClips);
    ~AmmoCounter();
    AmmoCounter* AddRef();
    unsigned int Release();
    void LoadClip();
    int GetMaxNumRounds() const;
    int GetNumRounds() const;
    void AddClips(float fClips);
    bool CanExpendRounds(float fRounds) const;
    void ExpendRounds(float fRounds);
    void UnloadClip();
    void UnloadClip_Unlimitted();
    void LoadClip_Unlimitted();

    static void* operator new(unsigned int size) { return sMemoryPool.Allocate(size); }
    static void operator delete(void* block) { sMemoryPool.Free(block); }

    void Write(NetPktGroup& group) const;
    void Read(NetPktGroup& group);

    bool HasInfiniteAmmo() const { return mMaxClips == FLT_MAX; }
    bool IsClipEmpty() const { return m_pClass->mAmmoPerRound > mCurClip; }
    bool IsReserveFull() const { return mNumClips >= mMaxClips - 1.0f; }
    float GetMaxClips() const { return mMaxClips; }
    float GetNumClips() const { return mNumClips; }
    static void SetupMemoryPool(unsigned int count, unsigned int size, unsigned int grow)
    {
        sMemoryPool.Setup(count, size, grow);
    }
    static MemoryPool sMemoryPool;

    const AmmoCounterClass* m_pClass;
    float mDefaultMaxClips;
    float mMaxClips;
    float mNumClips;
    float mCurClip;
    unsigned int m_uiRefCount;
};
