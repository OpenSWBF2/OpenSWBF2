#include "PebbleFL/pc/pcD3dxPort.h"

#include <string.h>
#include <emmintrin.h>

namespace
{
unsigned int FloatBits(float f)
{
    unsigned int uBits;
    memcpy(&uBits, &f, sizeof(uBits));
    return uBits;
}

float BitsFloat(unsigned int uBits)
{
    float f;
    memcpy(&f, &uBits, sizeof(f));
    return f;
}

bool IsNaN(float f)
{
    return (FloatBits(f) & 0x7FFFFFFFu) > 0x7F800000u;
}

float Quiet(float f)
{
    if (IsNaN(f))
    {
        return BitsFloat(FloatBits(f) | 0x00400000u);
    }
    return f;
}

float X87NaN(float fA, float fB, float fResult)
{
    if (!IsNaN(fA) || !IsNaN(fB))
    {
        return fResult;
    }
    const unsigned int uA = FloatBits(fA) | 0x00400000u;
    const unsigned int uB = FloatBits(fB) | 0x00400000u;
    const unsigned int uSigA = uA & 0x007FFFFFu;
    const unsigned int uSigB = uB & 0x007FFFFFu;
    if (uSigA != uSigB)
    {
        return BitsFloat(uSigA > uSigB ? uA : uB);
    }
    return BitsFloat(uA < uB ? uA : uB);
}

float X87Mul(float fA, float fB)
{
    return X87NaN(fA, fB, fA * fB);
}

float X87Add(float fA, float fB)
{
    return X87NaN(fA, fB, fA + fB);
}

float X87Sub(float fA, float fB)
{
    return X87NaN(fA, fB, fA - fB);
}

float X87OneMinusMinus(float fA, float fB)
{
    return X87NaN(fA, fB, (float)((1.0 - (double)fA) - (double)fB));
}

__m128 Splat(float f)
{
    return _mm_set1_ps(f);
}

__m128 FirstNaN(__m128 a, __m128 b, __m128 r)
{
    const __m128 both = _mm_and_ps(_mm_cmpunord_ps(a, a), _mm_cmpunord_ps(b, b));
    const __m128 quiet = _mm_or_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x00400000)));
    return _mm_or_ps(_mm_and_ps(both, quiet), _mm_andnot_ps(both, r));
}

__m128 Mul(__m128 a, __m128 b)
{
    return FirstNaN(a, b, _mm_mul_ps(a, b));
}

__m128 Add(__m128 a, __m128 b)
{
    return FirstNaN(a, b, _mm_add_ps(a, b));
}

__m128 Sub(__m128 a, __m128 b)
{
    return FirstNaN(a, b, _mm_sub_ps(a, b));
}

__m128 AddSs(__m128 a, __m128 b)
{
    return _mm_move_ss(a, Add(a, b));
}

__m128 SubSs(__m128 a, __m128 b)
{
    return _mm_move_ss(a, Sub(a, b));
}

__m128 MulSs(__m128 a, __m128 b)
{
    return _mm_move_ss(a, Mul(a, b));
}

__m128 LoadPairs(const float* pLo, const float* pHi)
{
    return _mm_loadh_pi(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)pLo), (const __m64*)pHi);
}

bool Aligned16(const void* p)
{
    return ((size_t)p & 15) == 0;
}

void StoreVec3(D3DXVECTOR3* pOut, __m128 v)
{
    float af[4];
    _mm_storeu_ps(af, v);
    pOut->x = af[0];
    pOut->y = af[1];
    pOut->z = af[2];
}

float Lane0(__m128 v)
{
    return _mm_cvtss_f32(v);
}
}

