#pragma once

#include "PebbleFL/Common/PblCrtTrig.h"

inline float PblX87MulAddF(float fA, float fB, float fC)
{
    const double dProduct = (double)fA * (double)fB;
    if (dProduct == dProduct)
    {
        return (float)(dProduct + (double)fC);
    }
    const unsigned int uA = PblCrtBitsOfFloat(fA);
    const unsigned int uB = PblCrtBitsOfFloat(fB);
    unsigned int uProduct = 0xFFC00000u;
    if (PblCrtIsNanBits(uA) || PblCrtIsNanBits(uB))
    {
        uProduct = PblCrtX87NanPair(uA, uB);
    }
    return PblCrtFloatOfBits(PblCrtX87NanPair(uProduct, PblCrtBitsOfFloat(fC)));
}

inline float PblX87AddMulF(float fA, float fB, float fC)
{
    const double dSum = (double)fA + (double)fB;
    if (dSum == dSum)
    {
        return (float)(dSum * (double)fC);
    }
    const unsigned int uA = PblCrtBitsOfFloat(fA);
    const unsigned int uB = PblCrtBitsOfFloat(fB);
    unsigned int uSum = 0xFFC00000u;
    if (PblCrtIsNanBits(uA) || PblCrtIsNanBits(uB))
    {
        uSum = PblCrtX87NanPair(uA, uB);
    }
    return PblCrtFloatOfBits(PblCrtX87NanPair(uSum, PblCrtBitsOfFloat(fC)));
}

inline float PblX87MulSubF(float fA, float fB, float fC)
{
    const double dProduct = (double)fA * (double)fB;
    if (dProduct == dProduct)
    {
        return (float)(dProduct - (double)fC);
    }
    return PblX87MulAddF(fA, fB, fC);
}

inline unsigned int PblX87NanStep(unsigned int uX, unsigned int uY)
{
    if (PblCrtIsNanBits(uX) || PblCrtIsNanBits(uY))
    {
        return PblCrtX87NanPair(uX, uY);
    }
    return 0xFFC00000u;
}

inline float PblX87AddAddF(float fA, float fB, float fC)
{
    const double dSum = (double)fA + (double)fB;
    const double dOut = dSum + (double)fC;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uSum = 0;
    if (dSum != dSum)
    {
        uSum = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uSum, PblCrtBitsOfFloat(fC)));
}

inline float PblX87SubSubF(float fA, float fB, float fC)
{
    const double dDiff = (double)fA - (double)fB;
    const double dOut = dDiff - (double)fC;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uDiff = 0;
    if (dDiff != dDiff)
    {
        uDiff = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uDiff, PblCrtBitsOfFloat(fC)));
}

inline float PblX87SubDivF(float fC, float fA, float fB)
{
    const double dQuot = (double)fA / (double)fB;
    const double dOut = (double)fC - dQuot;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uQuot = 0;
    if (dQuot != dQuot)
    {
        uQuot = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uQuot, PblCrtBitsOfFloat(fC)));
}

inline float PblX87MulDivF(float fA, float fB, float fC)
{
    const double dProduct = (double)fA * (double)fB;
    const double dOut = dProduct / (double)fC;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uProduct = 0;
    if (dProduct != dProduct)
    {
        uProduct = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uProduct, PblCrtBitsOfFloat(fC)));
}

inline float PblX87AddMulAddF(float fA, float fB, float fC, float fD)
{
    const double dSum = (double)fA + (double)fB;
    const double dProduct = dSum * (double)fC;
    const double dOut = dProduct + (double)fD;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uSum = 0;
    if (dSum != dSum)
    {
        uSum = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    unsigned int uProduct = 0;
    if (dProduct != dProduct)
    {
        uProduct = PblX87NanStep(uSum, PblCrtBitsOfFloat(fC));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uProduct, PblCrtBitsOfFloat(fD)));
}

inline float PblX87Dot3F(float fAx, float fAy, float fAz, float fBx, float fBy, float fBz)
{
    const double dY = (double)fAy * (double)fBy;
    const double dX = (double)fAx * (double)fBx;
    const double dSum = dY + dX;
    const double dZ = (double)fAz * (double)fBz;
    const double dOut = dSum + dZ;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uY = 0;
    if (dY != dY)
    {
        uY = PblX87NanStep(PblCrtBitsOfFloat(fAy), PblCrtBitsOfFloat(fBy));
    }
    unsigned int uX = 0;
    if (dX != dX)
    {
        uX = PblX87NanStep(PblCrtBitsOfFloat(fAx), PblCrtBitsOfFloat(fBx));
    }
    unsigned int uSum = 0;
    if (dSum != dSum)
    {
        uSum = PblX87NanStep(uY, uX);
    }
    unsigned int uZ = 0;
    if (dZ != dZ)
    {
        uZ = PblX87NanStep(PblCrtBitsOfFloat(fAz), PblCrtBitsOfFloat(fBz));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uSum, uZ));
}

inline double PblX87HalfStepD(double dX)
{
    return dX * 0.5 + 0.5;
}

inline float PblX87AddAddMulAddF(float fA, float fB, float fC, float fD, float fE)
{
    const double dSum = (double)fA + (double)fB;
    const double dSpread = dSum + (double)fC;
    const double dProduct = dSpread * (double)fD;
    const double dOut = dProduct + (double)fE;
    if (dOut == dOut)
    {
        return (float)dOut;
    }
    unsigned int uSum = 0;
    if (dSum != dSum)
    {
        uSum = PblX87NanStep(PblCrtBitsOfFloat(fA), PblCrtBitsOfFloat(fB));
    }
    unsigned int uSpread = 0;
    if (dSpread != dSpread)
    {
        uSpread = PblX87NanStep(uSum, PblCrtBitsOfFloat(fC));
    }
    unsigned int uProduct = 0;
    if (dProduct != dProduct)
    {
        uProduct = PblX87NanStep(uSpread, PblCrtBitsOfFloat(fD));
    }
    return PblCrtFloatOfBits(PblX87NanStep(uProduct, PblCrtBitsOfFloat(fE)));
}
