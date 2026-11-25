#pragma once

struct StallSample
{
    bool bHasProgress;
    unsigned int uTurns;
    unsigned int uSteps;
    bool bWaiting;
    unsigned int uWaitReason;
    bool bHasCpu;
    double fCpuMs;
    bool bHasProcCpu;
    double fProcCpuMs;
    bool bOtherReady;
    double fIntervalMs;
};

struct StallState
{
    bool bHasProgress;
    unsigned int uTurns;
    unsigned int uSteps;
    bool bSeen;
    bool bHasCpu;
    double fCpuMs;
    bool bHasProcCpu;
    double fProcCpuMs;
    double fSilenceMs;
    double fSilentMaxMs;
    bool bStalled;
    bool bKill;
};

const double kStallSilenceMs = 60000.0;

StallSample NewStallSample();
StallState NewStallState();
bool IsStarvedWaitReason(unsigned int uWaitReason);
StallState StepStallClock(const StallState& kState, const StallSample& kSample, double fWindowMs);
