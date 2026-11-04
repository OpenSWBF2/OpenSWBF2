#include "PblTimeBase.h"

#include <windows.h>

// GLOBAL: SWBF2 0x007e2c60
// PblTimeBase::TICK_SHIFT
int PblTimeBase::TICK_SHIFT = 12;
// GLOBAL: SWBF2 0x009caf20
// PblTimeBase::_iTicksPerSecond
unsigned __int64 PblTimeBase::_iTicksPerSecond;
// GLOBAL: SWBF2 0x009caf28
// PblTimeBase::_iTicksPerFrame
unsigned __int64 PblTimeBase::_iTicksPerFrame;
// GLOBAL: SWBF2 0x009caf30
// PblTimeBase::_fSecondsPerTick
float PblTimeBase::_fSecondsPerTick;

// FUNCTION: SWBF2 0x007278d0
PblTimeBase::PblTimeBase()
{
    _iValue = 0;

    if (_iTicksPerSecond == 0)
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);

        _iTicksPerSecond = Frequency.QuadPart;
        TICK_SHIFT = 0;
        while (_iTicksPerSecond > 12000)
        {
            _iTicksPerSecond >>= 1;
            TICK_SHIFT++;
        }

        ComputeConstants();
    }

    SetRate(1.0f);
}

// FUNCTION: SWBF2 0x00727970
void PblTimeBase::AdvanceTicks(unsigned __int64 iTicks)
{
    if (_fRate == 1.0f)
    {
        _iValue += iTicks;
    }
    else
    {
        _iValue += (unsigned __int64)((float)iTicks * _fRate);
    }
}

// FUNCTION: SWBF2 0x007279c0
void PblTimeBase::AdvanceTime(float fSeconds)
{
    _iValue += (unsigned __int64)((float)(int)(_iTicksPerSecond << TICK_SHIFT) * (_fRate * fSeconds));
}

// FUNCTION: SWBF2 0x00727a10
void PblTimeBase::ComputeConstants()
{
    _iTicksPerFrame = _iTicksPerSecond / FRAMES_PER_SECOND;
    _fSecondsPerTick = 1.0f / (float)_iTicksPerSecond;
}

// FUNCTION: SWBF2 0x00727a60
void PblTimeBase::PblJournal_Record()
{
}

// FUNCTION: SWBF2 0x00727a70
void PblTimeBase::PblJournal_RecordInit()
{
    ComputeConstants();
}

// FUNCTION: SWBF2 0x00727a80
void PblTimeBase::SetRate(float fRate)
{
    _fRate = fRate;
    _iIncrement = (unsigned __int64)((float)_iTicksPerFrame * fRate) << TICK_SHIFT;
}
