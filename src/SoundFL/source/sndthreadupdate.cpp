#include <windows.h>
#include <stdlib.h>

#include "sndthreadupdate.h"

#include "sndglobals.h"

namespace pcCrash { void __cdecl Exit(int); }

namespace Snd
{

// FUNCTION: SWBF2 0x0073c210
ThreadUpdate::Object::Object()
{
    mUpdate = 0;
    mDestructing = 0;
}

// FUNCTION: SWBF2 0x0073c290
ThreadUpdate::Object::~Object()
{
    if (mUpdate)
    {
        mUpdate->RemoveObject(this);
    }

}

// SYNTHETIC: SWBF2 0x0073c350
// Snd::ThreadUpdate::Object::`scalar deleting destructor'

// FUNCTION: SWBF2 0x0073c4e0
ThreadUpdate* ThreadUpdate::Object::GetThreadUpdate() const
{
    return mUpdate;
}

// FUNCTION: SWBF2 0x0073c230
ThreadUpdate::ThreadUpdate(unsigned int uiPriority, unsigned int uiStackSize)
    : mThread(UpdateObjects, uiPriority, uiStackSize)
{

    mSleepTime = 10;
    mUpdating = false;
    mStopUpdating = false;
    mPausing = false;
    mCompleteCallback = Complete;
    mCompleteCallbackData = 0;
}

// FUNCTION: SWBF2 0x0073c300
ThreadUpdate::~ThreadUpdate()
{
}

// FUNCTION: SWBF2 0x0073c3e0
void ThreadUpdate::AddObject(Object* pObject)
{
    if (mAccess.IsValid())
    {
        mAccess.Wait();
        mUpdating = false;
        mStopUpdating = false;
    }

    if (mUpdateList.IsEmpty())
    {
        mUpdating = false;

        if (!mAccess.Create(0, 1))
        {
            PrintDebugMessage("Unable to create semaphore : out of system resources!");
            pcCrash::Exit(1);
        }

        if (!mThread.Start(this))
        {
            PrintDebugMessage("Unable to start thread : out of system resources!");
            pcCrash::Exit(1);
        }
    }

    mUpdateList.AddLast(&pObject->mNode);
    pObject->mUpdate = this;

    if (mAccess.IsValid())
    {
        mPausing = false;
        mUpdating = true;
        mAccess.Signal();
    }
}

// FUNCTION: SWBF2 0x0073c530
void ThreadUpdate::RemoveObject(Object* pObject)
{
    if (mAccess.IsValid())
    {
        mAccess.Wait();
        mUpdating = false;
        mStopUpdating = false;
    }

    pObject->mNode.Remove();
    pObject->mUpdate = 0;

    if (mUpdateList.IsEmpty())
    {
        Semaphore done;
        if (!done.Create(0, 1))
        {
            PrintDebugMessage("Unable to create semaphore out of system resources!");
            pcCrash::Exit(1);
        }

        mCompleteCallbackData = &done;

        if (mAccess.IsValid())
        {
            mPausing = false;
            mUpdating = true;
            mAccess.Signal();
        }

        done.Wait();
        done.Destroy();
        mCompleteCallbackData = 0;
    }
    else if (mAccess.IsValid())
    {
        mPausing = false;
        mUpdating = true;
        mAccess.Signal();
    }
}

// FUNCTION: SWBF2 0x0073c4b0
void ThreadUpdate::Exit()
{
    PblListSingle::Node* pNode = mUpdateList.mHead.GetNext();
    while (pNode != &mUpdateList.mHead)
    {

        RemoveObject((Object*)pNode->GetObjectA(&mUpdateList, offsetof(Object, mNode)));
        pNode = mUpdateList.mHead.GetNext();
    }
}

// FUNCTION: SWBF2 0x0073c490
void __cdecl ThreadUpdate::Complete(ThreadUpdate* pThis, void* pData)
{
    (void)pThis;

    Semaphore* pDone = (Semaphore*)pData;
    if (pDone)
    {
        pDone->Signal();
    }
}

// FUNCTION: SWBF2 0x0073c4f0
void ThreadUpdate::Pause(bool bDeferred)
{
    if (!mAccess.IsValid())
    {
        return;
    }

    if (bDeferred)
    {
        mPausing = true;
        mStopUpdating = true;
        return;
    }

    mAccess.Wait();
    mUpdating = false;
    mStopUpdating = false;
}

// FUNCTION: SWBF2 0x0073c670
void ThreadUpdate::Resume()
{
    if (!mAccess.IsValid())
    {
        return;
    }

    mPausing = false;
    mUpdating = true;
    mAccess.Signal();
}

// FUNCTION: SWBF2 0x0073c6a0
void ThreadUpdate::SetSleepTimeMs(unsigned int uiSleepTimeMs)
{
    mSleepTime = uiSleepTimeMs;
}

// FUNCTION: SWBF2 0x0073c6b0
void __cdecl ThreadUpdate::UpdateObjects(void* pArg)
{
    ThreadUpdate* pThis = (ThreadUpdate*)pArg;
    int iObjects = -1;

    do
    {
        if (pThis->mUpdating)
        {
            pThis->mAccess.Wait();

            iObjects = 0;
            for (PblListSingle::Itor it(pThis->mUpdateList); it.GetNode() != pThis->mUpdateList.GetTerminator(); ++it)
            {
                Object* pObject = (Object*)it.GetNode()->GetObjectA(&pThis->mUpdateList, offsetof(Object, mNode));
                if (!pObject->mDestructing)
                {
                    pObject->UpdateObject();
                }
                ++iObjects;
            }

            if (pThis->mStopUpdating)
            {
                pThis->mStopUpdating = false;
                pThis->mUpdating = false;
            }

            pThis->mAccess.Signal();
        }

        Sleep(pThis->mSleepTime);
    } while (iObjects != 0);

    if (pThis->mAccess.IsValid())
    {
        pThis->mAccess.Destroy();
    }

    if (pThis->mCompleteCallback)
    {
        pThis->mCompleteCallback(pThis, pThis->mCompleteCallbackData);
    }
}

}
