#include <windows.h>

#include "sndthread.h"

namespace Snd
{

// FUNCTION: SWBF2 0x007451d0
bool Thread::Start(void* pArgs)
{
    DWORD id;

    mArgs = pArgs;

    HANDLE hThread = CreateThread(0, mStackSize, ThreadEntry, this, 0, &id);
    if (!hThread)
    {
        return false;
    }

    unsigned int uiPriority = mPriority;
    if (uiPriority > 0xFFFFFFFB)
    {
        uiPriority = 0xFFFFFFFB;
    }

    int iNativePriority;
    switch (uiPriority / (0xFFFFFFFFu / 7))
    {
    case 0:
        iNativePriority = THREAD_PRIORITY_TIME_CRITICAL;
        break;
    case 1:
        iNativePriority = THREAD_PRIORITY_HIGHEST;
        break;
    case 2:
        iNativePriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case 3:
        iNativePriority = THREAD_PRIORITY_NORMAL;
        break;
    case 4:
        iNativePriority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case 5:
        iNativePriority = THREAD_PRIORITY_LOWEST;
        break;
    default:
        iNativePriority = THREAD_PRIORITY_IDLE;
        break;
    }

    SetThreadPriority(hThread, iNativePriority);
    return true;
}

// FUNCTION: SWBF2 0x007452d0
DWORD WINAPI ThreadBase::ThreadEntry(LPVOID pArg)
{
    ThreadBase* pThread = (ThreadBase*)pArg;
    pThread->mThreadFunc(pThread->mArgs);
    return 0;
}

}
