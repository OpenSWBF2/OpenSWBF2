#pragma once

#include <math.h>
#include <string.h>

extern "C"
{
double __cdecl PcCrtSinX64(double d);
double __cdecl PcCrtCosX64(double d);
double __cdecl PcCrtTanX64(double d);
}

inline double PblCrtSin(double d)
{
    if constexpr (sizeof(void*) == 8)
    {
        return PcCrtSinX64(d);
    }
    else
    {
        return sin(d);
    }
}

inline double PblCrtCos(double d)
{
    if constexpr (sizeof(void*) == 8)
    {
        return PcCrtCosX64(d);
    }
    else
    {
        return cos(d);
    }
}

inline double PblCrtTan(double d)
{
    if constexpr (sizeof(void*) == 8)
    {
        return PcCrtTanX64(d);
    }
    else
    {
        return tan(d);
    }
}

inline unsigned int PblCrtBitsOfFloat(float f)
{
    unsigned int u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

inline float PblCrtFloatOfBits(unsigned int u)
{
    float f;
    memcpy(&f, &u, sizeof(f));
    return f;
}

inline bool PblCrtIsNanBits(unsigned int u)
{
    return (u & 0x7FFFFFFFu) > 0x7F800000u;
}

inline unsigned int PblCrtX87NanPair(unsigned int uX, unsigned int uY)
{
    const bool bNanX = PblCrtIsNanBits(uX);
    const bool bNanY = PblCrtIsNanBits(uY);
    uX |= 0x00400000u;
    uY |= 0x00400000u;
    if (!bNanY)
    {
        return uX;
    }
    if (!bNanX)
    {
        return uY;
    }
    const unsigned int uSigX = uX & 0x007FFFFFu;
    const unsigned int uSigY = uY & 0x007FFFFFu;
    return uSigX > uSigY ? uX : (uSigY > uSigX ? uY : (uX & uY));
}

inline float PblCrtSinF(float f)
{
    if constexpr (sizeof(void*) == 8)
    {
        return (float)PcCrtSinX64((double)f);
    }
    else
    {
        return (float)sin((double)f);
    }
}

inline float PblCrtCosF(float f)
{
    if constexpr (sizeof(void*) == 8)
    {
        return (float)PcCrtCosX64((double)f);
    }
    else
    {
        return (float)cos((double)f);
    }
}

inline float PblCrtTanF(float f)
{
    if constexpr (sizeof(void*) == 8)
    {
        return (float)PcCrtTanX64((double)f);
    }
    else
    {
        return (float)tan((double)f);
    }
}

inline float PblCrtAsinF(float f)
{
    return (float)asin((double)f);
}

inline float PblCrtAcosF(float f)
{
    return (float)acos((double)f);
}

inline float PblCrtAtan2F(float fY, float fX)
{
    if constexpr (sizeof(void*) == 8)
    {
        const unsigned int uY = PblCrtBitsOfFloat(fY);
        const unsigned int uX = PblCrtBitsOfFloat(fX);
        if (PblCrtIsNanBits(uY) || PblCrtIsNanBits(uX))
        {
            return PblCrtFloatOfBits(PblCrtX87NanPair(uY, uX));
        }
        return (float)atan2((double)fY, (double)fX);
    }
    else
    {
        return (float)atan2((double)fY, (double)fX);
    }
}

inline float PblCrtAtanF(float f)
{
    return (float)atan((double)f);
}

inline float PblCrtLogF(float f)
{
    return (float)log((double)f);
}

inline float PblCrtLog10F(float f)
{
    return (float)log10((double)f);
}

inline float PblCrtPowF(float fX, float fY)
{
    if constexpr (sizeof(void*) == 8)
    {
        const unsigned int uX = PblCrtBitsOfFloat(fX);
        if (PblCrtIsNanBits(uX) && (PblCrtBitsOfFloat(fY) & 0x7FFFFFFFu) != 0)
        {
            return PblCrtFloatOfBits(uX | 0x00400000u);
        }
        return (float)pow((double)fX, (double)fY);
    }
    else
    {
        return (float)pow((double)fX, (double)fY);
    }
}

inline float PblCrtExpF(float f)
{
    return (float)exp((double)f);
}

inline float PblCrtFmodF(float fX, float fY)
{
    if constexpr (sizeof(void*) == 8)
    {
        const unsigned int uX = PblCrtBitsOfFloat(fX);
        const unsigned int uY = PblCrtBitsOfFloat(fY);
        if (PblCrtIsNanBits(uX) || PblCrtIsNanBits(uY))
        {
            return PblCrtFloatOfBits(PblCrtX87NanPair(uX, uY));
        }
        const float fOut = (float)fmod((double)fX, (double)fY);
        if (PblCrtIsNanBits(PblCrtBitsOfFloat(fOut)))
        {
            return PblCrtFloatOfBits(0xFFC00000u);
        }
        return fOut;
    }
    else
    {
        return (float)fmod((double)fX, (double)fY);
    }
}
