#pragma once

#include <d3dx9math.h>

static D3DXMATRIXA16 _IdentityMatrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

// FUNCTION: SWBF2 0x004384f0
inline PblMatrix::PblMatrix(const PblVector3& x, const PblVector3& y,
                     const PblVector3& z, const PblVector3& t)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = 0.0f;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = 0.0f;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = 0.0f;
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
    m[3][3] = 1.0f;
}

// FUNCTION: SWBF2 0x0062c130
inline PblMatrix::PblMatrix(const PblVector4& x, const PblVector4& y,
                     const PblVector4& z, const PblVector4& t)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = x.w;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = y.w;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = z.w;
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
    m[3][3] = t.w;
}

// FUNCTION: SWBF2 0x0043a8c0
inline PblMatrix::PblMatrix(const PblQuaternion& rotation, const PblVector3& translation)
{
    D3DXQUATERNION normalized;
    D3DXMatrixRotationQuaternion((D3DXMATRIX*)this,
        D3DXQuaternionNormalize(&normalized, (const D3DXQUATERNION*)&rotation));
    m[3][0] = translation.x;
    m[3][1] = translation.y;
    m[3][2] = translation.z;
    m[3][3] = 1.0f;
}

// FUNCTION: SWBF2 0x00458960
inline void PblMatrix::Reset()
{
    D3DXMatrixIdentity(reinterpret_cast<D3DXMATRIX*>(this));
}

inline void PblMatrix::Transpose3x3()
{
    float fSwap = m[1][0];
    m[1][0] = m[0][1];
    m[0][1] = fSwap;
    fSwap = m[2][0];
    m[2][0] = m[0][2];
    m[0][2] = fSwap;
    fSwap = m[2][1];
    m[2][1] = m[1][2];
    m[1][2] = fSwap;
}

inline void PblMatrix::Transpose4x4()
{
    float t;
    t = m[0][1]; m[0][1] = m[1][0]; m[1][0] = t;
    t = m[0][2]; m[0][2] = m[2][0]; m[2][0] = t;
    t = m[0][3]; m[0][3] = m[3][0]; m[3][0] = t;
    t = m[1][2]; m[1][2] = m[2][1]; m[2][1] = t;
    t = m[1][3]; m[1][3] = m[3][1]; m[3][1] = t;
    t = m[2][3]; m[2][3] = m[3][2]; m[3][2] = t;
}

inline const PblMatrix PblMatrix::GetInverse() const
{
    PblMatrix result;
    D3DXMatrixInverse(reinterpret_cast<D3DXMATRIX*>(&result), 0,
                      reinterpret_cast<const D3DXMATRIX*>(this));
    return result;
}

// FUNCTION: SWBF2 0x00411dd0
inline const PblMatrix PblMatrix::GetInverseFast() const
{
    PblMatrix out;

    out = *this;

    out.Transpose3x3();

    PblVector3 vRotated;
    D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(&vRotated),
                            reinterpret_cast<const D3DXVECTOR3*>(&out.GetTranslation()),
                            reinterpret_cast<const D3DXMATRIX*>(&out));

    out.SetTranslation(PblVector3(-vRotated.x, -vRotated.y, -vRotated.z));

    return out;
}

// FUNCTION: SWBF2 0x00410890
inline const PblVector3& PblMatrix::GetTranslation() const { return *(const PblVector3*)&m[3][0]; }

inline const PblVector3& PblMatrix::GetXAxis() const { return *(const PblVector3*)&m[0][0]; }

inline const PblVector3& PblMatrix::GetYAxis() const { return *(const PblVector3*)&m[1][0]; }

// FUNCTION: SWBF2 0x0040cb70
inline const PblVector3& PblMatrix::GetZAxis() const { return *(const PblVector3*)&m[2][0]; }

inline void PblMatrix::SetXAxis(const PblVector3& vAxis)
{
    *reinterpret_cast<PblVector3*>(m[0]) = vAxis;
    m[0][3] = 0.0f;
}

inline void PblMatrix::SetYAxis(const PblVector3& vAxis)
{
    *reinterpret_cast<PblVector3*>(m[1]) = vAxis;
    m[1][3] = 0.0f;
}

inline void PblMatrix::SetZAxis(const PblVector3& vAxis)
{
    *reinterpret_cast<PblVector3*>(m[2]) = vAxis;
    m[2][3] = 0.0f;
}

inline void PblMatrix::SetTranslation(const PblVector3& vTranslation)
{
    *reinterpret_cast<PblVector3*>(m[3]) = vTranslation;
    m[3][3] = 1.0f;
}

inline void PblMatrix::SetXAxis(float fX, float fY, float fZ, float fW)
{
    m[0][0] = fX;
    m[0][1] = fY;
    m[0][2] = fZ;
    m[0][3] = fW;
}