namespace PcD3dx
{
D3DXMATRIX* MatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2)
{
    const __m128 b0 = _mm_loadu_ps(&pM2->_11);
    const __m128 b1 = _mm_loadu_ps(&pM2->_21);
    const __m128 b2 = _mm_loadu_ps(&pM2->_31);
    const __m128 b3 = _mm_loadu_ps(&pM2->_41);
    const float* pA = &pM1->_11;

    const __m128 r0 = Add(Add(Mul(Splat(pA[0]), b0), Mul(Splat(pA[1]), b1)),
                          Add(Mul(Splat(pA[2]), b2), Mul(Splat(pA[3]), b3)));
    const __m128 r1 = Add(Add(Add(Mul(Splat(pA[4]), b0), Mul(Splat(pA[5]), b1)), Mul(Splat(pA[6]), b2)),
                          Mul(Splat(pA[7]), b3));
    const __m128 r2 = Add(Add(Add(Mul(Splat(pA[8]), b0), Mul(Splat(pA[9]), b1)), Mul(Splat(pA[10]), b2)),
                          Mul(Splat(pA[11]), b3));
    const __m128 r3 = Add(Add(Mul(b0, Splat(pA[12])), Mul(b1, Splat(pA[13]))),
                          Add(Mul(b2, Splat(pA[14])), Mul(b3, Splat(pA[15]))));

    _mm_storeu_ps(&pOut->_11, r0);
    _mm_storeu_ps(&pOut->_21, r1);
    _mm_storeu_ps(&pOut->_31, r2);
    _mm_storeu_ps(&pOut->_41, r3);
    return pOut;
}

D3DXVECTOR3* Vec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
    const __m128 x = Splat(pV->x);
    const __m128 y = Splat(pV->y);
    const __m128 z = Splat(pV->z);
    const __m128 m0 = _mm_loadu_ps(&pM->_11);
    const __m128 m1 = _mm_loadu_ps(&pM->_21);
    const __m128 m2 = _mm_loadu_ps(&pM->_31);
    const __m128 m3 = _mm_loadu_ps(&pM->_41);
    const __m128 xy = Add(Mul(y, m1), Mul(x, m0));
    const __m128 zw = Aligned16(pM) ? Add(Mul(z, m2), m3) : Add(Mul(m2, z), m3);
    const __m128 s = Add(xy, zw);
    const __m128 r = _mm_rcp_ps(s);
    __m128 n = Sub(Add(r, r), Mul(Mul(r, s), r));
    n = _mm_shuffle_ps(n, n, 0xFF);
    StoreVec3(pOut, Mul(n, s));
    return pOut;
}

D3DXVECTOR3* Vec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
    const __m128 x = Splat(pV->x);
    const __m128 y = Splat(pV->y);
    const __m128 z = Splat(pV->z);
    const __m128 m0 = _mm_loadu_ps(&pM->_11);
    const __m128 m1 = _mm_loadu_ps(&pM->_21);
    const __m128 m2 = _mm_loadu_ps(&pM->_31);
    const __m128 xy = Add(Mul(y, m1), Mul(x, m0));
    const __m128 zt = Aligned16(pM) ? Mul(z, m2) : Mul(m2, z);
    StoreVec3(pOut, Add(xy, zt));
    return pOut;
}

D3DXVECTOR3* Vec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
    const __m128 x = _mm_set_ss(pV->x);
    const __m128 y = _mm_set_ss(pV->y);
    const __m128 z = _mm_set_ss(pV->z);
    const __m128 l = Add(Add(Mul(x, x), Mul(y, y)), Mul(z, z));
    if (!_mm_comige_ss(l, _mm_set_ss(BitsFloat(0x28800000u))))
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
        pOut->z = 0.0f;
        return pOut;
    }
    const __m128 r = _mm_rsqrt_ss(l);
    const __m128 t = Mul(Mul(r, l), r);
    const __m128 h = Mul(r, _mm_set_ss(0.5f));
    const __m128 k = Mul(Sub(_mm_set_ss(3.0f), t), h);
    pOut->x = Lane0(Mul(x, k));
    pOut->y = Lane0(Mul(y, k));
    pOut->z = Lane0(Mul(z, k));
    return pOut;
}

