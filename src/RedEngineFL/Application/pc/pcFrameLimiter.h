#pragma once

namespace FrameLimiter
{

enum TimerKind
{
    kTimerNone = 0,
    kTimerPeriod = 1,
    kTimerHires = 2
};

struct FrameOps
{
    __int64 (*Now)(void* pCtx);
    void (*WaitTicks)(void* pCtx, __int64 iTicks);
    void* pCtx;
};

struct FramePlan
{
    __int64 iMinTicks;
    __int64 iMarginTicks;
};

__int64 MinTicksForFps(__int64 iFrequency, unsigned int uiFps);

__int64 Pace(const FrameOps& ops, const FramePlan& plan, __int64 iOld, __int64& iNowOut);

int Init(bool bNoRender);

void Shutdown();

void EndPeriod();

int Kind();

const char* KindName(int iKind);

void BindReal(FrameOps& ops, FramePlan& plan, unsigned int uiFps);

}
