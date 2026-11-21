#pragma once

#include <d3dx9math.h>

namespace PcD3dx
{
D3DXMATRIX* MatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2);
D3DXVECTOR3* Vec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM);
D3DXVECTOR3* Vec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM);
D3DXVECTOR3* Vec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV);
D3DXQUATERNION* QuaternionNormalize(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ);
D3DXMATRIX* MatrixRotationQuaternion(D3DXMATRIX* pOut, const D3DXQUATERNION* pQ);
D3DXMATRIX* MatrixScaling(D3DXMATRIX* pOut, FLOAT fSx, FLOAT fSy, FLOAT fSz);
D3DXVECTOR4* Vec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM);
D3DXMATRIX* MatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, const D3DXMATRIX* pM);
}

extern "C"
{
D3DXMATRIX* PcD3dxMatrixRotationX(D3DXMATRIX* pOut, FLOAT fAngle);
D3DXMATRIX* PcD3dxMatrixRotationY(D3DXMATRIX* pOut, FLOAT fAngle);
D3DXMATRIX* PcD3dxMatrixRotationZ(D3DXMATRIX* pOut, FLOAT fAngle);
D3DXMATRIX* PcD3dxMatrixRotationAxis(D3DXMATRIX* pOut, const D3DXVECTOR3* pV, FLOAT fAngle);
D3DXVECTOR3* PcD3dxVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV);
}
