#pragma once

#include <stddef.h>

namespace HUD
{
class Envelope
{
public:

    enum Phase
    {
        phase_Inactive = 0,
        phase_Attack = 1,
        phase_Hold = 2,
        phase_Sustain = 3,
        phase_Release = 4,
        phase_MAX = 5
    };

    Envelope();
    Phase Update(float fTime);
    float CalculateValue() const;
    Envelope& operator=(const Envelope& other);

    void Start();
    void Stop();

    Phase GetPhase() const;
    float GetElapsed() const { return mTime; }
    float GetPhaseTime(Phase ePhase) const;

    void SetPhaseTime(Phase ePhase, float fTime);

    float mPhaseTime[phase_MAX];
    float mTime;
    Phase mPhase;
};
}

 
 
