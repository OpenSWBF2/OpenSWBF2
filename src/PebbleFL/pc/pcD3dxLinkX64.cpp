#include <d3dx9math.h>

#include "PebbleFL/pc/pcD3dxPort.h"

extern "C"
{
D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX* pOut, CONST D3DXMATRIX* pM1, CONST D3DXMATRIX* pM2)
{
    return PcD3dx::MatrixMultiply(pOut, pM1, pM2);
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3* pOut, CONST D3DXVECTOR3* pV, CONST D3DXMATRIX* pM)
{
    return PcD3dx::Vec3TransformCoord(pOut, pV, pM);
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3* pOut, CONST D3DXVECTOR3* pV, CONST D3DXMATRIX* pM)
{
    return PcD3dx::Vec3TransformNormal(pOut, pV, pM);
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pOut, CONST D3DXVECTOR3* pV)
{
    return PcD3dx::Vec3Normalize(pOut, pV);
}

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize(D3DXQUATERNION* pOut, CONST D3DXQUATERNION* pQ)
{
    return PcD3dx::QuaternionNormalize(pOut, pQ);
}

D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion(D3DXMATRIX* pOut, CONST D3DXQUATERNION* pQ)
{
    return PcD3dx::MatrixRotationQuaternion(pOut, pQ);
}

D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX* pOut, FLOAT sx, FLOAT sy, FLOAT sz)
{
    return PcD3dx::MatrixScaling(pOut, sx, sy, sz);
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4* pOut, CONST D3DXVECTOR4* pV, CONST D3DXMATRIX* pM)
{
    return PcD3dx::Vec4Transform(pOut, pV, pM);
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, CONST D3DXMATRIX* pM)
{
    return PcD3dx::MatrixInverse(pOut, pDeterminant, pM);
}

D3DXMATRIX* WINAPI D3DXMatrixRotationX(D3DXMATRIX* pOut, FLOAT Angle)
{
    return PcD3dxMatrixRotationX(pOut, Angle);
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX* pOut, FLOAT Angle)
{
    return PcD3dxMatrixRotationY(pOut, Angle);
}

D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX* pOut, FLOAT Angle)
{
    return PcD3dxMatrixRotationZ(pOut, Angle);
}

D3DXMATRIX* WINAPI D3DXMatrixRotationAxis(D3DXMATRIX* pOut, CONST D3DXVECTOR3* pV, FLOAT Angle)
{
    return PcD3dxMatrixRotationAxis(pOut, pV, Angle);
}
}
