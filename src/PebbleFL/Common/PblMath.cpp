#include <math.h>
#include "PblMath.h"
#include "PblQuaternion.h"
#include "PblVector.h"

// GLOBAL: SWBF2 0x00785af0
// __xmm@3b490fdb3b490fdb3b490fdb3b490fdb

#define TABLE_SIZE 2048
#define TABLE_MASK (TABLE_SIZE - 1)

class SinCosTable
{
public:
    SinCosTable();

    struct SinCos
    {
        float fSin;
        float fCos;
    };

    SinCos _Data[TABLE_SIZE];
};
 

// FUNCTION: SWBF2 0x00727ad0
SinCosTable::SinCosTable()
{
    int i;
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        const double fAngle = (float)i * (3.141592654f * 2.0f / (float)TABLE_SIZE);
        _Data[i].fSin = ::sin(fAngle);
        _Data[i].fCos = ::cos(fAngle);
    }
}

// SYNTHETIC: SWBF2 0x0040aea0
// `dynamic initializer for '_TheSinCosTable''

// GLOBAL: SWBF2 0x009caf38
// _TheSinCosTable
SinCosTable _TheSinCosTable;

// FUNCTION: SWBF2 0x00727b70
float PblMath::Atan2(float y, float x)
{
    return atan2f(y, x);
}

// FUNCTION: SWBF2 0x00727c20
float PblMath::Sin(float fAngle)
{
    const float fIndex = fAngle * ((float)TABLE_SIZE / PblMath::fTwoPi);
    const int i = (int)fIndex;
    const float fFrac = fIndex - (float)i;
    const int i0 = i & TABLE_MASK;
    const int i1 = (i0 + 1) & TABLE_MASK;

    return _TheSinCosTable._Data[i0].fSin * (1.0f - fFrac)
         + _TheSinCosTable._Data[i1].fSin * fFrac;
}

// FUNCTION: SWBF2 0x00727bb0
float PblMath::Cos(float fAngle)
{
    const float fIndex = fAngle * ((float)TABLE_SIZE / PblMath::fTwoPi);
    const int i = (int)fIndex;
    const float fFrac = fIndex - (float)i;
    const int i0 = i & TABLE_MASK;
    const int i1 = (i0 + 1) & TABLE_MASK;

    return _TheSinCosTable._Data[i0].fCos * (1.0f - fFrac)
         + _TheSinCosTable._Data[i1].fCos * fFrac;
}

// FUNCTION: SWBF2 0x00727c90
void PblMath::SinCos(float* pSin, float* pCos, float fAngle)
{
    const float fIndex = fAngle * ((float)TABLE_SIZE / PblMath::fTwoPi);
    const int i = (int)fIndex;
    const float fFrac = fIndex - (float)i;
    const int i0 = i & TABLE_MASK;
    const int i1 = (i0 + 1) & TABLE_MASK;

    *pSin = _TheSinCosTable._Data[i0].fSin * (1.0f - fFrac)
          + _TheSinCosTable._Data[i1].fSin * fFrac;
    *pCos = _TheSinCosTable._Data[i0].fCos * (1.0f - fFrac)
          + _TheSinCosTable._Data[i1].fCos * fFrac;
}



// SYNTHETIC: SWBF2 0x00403f40
// `dynamic initializer for 'sNegativeXRotation80''

// GLOBAL: SWBF2 0x01ec60e0
// sNegativeXRotation80
PblQuaternion sNegativeXRotation80 = PblQuaternion(PblVector3(-1.0f, 0.0f, 0.0f), 1.3962634f);

// SYNTHETIC: SWBF2 0x00407300
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407320
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004074e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407500
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407520
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407540
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407770
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407790
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004078d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004078f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407990
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004079b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004079d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004079f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407a30
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407a50
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407bc0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407be0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407c00
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407c20
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407c40
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407c60
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407c80
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407ca0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407ce0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407d00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407d20
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407d40
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407d80
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407da0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407dc0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407de0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407e00
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407e20
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407e80
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407ea0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407ec0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407ee0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00407f60
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00407f80
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408090
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004080b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408140
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408160
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004081f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408210
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004082a0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004082c0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004082e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408300
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408320
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408340
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408360
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408380
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408bc0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408ba0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408420
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408400
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004084c0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004084a0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408500
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004084e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408540
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408520
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408580
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408560
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004086b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408690
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408710
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004086f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408750
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408730
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408790
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408770
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004088f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004088d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408960
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408940
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408a00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004089e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408a40
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408a20
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408b20
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408b00
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408c00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408be0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408cc0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408ca0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408d00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408ce0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408e30
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408e10
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408eb0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408e90
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00408ef0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00408ed0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409100
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004090e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409200
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004091e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409240
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00409220
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409280
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00409260
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x004092d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004092b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409480
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00409460
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409500
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x004094e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409750
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00409730
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409790
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00409770
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040a730
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040a710
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040a880
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040a860
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040a8c0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040a8a0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040aa40
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040aa20
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040aa80
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040aa60
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040aac0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040aaa0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ab00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040aae0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ab70
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ab50
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040acf0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040acd0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ad30
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ad10
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ad70
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ad50
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040adb0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ad90
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040adf0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040add0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ae30
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ae10
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040aed0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040aeb0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040af20
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040af00
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040afa0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040af80
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040afe0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040afc0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b040
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b020
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b090
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b070
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b0d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b0b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b130
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b110
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b170
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b150
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b1b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b190
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b1f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b1d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b230
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b210
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b270
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b250
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b2b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b290
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b2f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b2d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b330
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b310
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b370
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b350
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b3b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b390
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b430
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b410
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b4a0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b480
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b4e0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b4c0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b5a0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b580
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b5e0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b5c0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b700
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b6e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b790
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b770
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b7d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b7b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040b8b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b890
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ba00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040b9e0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ba40
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ba20
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040ba80
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040ba60
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bac0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040baa0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bb20
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bb00
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bc00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bbe0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bcf0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bcd0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bd30
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bd10
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bd70
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bd50
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bdb0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bd90
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bdf0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bdd0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040be30
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040be10
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040be70
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040be50
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bec0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bea0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bf00
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bee0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bf40
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bf20
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bf80
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bf60
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040bfc0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bfa0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c000
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040bfe0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c040
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c020
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c090
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c070
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c0e0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c0c0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c120
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c100
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c170
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c150
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c1e0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c1c0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c220
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c200
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c260
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c240
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c2a0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c280
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c2e0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c2c0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c320
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c300
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c360
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c340
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c3a0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c380
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c430
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c410
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c470
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c450
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c4b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c490
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c4f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c4d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c530
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c510
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c570
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c550
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c5b0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c590
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c5f0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c5d0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c650
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c630
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c690
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c670
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c6d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c6b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c710
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c6f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c750
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c730
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c790
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c770
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c7d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c7b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c810
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c7f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c850
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c830
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c890
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c870
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c8d0
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c8b0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040c910
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040c8f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''
