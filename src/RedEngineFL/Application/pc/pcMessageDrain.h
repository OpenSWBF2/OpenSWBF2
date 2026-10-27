#pragma once

#include <windows.h>

namespace MessageDrain
{

const unsigned int kMaxMsgsPerPass = 64;
const unsigned int kMaxMsPerPass = 2;

struct DrainOps
{
    bool (*Peek)(void* pCtx, MSG* pMsg);
    void (*Dispatch)(void* pCtx, const MSG* pMsg);
    __int64 (*Now)(void* pCtx);
    void* pCtx;
};

struct DrainResult
{
    unsigned int uiCount;
    bool bQuit;
    bool bHitCount;
    bool bHitTime;
};

DrainResult DrainMessages(const DrainOps& ops, unsigned int uiMaxMsgs, __int64 iMaxTicks);

}
