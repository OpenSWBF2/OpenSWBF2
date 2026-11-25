#include "harness/stallclock.h"

#include <string.h>

StallSample NewStallSample()
{
    StallSample kSample;
    memset(&kSample, 0, sizeof(kSample));
    return kSample;
}

StallState NewStallState()
{
    StallState kState;
    memset(&kState, 0, sizeof(kState));
    return kState;
}

bool IsStarvedWaitReason(unsigned int uWaitReason)
{
    switch (uWaitReason)
    {
    case 1:
    case 8:
    case 2:
    case 9:
    case 3:
    case 10:
    case 5:
    case 12:
    case 18:
    case 19:
        return true;
    default:
        return false;
    }
}

static double Clamp(double f, double fMax)
{
    if (f < 0.0)
    {
        f = 0.0;
    }
    return f < fMax ? f : fMax;
}

StallState StepStallClock(const StallState& kState, const StallSample& kSample, double fWindowMs)
{
    StallState s = kState;
    double fCpu = 0.0;
    if (kSample.bHasCpu && s.bHasCpu)
    {
        fCpu = Clamp(kSample.fCpuMs - s.fCpuMs, kSample.fIntervalMs);
    }
    s.bHasCpu = kSample.bHasCpu;
    s.fCpuMs = kSample.fCpuMs;
    double fProc = 0.0;
    if (kSample.bHasProcCpu && s.bHasProcCpu)
    {
        fProc = Clamp(kSample.fProcCpuMs - s.fProcCpuMs, kSample.fIntervalMs);
    }
    s.bHasProcCpu = kSample.bHasProcCpu;
    s.fProcCpuMs = kSample.fProcCpuMs;
    if (kSample.bHasProgress &&
        (!s.bHasProgress || kSample.uTurns != s.uTurns || kSample.uSteps != s.uSteps))
    {
        s.bHasProgress = true;
        s.uTurns = kSample.uTurns;
        s.uSteps = kSample.uSteps;
        s.bSeen = true;
        s.fSilenceMs = 0.0;
        return s;
    }
    double fAdd = fCpu;
    if (kSample.bWaiting && !IsStarvedWaitReason(kSample.uWaitReason))
    {
        fAdd = kSample.bOtherReady ? fProc : kSample.fIntervalMs;
    }
    s.fSilenceMs += fAdd;
    if (s.fSilenceMs > s.fSilentMaxMs)
    {
        s.fSilentMaxMs = s.fSilenceMs;
    }
    if (s.fSilenceMs >= fWindowMs)
    {
        if (s.bStalled || !s.bSeen)
        {
            s.bKill = true;
        }
        s.bStalled = true;
        s.fSilenceMs = 0.0;
    }
    return s;
}