D3DXQUATERNION* QuaternionNormalize(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ)
{
    const __m128 x = _mm_set_ss(pQ->x);
    const __m128 y = _mm_set_ss(pQ->y);
    const __m128 z = _mm_set_ss(pQ->z);
    const __m128 w = _mm_set_ss(pQ->w);
    const __m128 l = Add(Add(Mul(x, x), Mul(y, y)), Add(Mul(z, z), Mul(w, w)));
    const __m128 r = _mm_rsqrt_ss(l);
    const __m128 t = Mul(l, Mul(r, r));
    const __m128 k = Mul(Mul(Sub(_mm_set_ss(3.0f), t), _mm_set_ss(0.5f)), r);
    pOut->x = Lane0(Mul(x, k));
    pOut->y = Lane0(Mul(y, k));
    pOut->z = Lane0(Mul(z, k));
    pOut->w = Lane0(Mul(w, k));
    return pOut;
}

D3DXMATRIX* MatrixRotationQuaternion(D3DXMATRIX* pOut, const D3DXQUATERNION* pQ)
{
    const float fX = Quiet(pQ->x);
    const float fY = Quiet(pQ->y);
    const float fZ = Quiet(pQ->z);
    const float fW = Quiet(pQ->w);
    const float fX2 = fX + fX;
    const float fY2 = fY + fY;
    const float fZ2 = fZ + fZ;
    const float fWX = X87Mul(fW, fX2);
    const float fWY = X87Mul(fW, fY2);
    const float fWZ = X87Mul(fW, fZ2);
    const float fXX = X87Mul(fX2, fX);
    const float fXY = X87Mul(fX, fY2);
    const float fXZ = X87Mul(fX, fZ2);
    const float fYY = X87Mul(fY2, fY);
    const float fYZ = X87Mul(fY, fZ2);
    const float fZZ = X87Mul(fZ2, fZ);

    pOut->_11 = X87OneMinusMinus(fYY, fZZ);
    pOut->_12 = X87Add(fXY, fWZ);
    pOut->_13 = X87Sub(fXZ, fWY);
    pOut->_14 = 0.0f;
    pOut->_21 = X87Sub(fXY, fWZ);
    pOut->_22 = X87OneMinusMinus(fXX, fZZ);
    pOut->_23 = X87Add(fYZ, fWX);
    pOut->_24 = 0.0f;
    pOut->_31 = X87Add(fWY, fXZ);
    pOut->_32 = X87Sub(fYZ, fWX);
    pOut->_33 = X87OneMinusMinus(fXX, fYY);
    pOut->_34 = 0.0f;
    pOut->_41 = 0.0f;
    pOut->_42 = 0.0f;
    pOut->_43 = 0.0f;
    pOut->_44 = 1.0f;
    return pOut;
}

D3DXMATRIX* MatrixScaling(D3DXMATRIX* pOut, FLOAT fSx, FLOAT fSy, FLOAT fSz)
{
    const float fX = Quiet(fSx);
    const float fY = Quiet(fSy);
    const float fZ = Quiet(fSz);
    pOut->_11 = fX;
    pOut->_12 = 0.0f;
    pOut->_13 = 0.0f;
    pOut->_14 = 0.0f;
    pOut->_21 = 0.0f;
    pOut->_22 = fY;
    pOut->_23 = 0.0f;
    pOut->_24 = 0.0f;
    pOut->_31 = 0.0f;
    pOut->_32 = 0.0f;
    pOut->_33 = fZ;
    pOut->_34 = 0.0f;
    pOut->_41 = 0.0f;
    pOut->_42 = 0.0f;
    pOut->_43 = 0.0f;
    pOut->_44 = 1.0f;
    return pOut;
}