inline void PblMatrix::SetYAxis(float fX, float fY, float fZ, float fW)
{
    m[1][0] = fX;
    m[1][1] = fY;
    m[1][2] = fZ;
    m[1][3] = fW;
}

inline void PblMatrix::SetZAxis(float fX, float fY, float fZ, float fW)
{
    m[2][0] = fX;
    m[2][1] = fY;
    m[2][2] = fZ;
    m[2][3] = fW;
}

inline void PblMatrix::SetTranslation(float fX, float fY, float fZ, float fW)
{
    m[3][0] = fX;
    m[3][1] = fY;
    m[3][2] = fZ;
    m[3][3] = fW;
}

inline float PblMatrix::operator()(int iRow, int iColumn) const
{
    return m[iRow][iColumn];
}

inline float& PblMatrix::operator()(int iRow, int iColumn)
{
    return m[iRow][iColumn];
}

inline const PblVector3& PblMatrix::operator[](int iRow) const
{
    return *reinterpret_cast<const PblVector3*>(m[iRow]);
}

inline PblVector3& PblMatrix::operator[](int iRow)
{
    return *reinterpret_cast<PblVector3*>(m[iRow]);
}

inline const PblVector3 PblMatrix::ApplyTranslation(const PblVector3& vector) const
{
    return PblVector3(m[3][0] + vector.x, m[3][1] + vector.y, m[3][2] + vector.z);
}

inline const PblMatrix& PblMatrix::operator*=(const PblMatrix& rhs)
{
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(this),
                       reinterpret_cast<const D3DXMATRIX*>(this),
                       reinterpret_cast<const D3DXMATRIX*>(&rhs));
    return *this;
}

inline const PblMatrix operator*(const PblMatrix& a, const PblMatrix& b)
{
    PblMatrix result;
    D3DXMatrixMultiply((D3DXMATRIX*)&result, (const D3DXMATRIX*)&a, (const D3DXMATRIX*)&b);
    return result;
}

inline const PblVector3 PblMatrix::ApplyTransform(const PblVector3& vector) const
{
    PblVector3 result;
    D3DXVec3TransformCoord(&result, &vector, reinterpret_cast<const D3DXMATRIX*>(this));
    return result;
}

// FUNCTION: SWBF2 0x00411e90
inline const PblVector3 PblMatrix::ApplyRotation(const PblVector3& vector) const
{
    PblVector3 result;
    D3DXVec3TransformNormal(&result, &vector, reinterpret_cast<const D3DXMATRIX*>(this));
    return result;
}

inline void PblMatrix::Translate(float fX, float fY, float fZ)
{
    m[3][0] += fX;
    m[3][1] += fY;
    m[3][2] += fZ;
}

inline void PblMatrix::RotateX(float fAngle)
{
    D3DXMATRIXA16 Temp;
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(this), reinterpret_cast<const D3DXMATRIX*>(this), D3DXMatrixRotationX(&Temp, fAngle));
}

inline void PblMatrix::RotateY(float fAngle)
{
    D3DXMATRIXA16 Temp;
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(this), reinterpret_cast<const D3DXMATRIX*>(this), D3DXMatrixRotationY(&Temp, fAngle));
}

inline void PblMatrix::RotateZ(float fAngle)
{
    D3DXMATRIXA16 Temp;
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(this), reinterpret_cast<const D3DXMATRIX*>(this), D3DXMatrixRotationZ(&Temp, fAngle));
}

inline void PblMatrix::SetRotateX(float fAngle)
{
    D3DXMatrixRotationX(reinterpret_cast<D3DXMATRIX*>(this), fAngle);
}

inline void PblMatrix::SetRotateY(float fAngle)
{
    D3DXMatrixRotationY(reinterpret_cast<D3DXMATRIX*>(this), fAngle);
}

inline void PblMatrix::SetRotateZ(float fAngle)
{
    D3DXMatrixRotationZ(reinterpret_cast<D3DXMATRIX*>(this), fAngle);
}

inline void PblMatrix::SetRotateArbitraryAxis(float fAngle, const PblVector3& vAxis)
{
    D3DXMatrixRotationAxis(reinterpret_cast<D3DXMATRIX*>(this),
                           reinterpret_cast<const D3DXVECTOR3*>(&vAxis), fAngle);
}

inline void PblMatrix::Scale(float fScaleX, float fScaleY, float fScaleZ)
{
    D3DXMATRIXA16 Temp;
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX*>(this), reinterpret_cast<const D3DXMATRIX*>(this), D3DXMatrixScaling(&Temp, fScaleX, fScaleY, fScaleZ));
}
