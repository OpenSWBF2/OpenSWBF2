#pragma once

#include "PblVector.h"

class PblRandom
{
public:

    // FUNCTION: SWBF2 0x0053e440
    PblRandom() : _uiSeed(GetGlobalInt() ^ 0xC71C2830u) {}
    PblRandom(unsigned int uiSeed) : _uiSeed(uiSeed) {}

    // FUNCTION: SWBF2 0x00428e80
    unsigned int GetInt()
    {
        _uiSeed *= 0x19660Du;
        _uiSeed += 0x3C6EF35Fu;
        unsigned int uiReturn = _uiSeed >> 16;
        _uiSeed *= 0x19660Du;
        _uiSeed += 0x3C6EF35Fu;
        uiReturn |= _uiSeed & 0xFFFF0000u;
        return uiReturn;
    }

    // FUNCTION: SWBF2 0x00448950
    static unsigned int GetGlobalInt() { return _Global.GetInt(); }

    static float GetGlobalFloat();

    // FUNCTION: SWBF2 0x00428eb0
    float GetFloat()
    {
        union
        {
            unsigned int u;
            float f;
        } bits;
        bits.u = (GetInt() >> 9) | 0x3F800000u;
        return bits.f - 1.0f;
    }

    float GetFloatRange(float fMid, float fRange) { return (GetFloat() - 0.5f) * fRange + fMid; }

    // FUNCTION: SWBF2 0x004a9c50
    static float GetGlobalFloatRange(float fMid, float fRange) { return _Global.GetFloatRange(fMid, fRange); }

    // FUNCTION: SWBF2 0x0043ec30
    const PblVector3 GetVector3()
    {
        return PblVector3(GetFloat() - 0.5f, GetFloat() - 0.5f, GetFloat() - 0.5f) * 2.0f;
    }

    static const PblVector3 GetGlobalVector3() { return _Global.GetVector3(); }

    // FUNCTION: SWBF2 0x005391a0
    void SetSeed(unsigned int uiSeed) { _uiSeed = uiSeed; }

    // FUNCTION: SWBF2 0x00530de0
    unsigned int GetSeed() { return _uiSeed; }
    static void SetGlobalSeed(unsigned int uiSeed) { _Global.SetSeed(uiSeed); }

    // FUNCTION: SWBF2 0x0043ed30
    static PblRandom& GetSim() { return _Sim; }

    float GetNormal();
    static void GetNormalCache(int* piHas, float* pfValue);
    static void SetNormalCache(int iHas, float fValue);
    float GetNormal(float fMean, float fStdDev) { return GetNormal() * fStdDev + fMean; }

    unsigned int _uiSeed;

    static PblRandom _Global;
    static PblRandom _Sim;
};
 
