#pragma once

#include <stddef.h>
#include <float.h>
#include <d3dx9math.h>
#include "PblMath.h"
#include <cmath>

static_assert(sizeof(sin(1.0f)) == 4, "float math overloads");

class PblVector3 : public D3DXVECTOR3
{
public:

    // FUNCTION: SWBF2 0x0040c9a0
    PblVector3() {}

    PblVector3(float fX, float fY, float fZ);

    PblVector3(const PblVector3& v);

    PblVector3(const D3DXVECTOR3* pVector);

    void SetAll(float fValue);

    bool operator!=(const PblVector3& v) const;

    bool operator==(const PblVector3& v) const;

    const PblVector3& operator+=(const PblVector3& rhs);
    const PblVector3& operator-=(const PblVector3& rhs);
    const PblVector3& operator*=(float rhs);
    const PblVector3& operator/=(float rhs);
    const PblVector3 operator-() const;

    float& operator[](int i);
    float operator[](int i) const;
    float& operator()(int i);
    float operator()(int i) const;

    float GetX() const;
    float GetY() const;
    float GetZ() const;

    void Set(float fX, float fY, float fZ);
    void SetX(float fVal);
    void SetY(float fVal);
    void SetZ(float fVal);
    void Reset();

    void AddX(float value);
    void AddY(float value);
    void AddZ(float value);
    void MulX(float value);
    void MulY(float value);
    void MulZ(float value);

};

 
 
 
 

const PblVector3 operator+(const PblVector3& lhs, const PblVector3& rhs);
const PblVector3 operator-(const PblVector3& lhs, const PblVector3& rhs);
const PblVector3 operator*(const PblVector3& lhs, float rhs);
const PblVector3 operator*(float lhs, const PblVector3& rhs);
const PblVector3 operator*(const PblVector3& lhs, const PblVector3& rhs);
const PblVector3 operator/(const PblVector3& lhs, float rhs);

float DotProduct(const PblVector3& vVector1, const PblVector3& vVector2);
const float Length(const PblVector3& vVector);
const float LengthXZ(const PblVector3& vVector);
const float SquaredLength(const PblVector3& vVector);
const float SquaredLengthXZ(const PblVector3& vVector);
const PblVector3 CrossProduct(const PblVector3& lhs, const PblVector3& rhs);
const PblVector3 Normalize(const PblVector3& vVector);
const PblVector3 RotateY(const PblVector3& vVector, float fAngle);
const PblVector3 RotateZ(const PblVector3& vVector, float fAngle);



const PblVector3 Interpolate(const PblVector3& v0, const PblVector3& v1, float fT);

extern PblVector3 gVectorUnitX;
extern PblVector3 gVectorUnitY;
extern PblVector3 gVectorUnitZ;

class PblVector4 : public D3DXVECTOR4
{
public:

    PblVector4();
    PblVector4(float fX, float fY, float fZ, float fW);
    explicit PblVector4(float fValue);


    operator const PblVector3&() const;

    void Set(float fX, float fY, float fZ, float fW);
    void SetX(float fVal);
    void SetY(float fVal);
    void SetZ(float fVal);
    void SetW(float fVal);

};

#include "PebbleFL/pc/pcPblVector.inl"

class PblSphere : public PblVector4
{
public:
    PblSphere() {}
    PblSphere(float fX, float fY, float fZ, float fRadius)
        : PblVector4(fX, fY, fZ, fRadius)
    {
    }
    PblSphere(const PblVector3& vCenter, float fRadius)
        : PblVector4(vCenter.x, vCenter.y, vCenter.z, fRadius)
    {
    }

    // FUNCTION: SWBF2 0x0040fdf0
    const PblVector3& GetCenter() const { return *(const PblVector3*)this; }
    float GetRadius() const { return w; }

    void SetCenter(const PblVector3& rCenter) { x = rCenter.x; y = rCenter.y; z = rCenter.z; }
    void SetRadius(float fRadius) { w = fRadius; }
};

inline const float LengthEst(const PblVector3& rv)
{
    return PblMath::Length3Est(rv.x, rv.y, rv.z);
}

// FUNCTION: SWBF2 0x004a9b50
inline const PblVector3 NormalizeEst(const PblVector3& rv)
{
    const float length = LengthEst(rv);
    return length != 0.0f ? rv / length : rv;
}

class PblBox
{
public:

    void AddPoint(const PblVector3& rv)
    {
        if (_vMin.x > _vMax.x)
        {
            _vMin = _vMax = rv;
            return;
        }
        if (_vMin.x > rv.x)
        {
            _vMin.x = rv.x;
        }
        else if (rv.x > _vMax.x)
        {
            _vMax.x = rv.x;
        }
        if (_vMin.y > rv.y)
        {
            _vMin.y = rv.y;
        }
        else if (rv.y > _vMax.y)
        {
            _vMax.y = rv.y;
        }
        if (_vMin.z > rv.z)
        {
            _vMin.z = rv.z;
        }
        else if (rv.z > _vMax.z)
        {
            _vMax.z = rv.z;
        }
    }

    const PblVector3 GetBoxCorner(int iCorner) const
    {
        return PblVector3(iCorner & 4 ? _vMax.x : _vMin.x,
                          iCorner & 2 ? _vMax.y : _vMin.y,
                          iCorner & 1 ? _vMax.z : _vMin.z);
    }

    const PblBox GetTransformedBox(const class PblMatrix& rMatrix) const;

    PblBox() {}

    explicit PblBox(const PblVector3& rv) : _vMin(rv), _vMax(rv) {}
    PblBox(const PblVector3& vMin, const PblVector3& vMax) : _vMin(vMin), _vMax(vMax) {}
    // FUNCTION: SWBF2 0x0040c930
    PblBox(float fMinX, float fMinY, float fMinZ, float fMaxX, float fMaxY, float fMaxZ)
: _vMin(fMinX, fMinY, fMinZ), _vMax(fMaxX, fMaxY, fMaxZ)
{
}

    void SetMin(const PblVector3& rv) { _vMin = rv; }
    void SetMax(const PblVector3& rv) { _vMax = rv; }
    void SetMin(float fX, float fY, float fZ) { _vMin.Set(fX, fY, fZ); }
    void SetMax(float fX, float fY, float fZ) { _vMax.Set(fX, fY, fZ); }

    void Set(const PblVector3& vMin, const PblVector3& vMax) { _vMin = vMin; _vMax = vMax; }

    void SetEmpty()
    {
        SetMin(FLT_MAX, FLT_MAX, FLT_MAX);
        SetMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    // FUNCTION: SWBF2 0x0040c980
    const PblVector3& GetMin() const { return _vMin; }
    // FUNCTION: SWBF2 0x0040c990
    const PblVector3& GetMax() const { return _vMax; }

    bool ContainsPoint(const PblVector3& point) const
    {
        return point.x >= _vMin.x && point.x <= _vMax.x &&
               point.y >= _vMin.y && point.y <= _vMax.y &&
               point.z >= _vMin.z && point.z <= _vMax.z;
    }

    PblVector3 _vMin;
    PblVector3 _vMax;
};

// SYNTHETIC: SWBF2 0x004615e0
// PblBox::PblBox(class PblBox const &)

 
 
 
 
 
 
 
 
 
 
 
 
