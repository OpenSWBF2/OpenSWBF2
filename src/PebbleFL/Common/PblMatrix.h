#pragma once

#include <stddef.h>
#include <string.h>
#include <d3dx9math.h>
#include "PblVector.h"

class PblVector3;
class PblVector4;
class PblQuaternion;
struct _D3DXMATRIXA16;

class __declspec(align(16)) PblMatrix : public D3DXMATRIXA16
{
public:

    // FUNCTION: SWBF2 0x0040fe00
    PblMatrix() {}

    // FUNCTION: SWBF2 0x0040cb30
    PblMatrix(const PblMatrix& other)
    {
        memcpy(m, other.m, sizeof(m));
    }

    PblMatrix(const _D3DXMATRIXA16& other)
    {
        memcpy(m, &other, sizeof(m));
    }

    // FUNCTION: SWBF2 0x0043a880
    PblMatrix(const _D3DXMATRIXA16* other)
    {
        memcpy(m, other, sizeof(m));
    }

    PblMatrix(const PblQuaternion& rotation, const PblVector3& translation);

    PblMatrix(const PblVector4& x, const PblVector4& y,
              const PblVector4& z, const PblVector4& t);

    PblMatrix(const PblVector3& x, const PblVector3& y,
              const PblVector3& z, const PblVector3& t);

    const PblMatrix& operator*=(const PblMatrix& rhs);

    const PblMatrix GetInverse() const;
    const PblMatrix GetInverseFast() const;

    const PblVector3 ApplyRotation(const PblVector3& vector) const;
    const PblVector3 ApplyTransform(const PblVector3& vector) const;
    const PblVector3 ApplyTranslation(const PblVector3& vector) const;

    void Reset();

    void SetRotateArbitraryAxis(float fAngle, const PblVector3& vAxis);
    void SetRotateX(float fAngle);
    void SetRotateY(float fAngle);
    void SetRotateZ(float fAngle);
    void RotateX(float fAngle);
    void RotateY(float fAngle);
    void RotateZ(float fAngle);
    void Scale(float fScaleX, float fScaleY, float fScaleZ);
    void Scale(float fScale) { Scale(fScale, fScale, fScale); }

    void Transpose3x3();

    void Transpose4x4();

    const PblVector3& GetTranslation() const;

    const PblVector3& GetXAxis() const;
    const PblVector3& GetYAxis() const;

    const PblVector3& GetZAxis() const;

    void SetXAxis(const PblVector3& vAxis);
    void SetYAxis(const PblVector3& vAxis);
    void SetZAxis(const PblVector3& vAxis);

    void SetXAxis(float fX, float fY, float fZ, float fW);
    void SetYAxis(float fX, float fY, float fZ, float fW);
    void SetZAxis(float fX, float fY, float fZ, float fW);
    void SetTranslation(float fX, float fY, float fZ, float fW);
    void SetTranslation(const PblVector3& vTranslation);

    void Translate(float fX, float fY, float fZ);
    void Translate(const PblVector3& vTranslation)
    {
        Translate(vTranslation.x, vTranslation.y, vTranslation.z);
    }

    float& operator()(int iRow, int iColumn);
    float operator()(int iRow, int iColumn) const;

    PblVector3& operator[](int iRow);
    const PblVector3& operator[](int iRow) const;

};

#include "PebbleFL/pc/pcPblMatrix.inl"

// FUNCTION: SWBF2 0x0043cd40
inline PblMatrix DirectionMatrix(const PblVector3& vDirection, const PblVector3& vReferenceUp,
                                 const PblVector3& vTranslation)
{
    PblVector3 vZAxis = Normalize(vDirection);
    PblVector3 vXAxis = Normalize(CrossProduct(vReferenceUp, vDirection));
    PblVector3 vYAxis = CrossProduct(vZAxis, vXAxis);
    return PblMatrix(vXAxis, vYAxis, vZAxis, vTranslation);
}

// FUNCTION: SWBF2 0x0047a570
inline PblMatrix NormalMatrix(const PblVector3& vDir, const PblVector3& vNormal, const PblVector3& vPos)
{
    PblVector3 vUp;
    D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vUp), reinterpret_cast<const D3DXVECTOR3*>(&vNormal));
    PblVector3 vRight;
    D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vRight), reinterpret_cast<const D3DXVECTOR3*>(&CrossProduct(vUp, vDir)));

    PblMatrix mResult(vRight, vUp, CrossProduct(vRight, vUp), vPos);
    return mResult;
}

extern PblMatrix gMatrixIdentity;
