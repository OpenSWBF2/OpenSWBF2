#pragma once

#include <math.h>

inline const float Value(float f)
{
    return f < 0.0f ? -f : f;
}

inline const float Interpolate(float f0, float f1, float fT)
{
    return (f1 - f0) * fT + f0;
}

namespace PblMath
{
    static const float fPi = 3.141592654f;
    static const float fTwoPi = fPi * 2.0f;
    static const float fHalfPi = fPi * 0.5f;

    static const float fOneOverPi = 1.0f / fPi;

    static const float fOneOverTwoPi = 1.0f / fTwoPi;

    static const float fOneOverHalfPi = 1.0f / fHalfPi;

    static const float fLength2EstAlpha = 0.94805944f;
    static const float fLength2EstBeta = 0.39269909f;

    inline float Length2Est(float fX, float fY)
    {
        fX = (float)fabs((double)fX);
        fY = (float)fabs((double)fY);
        return fX > fY ? fX * fLength2EstAlpha + fY * fLength2EstBeta
                       : fX * fLength2EstBeta + fY * fLength2EstAlpha;
    }

    inline float Length3Est(float fX, float fY, float fZ)
    {
        return Length2Est(Length2Est(fX, fY), fZ);
    }

    float Sqrt(float value);

    float __cdecl Atan2(float y, float x);
    float __cdecl Sin(float fAngle);
    float __cdecl Cos(float fAngle);
    void __cdecl SinCos(float* pSin, float* pCos, float fAngle);

    inline float Sign(float f) { return f < 0.0f ? -1.0f : 1.0f; }

    inline int Ceiling(float f) { return (int)(f - 4194304.0f) + 4194304; }

    inline int Floor(float f) { return (int)(f + 4194304.0f) - 4194304; }
    inline int Round(float f) { return Floor(f + 0.5f); }
}

// FUNCTION: SWBF2 0x00414900
inline const int Clamp(int iValue, int iMin, int iMax)
{
    return iValue < iMin ? iMin : (iValue > iMax ? iMax : iValue);
}

// FUNCTION: SWBF2 0x0040f720
inline const float Clamp(float fValue, float fLow, float fHigh)
{
    return fLow > fValue ? fLow : (fValue > fHigh ? fHigh : fValue);
}

// FUNCTION: SWBF2 0x00438a30
inline const int Min(int a, int b)
{
    return a < b ? a : b;
}

// FUNCTION: SWBF2 0x0042ced0
inline const int Max(int iA, int iB)
{
    return iA > iB ? iA : iB;
}

inline const float Min(float a, float b)
{
    return a < b ? a : b;
}

// FUNCTION: SWBF2 0x0041fa40
inline const float Max(float fA, float fB)
{
    return fA > fB ? fA : fB;
}

#include "PebbleFL/pc/pcPblMath.h"
