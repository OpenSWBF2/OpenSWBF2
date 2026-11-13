#pragma once

#include <stddef.h>
#include "PebbleFL/Common/PblList.h"

class Thread;

enum ThreadType
{
    PER_FRAME = 0,
    PER_LOCAL_TURN = 1,
    PER_TURN = 2
};

// VTABLE: SWBF2 0x007af5a8
class Thread
{
public:
    Thread();

    virtual ~Thread();
    virtual bool Update(float fDeltaTime) = 0;
    virtual void ActivateThread(int iOrder, ThreadType eThreadType);
    virtual void DeactivateThread();
    // FUNCTION: SWBF2 0x00429200
    virtual bool IsThreadActive() const { return mThreadListNode._pList != 0; }

    typedef PblOrderedList<Thread, int>::Node ThreadListNode;
    typedef PblOrderedList<Thread, int> ThreadList;

    ThreadListNode mThreadListNode;

    static ThreadList::Itor sExecutingThread;

    static void EnterClient();
    static void LeaveClient();
    static void Reset();

    static void UpdateAll(float fDeltaTime, bool bFrame, bool bLocalTurn);
};
 
 
 
 
 
 
 
 
 

class ThreadGroup
{
public:
    ThreadGroup(int iPriority);
    ~ThreadGroup();

    ThreadGroup* FindCreate(int iPriority);
    void AddBefore(ThreadGroup* pNext);
    void Remove();
    void Clear();
    void Update(float fDeltaTime);
    void UpdateAll(float fDeltaTime);

    int mPriority;
    ThreadGroup* mNext;
    ThreadGroup* mPrev;
    PblOrderedList<Thread, int> mThreads;
};
 
 