D3DXVECTOR4* Vec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM)
{
    const __m128 v = _mm_loadu_ps(&pV->x);
    const __m128 x = _mm_shuffle_ps(v, v, 0x00);
    const __m128 y = _mm_shuffle_ps(v, v, 0x55);
    const __m128 z = _mm_shuffle_ps(v, v, 0xAA);
    const __m128 w = _mm_shuffle_ps(v, v, 0xFF);
    const __m128 zw = Add(Mul(w, _mm_loadu_ps(&pM->_41)), Mul(z, _mm_loadu_ps(&pM->_31)));
    const __m128 xy = Add(Mul(y, _mm_loadu_ps(&pM->_21)), Mul(x, _mm_loadu_ps(&pM->_11)));
    _mm_storeu_ps(&pOut->x, Add(zw, xy));
    return pOut;
}

D3DXMATRIX* MatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, const D3DXMATRIX* pM)
{
    const float* pIn = &pM->_11;
    __m128 x1 = LoadPairs(pIn + 0, pIn + 4);
    __m128 x2 = LoadPairs(pIn + 2, pIn + 6);
    __m128 x3 = LoadPairs(pIn + 8, pIn + 12);
    __m128 x4 = LoadPairs(pIn + 10, pIn + 14);
    __m128 x5;
    __m128 x6;
    __m128 x7;
    __m128 s20;
    __m128 s30;
    __m128 s40;
    __m128 s50;
    __m128 s60;
    __m128 s70;
    __m128 s80;
    __m128 s90;
    __m128 sA0;
    __m128 sB0;
    __m128 sC0;
    __m128 sD0;
    __m128 sE0;
    x5 = x2;
    x5 = _mm_shuffle_ps(x5, x4, 0x88);
    x4 = _mm_shuffle_ps(x4, x2, 0xDD);
    x2 = x4;
    x2 = Mul(x2, x5);
    x2 = _mm_shuffle_ps(x2, x2, 0xB1);
    x6 = x2;
    x6 = _mm_shuffle_ps(x6, x6, 0x4E);
    x7 = x3;
    x3 = _mm_shuffle_ps(x3, x1, 0xDD);
    x1 = _mm_shuffle_ps(x1, x7, 0x88);
    x7 = x3;
    x3 = Mul(x3, x6);
    x6 = Mul(x6, x1);
    sD0 = x6;
    x6 = x7;
    x7 = Mul(x7, x2);
    x2 = Mul(x2, x1);
    x3 = Sub(x3, x7);
    x7 = x6;
    x7 = Mul(x7, x5);
    x5 = _mm_shuffle_ps(x5, x5, 0x4E);
    x7 = _mm_shuffle_ps(x7, x7, 0xB1);
    sC0 = x2;
    x2 = x4;
    x2 = Mul(x2, x7);
    x2 = Add(x2, x3);
    x3 = x7;
    x7 = _mm_shuffle_ps(x7, x7, 0x4E);
    x3 = Mul(x3, x1);
    sB0 = x3;
    x3 = x4;
    x3 = Mul(x3, x7);
    x7 = Mul(x7, x1);
    x2 = Sub(x2, x3);
    x3 = x6;
    x3 = _mm_shuffle_ps(x3, x3, 0x4E);
    x3 = Mul(x3, x4);
    x3 = _mm_shuffle_ps(x3, x3, 0xB1);
    sA0 = x7;
    x7 = x5;
    x5 = Mul(x5, x3);
    x5 = Add(x5, x2);
    x2 = x3;
    x3 = _mm_shuffle_ps(x3, x3, 0x4E);
    x2 = Mul(x2, x1);
    s90 = x4;
    x4 = x7;
    x7 = Mul(x7, x3);
    x3 = Mul(x3, x1);
    x5 = Sub(x5, x7);
    x3 = Sub(x3, x2);
    x2 = x1;
    x1 = Mul(x1, x5);
    x3 = _mm_shuffle_ps(x3, x3, 0x4E);
    x7 = x1;
    x1 = _mm_shuffle_ps(x1, x1, 0x4E);
    sE0 = x5;
    x1 = Add(x1, x7);
    x5 = x1;
    x1 = _mm_shuffle_ps(x1, x1, 0xB1);
    x1 = AddSs(x1, x5);
    x5 = x6;
    x5 = Mul(x5, x2);
    x5 = _mm_shuffle_ps(x5, x5, 0xB1);
    x7 = x5;
    x5 = _mm_shuffle_ps(x5, x5, 0x4E);
    s80 = x4;
    x4 = s90;
    s70 = x6;
    x6 = x4;
    x6 = Mul(x6, x7);
    x6 = Add(x6, x3);
    x3 = x4;
    x3 = Mul(x3, x5);
    x3 = Sub(x3, x6);
    x6 = x4;
    x6 = Mul(x6, x2);
    x6 = _mm_shuffle_ps(x6, x6, 0xB1);
    s60 = x5;
    x5 = s70;
    s50 = x7;
    x7 = x6;
    x7 = Mul(x7, x5);
    x7 = Add(x7, x3);
    x3 = x6;
    x3 = _mm_shuffle_ps(x3, x3, 0x4E);
    s40 = x4;
    x4 = x5;
    x5 = Mul(x5, x3);
    s30 = x4;
    x4 = x6;
    x6 = x7;
    x6 = Sub(x6, x5);
    x5 = sD0;
    x7 = sC0;
    x5 = Sub(x5, x7);
    x5 = _mm_shuffle_ps(x5, x5, 0x4E);
    x7 = s80;
    x4 = Mul(x4, x7);
    x3 = Mul(x3, x7);
    x5 = Sub(x5, x4);
    x2 = Mul(x2, x7);
    x3 = Add(x3, x5);
    x2 = _mm_shuffle_ps(x2, x2, 0xB1);
    x4 = x2;
    x4 = _mm_shuffle_ps(x4, x4, 0x4E);
    x5 = s40;
    s20 = x6;
    x6 = x5;
    x5 = Mul(x5, x2);
    x6 = Mul(x6, x4);
    x5 = Add(x5, x3);
    x3 = x4;
    x4 = x5;
    x4 = Sub(x4, x6);
    x5 = sA0;
    x6 = sB0;
    x5 = Sub(x5, x6);
    x5 = _mm_shuffle_ps(x5, x5, 0x4E);
    x6 = s50;
    x6 = Mul(x6, x7);
    x6 = Sub(x6, x5);
    x5 = s60;
    x7 = Mul(x7, x5);
    x6 = Sub(x6, x7);
    x5 = s30;
    x2 = Mul(x2, x5);
    x5 = Mul(x5, x3);
    x6 = Sub(x6, x2);
    x2 = x5;
    x2 = Add(x2, x6);
    x6 = s20;

    const float fDet = Lane0(x1);
    if (fDet == 0.0f || IsNaN(fDet))
    {
        return 0;
    }
    if (pDeterminant)
    {
        *pDeterminant = fDet;
    }
    __m128 x0 = x1;
    x1 = sE0;
    x3 = x0;
    x0 = _mm_rcp_ss(x0);
    x5 = x0;
    x0 = MulSs(x0, x0);
    x3 = MulSs(x3, x0);
    x5 = AddSs(x5, x5);
    x5 = SubSs(x5, x3);
    x5 = _mm_shuffle_ps(x5, x5, 0x00);
    _mm_storeu_ps(&pOut->_11, Mul(x1, x5));
    _mm_storeu_ps(&pOut->_21, Mul(x4, x5));
    _mm_storeu_ps(&pOut->_31, Mul(x6, x5));
    _mm_storeu_ps(&pOut->_41, Mul(x5, x2));
    return pOut;
}
}

D3DXVECTOR3* PcD3dxVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
    return PcD3dx::Vec3Normalize(pOut, pV);
}
