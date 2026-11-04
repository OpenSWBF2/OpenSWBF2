#pragma once

#include "win/sndsemaphore.h"
#include "win/sndthread.h"
#include "PebbleFL/Common/PblListSingle.h"

namespace Snd
{

class ThreadUpdate
{
public:

    // VTABLE: SWBF2 0x007867b4
    class Object
    {
    public:
        Object();
        virtual ~Object();

        virtual void UpdateObject() = 0;

        ThreadUpdate* GetThreadUpdate() const;

        PblListSingle::Node mNode;
        ThreadUpdate* mUpdate;
        unsigned char mDestructing;
    };

    ThreadUpdate(unsigned int uiPriority, unsigned int uiStackSize);
    ~ThreadUpdate();

    void AddObject(Object* pObject);
    void RemoveObject(Object* pObject);
    void Exit();

    void Pause(bool bDeferred);
    void Resume();
    void SetSleepTimeMs(unsigned int uiSleepTimeMs);

private:

    static void __cdecl Complete(ThreadUpdate* pThis, void* pData);
    static void __cdecl UpdateObjects(void* pArg);

public:
    Thread mThread;
    PblListSingle mUpdateList;
    Semaphore mAccess;
    unsigned int mSleepTime;
    bool mUpdating;
    volatile bool mStopUpdating;
    volatile bool mPausing;
    void(__cdecl* mCompleteCallback)(ThreadUpdate*, void*);
    void* mCompleteCallbackData;
};

 
 

}
