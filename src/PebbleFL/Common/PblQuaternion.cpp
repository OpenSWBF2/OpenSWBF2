#include "PblQuaternion.h"

#include <math.h>
#include <d3dx9math.h>

#include "PblMath.h"
#include "PblMatrix.h"
#include "PblVector.h"





enum QUAT_INDEX
{
    X_INDEX = 0,
    Y_INDEX = 1,
    Z_INDEX = 2
};

// GLOBAL: SWBF2 0x007e2d80
static const QUAT_INDEX QIndex[3] = { Y_INDEX, Z_INDEX, X_INDEX };

// FUNCTION: SWBF2 0x0072a0d0
PblQuaternion::PblQuaternion(const PblMatrix& rMatrix)
{
    float* pq = &_fX;
    float fTrace = rMatrix.m[0][0] + rMatrix.m[1][1] + rMatrix.m[2][2];
    if (fTrace > 0.0f)
    {
        float fS = (float)sqrt((double)(fTrace + 1.0f));
        _fS = fS * 0.5f;
        fS = 0.5f / fS;
        _fX = (rMatrix.m[2][1] - rMatrix.m[1][2]) * fS;
        _fY = (rMatrix.m[0][2] - rMatrix.m[2][0]) * fS;
        _fZ = (rMatrix.m[1][0] - rMatrix.m[0][1]) * fS;
    }
    else
    {
        int i = X_INDEX;
        if (rMatrix.m[1][1] > rMatrix.m[0][0])
        {
            i = Y_INDEX;
        }
        if (rMatrix.m[2][2] > rMatrix.m[i][i])
        {
            i = Z_INDEX;
        }
        int j = QIndex[i];
        int k = QIndex[j];

        float fS = (float)sqrt((double)(rMatrix.m[i][i] + 1.0f - rMatrix.m[j][j] - rMatrix.m[k][k]));
        pq[i] = fS * 0.5f;
        fS = 0.5f / fS;
        _fS = (rMatrix.m[k][j] - rMatrix.m[j][k]) * fS;
        pq[j] = (rMatrix.m[j][i] + rMatrix.m[i][j]) * fS;
        pq[k] = (rMatrix.m[k][i] + rMatrix.m[i][k]) * fS;
    }
    _fX = -_fX;
    _fY = -_fY;
    _fZ = -_fZ;
}

// FUNCTION: SWBF2 0x0072a2a0
PblQuaternion operator*(const PblQuaternion& q0, const PblQuaternion& q1)
{
    return PblQuaternion(
        q1.GetS() * q0.GetS() - q1.GetX() * q0.GetX() - q1.GetY() * q0.GetY() - q1.GetZ() * q0.GetZ(),
        q0.GetX() * q1.GetS() + q1.GetX() * q0.GetS() - q0.GetZ() * q1.GetY() + q1.GetZ() * q0.GetY(),
        q0.GetZ() * q1.GetX() + q0.GetY() * q1.GetS() + q1.GetY() * q0.GetS() - q1.GetZ() * q0.GetX(),
        q0.GetZ() * q1.GetS() - q0.GetY() * q1.GetX() + q1.GetY() * q0.GetX() + q1.GetZ() * q0.GetS());
}

// FUNCTION: SWBF2 0x0072a380
PblQuaternion Interpolate(const PblQuaternion& q0, const PblQuaternion& q1, float fT)
{
    const float fCos = q0.GetS() * q1.GetS() + q0.GetX() * q1.GetX()
                     + q0.GetY() * q1.GetY() + q0.GetZ() * q1.GetZ();

    float fScale0;
    float fScale1;
    float fAngle;
    float fInvSin;
    if (fCos >= 0.0f)
    {
        if (1.0f - fCos < 0.0001f)
        {
            fScale0 = 1.0f - fT;
            fScale1 = fT;
        }
        else
        {
            fAngle = acosf(fCos);
            fInvSin = 1.0f / PblMath::Sin(fAngle);
            fScale0 = PblMath::Sin((1.0f - fT) * fAngle) * fInvSin;
            fScale1 = PblMath::Sin(fAngle * fT) * fInvSin;
        }
    }
    else
    {
        const float fNegCos = -fCos;
        if (1.0f - fNegCos < 0.0001f)
        {
            fScale0 = 1.0f - fT;
            fScale1 = -fT;
        }
        else
        {
            fAngle = acosf(fNegCos);
            fInvSin = 1.0f / PblMath::Sin(fAngle);
            fScale0 = PblMath::Sin((1.0f - fT) * fAngle) * fInvSin;
            fScale1 = -(PblMath::Sin(fAngle * fT) * fInvSin);
        }
    }

    PblQuaternion q = q0.Scale(fScale0) + q1.Scale(fScale1);

    const float fLengthSq = q.GetX() * q.GetX() + q.GetS() * q.GetS()
                          + q.GetY() * q.GetY() + q.GetZ() * q.GetZ();
    if (fabsf(fLengthSq - 1.0f) > 0.0001f)
    {
        q = q.Scale(1.0f / PblMath::Sqrt(fLengthSq));
    }
    return q;
}

// FUNCTION: SWBF2 0x0072a670
PblVector3 PblQuaternion::RotateVector(const PblVector3& v) const
{
    D3DXQUATERNION qUnit;
    PblMatrix mRot;
    D3DXMatrixRotationQuaternion(reinterpret_cast<D3DXMATRIX*>(&mRot),
        D3DXQuaternionNormalize(&qUnit, reinterpret_cast<const D3DXQUATERNION*>(this)));
    mRot.m[3][0] = 0.0f;
    mRot.m[3][1] = 0.0f;
    mRot.m[3][2] = 0.0f;
    mRot.m[3][3] = 1.0f;
    PblVector3 vOut;
    D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(&vOut),
        reinterpret_cast<const D3DXVECTOR3*>(&v), reinterpret_cast<const D3DXMATRIX*>(&mRot));
    return vOut;
}
