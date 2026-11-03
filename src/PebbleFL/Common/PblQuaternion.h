#pragma once

#include "PblVector.h"
#include "PblMath.h"

class PblMatrix;
class PblVector3;

class PblQuaternion
{
public:

    // FUNCTION: SWBF2 0x0043ce60
    PblQuaternion()
    {
        _fS = 1.0f;
        _fX = 0.0f;
        _fY = 0.0f;
        _fZ = 0.0f;
    }
    PblQuaternion(float fS, float fX, float fY, float fZ)
    {
        _fS = fS;
        _fX = fX;
        _fY = fY;
        _fZ = fZ;
    }
    PblQuaternion(const PblVector3& vAxis, float fAngle)
    {
        float fSin, fCos;
        PblMath::SinCos(&fSin, &fCos, fAngle * 0.5f);
        _fS = fCos;
        _fX = vAxis.x * fSin;
        _fY = vAxis.y * fSin;
        _fZ = vAxis.z * fSin;
    }

    PblQuaternion(const PblMatrix& rMatrix);

    float GetS() const { return _fS; }
    float GetX() const { return _fX; }
    float GetY() const { return _fY; }
    float GetZ() const { return _fZ; }

    void SetS(float value) { _fS = value; }
    void SetX(float value) { _fX = value; }
    void SetY(float value) { _fY = value; }
    void SetZ(float value) { _fZ = value; }

    PblQuaternion Scale(float scale) const
    {
        return PblQuaternion(_fS * scale, _fX * scale, _fY * scale, _fZ * scale);
    }

    float DotProduct(const PblQuaternion& other) const
    {
        return _fS * other._fS + _fX * other._fX + _fY * other._fY + _fZ * other._fZ;
    }

    PblQuaternion Inverse() const
    {
        return PblQuaternion(_fS, -_fX, -_fY, -_fZ);
    }

    PblVector3 RotateVector(const PblVector3& v) const;

    float _fX;
    float _fY;
    float _fZ;
    float _fS;
};

PblQuaternion Interpolate(const PblQuaternion& q0, const PblQuaternion& q1, float fT);

PblQuaternion operator*(const PblQuaternion& q0, const PblQuaternion& q1);

inline PblQuaternion operator*=(PblQuaternion& lhs, const PblQuaternion& rhs)
{
    return lhs = lhs * rhs;
}

 

inline PblQuaternion operator+(const PblQuaternion& lhs, const PblQuaternion& rhs)
{
    return PblQuaternion(lhs.GetS() + rhs.GetS(), lhs.GetX() + rhs.GetX(),
                         lhs.GetY() + rhs.GetY(), lhs.GetZ() + rhs.GetZ());
}
