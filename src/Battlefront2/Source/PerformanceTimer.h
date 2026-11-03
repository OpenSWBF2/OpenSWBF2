#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// VTABLE: SWBF2 0x0079f7fc
class PblPerformanceTimer
{
public:
    PblPerformanceTimer() { Reset(); }

    // SYNTHETIC: SWBF2 0x00530e70
    // PblPerformanceTimer::`scalar deleting destructor'
    virtual ~PblPerformanceTimer() {}

    static void Init()
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        _fToSeconds = 1.0 / (double)frequency.QuadPart;
    }

    void Reset() { _iAccum = 0; }

    // FUNCTION: SWBF2 0x00530e10
    virtual void Start() { _iValue = _GetCycleCounter(); }

    // FUNCTION: SWBF2 0x00530e40
    virtual void Stop() { _iAccum += _GetCycleCounter() - _iValue; }

    unsigned __int64 GetCount() const;

    float GetTime() const
    {
        return (float)((double)(_GetCycleCounter() - _iValue + _iAccum) * _fToSeconds);
    }

    static float GetSystemTime()
    {
        return (float)((double)_GetCycleCounter() * _fToSeconds);
    }

    static unsigned __int64 _GetCycleCounter()
    {
        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        return count.QuadPart;
    }

private:
    unsigned __int64 _iValue;
    unsigned __int64 _iAccum;
    static double _fToSeconds;
};
