#include <d3dx9math.h>
#include "RedTerrain.h"
#include <new>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "Battlefront2/Source/GameMemory.h"
#include "Battlefront2/Source/CollisionObject.h"
#include "Battlefront2/Source/CollisionManager.h"
#include "RedWater.h"

#include <float.h>

bool PointInSphere(const PblVector3& vPoint, const PblVector3& vCenter, float fRadius);

// FUNCTION: SWBF2 0x006c21e0
bool PointInSphere(const PblVector3& vPoint, const PblVector3& vCenter, float fRadius)
{
    return SquaredLength(vPoint - vCenter) < fRadius * fRadius;
}


static const float EPSILON = 0.001f;

static inline int Log2(int i)
{
    int iLog = -1;
    for (; i; i >>= 1)
    {
        ++iLog;
    }
    return iLog;
}

// GLOBAL: SWBF2 0x0093e918
int RedTerrain::_iPointsPerDimensionLog2;
// GLOBAL: SWBF2 0x0093e91c
int RedTerrain::_iPatchesPerDimensionLog2;
// GLOBAL: SWBF2 0x0093e920
int RedTerrain::_iHeightPatchesPerDimensionLog2;
// GLOBAL: SWBF2 0x0093e924
int RedTerrain::_iPatchPointsPerDimensionLog2;
// GLOBAL: SWBF2 0x0093e928
int RedTerrain::_iHeightPatchPointsPerDimensionLog2;
// GLOBAL: SWBF2 0x0093e92c
int RedTerrain::_iNumPointsLog2;
// GLOBAL: SWBF2 0x0093e930
int RedTerrain::_iNumPatchesLog2;
// GLOBAL: SWBF2 0x0093e934
int RedTerrain::_iNumHeightPatchesLog2;
// GLOBAL: SWBF2 0x0093e938
int RedTerrain::_iNumHeightPatchPointsLog2;
// GLOBAL: SWBF2 0x0093e93c
int RedTerrain::_iPointsPerDimensionMask;
// GLOBAL: SWBF2 0x0093e940
int RedTerrain::_iPatchesPerDimensionMask;
// GLOBAL: SWBF2 0x0093e944
int RedTerrain::_iHeightPatchesPerDimensionMask;
// GLOBAL: SWBF2 0x0093e948
int RedTerrain::_iPatchPointsPerDimensionMask;
// GLOBAL: SWBF2 0x0093e94c
int RedTerrain::_iHeightPatchPointsPerDimensionMask;
// GLOBAL: SWBF2 0x0093e950
int RedTerrain::_iNumPointsMask;
// GLOBAL: SWBF2 0x0093e954
int RedTerrain::_iNumPatchesMask;
// GLOBAL: SWBF2 0x0093e958
int RedTerrain::_iNumHeightPatchesMask;
// GLOBAL: SWBF2 0x0093e95c
int RedTerrain::_iNumHeightPatchPointsMask;
// GLOBAL: SWBF2 0x0093e960
int RedTerrain::_iNumLayers;

// GLOBAL: SWBF2 0x0093e964
int RedTerrain::_iNumDecalTextures;
// GLOBAL: SWBF2 0x0093e968
int RedTerrain::_iNumDecalTiles;
// GLOBAL: SWBF2 0x0093e974
float RedTerrain::_fHeightScale;
// GLOBAL: SWBF2 0x0093e978
float RedTerrain::_fHeightMax;
// GLOBAL: SWBF2 0x0093e97c
float RedTerrain::_fHeightMin;
// GLOBAL: SWBF2 0x0093e980
float RedTerrain::_fMetersPerPoint;
// GLOBAL: SWBF2 0x0093e984
float RedTerrain::_fPointsPerMeter;

// GLOBAL: SWBF2 0x0093e988
float RedTerrain::_fTerrainCenterX;

// GLOBAL: SWBF2 0x0093e98c
float RedTerrain::_fTerrainCenterZ;
// GLOBAL: SWBF2 0x0093e990
unsigned char* RedTerrain::_puiHeight;
// GLOBAL: SWBF2 0x0093e994
unsigned int* RedTerrain::_puiScorch;
// GLOBAL: SWBF2 0x0093e998
unsigned char* RedTerrain::_puiLuminosity;
// GLOBAL: SWBF2 0x0093e99c
unsigned int RedTerrain::_uiDetailTextureHash;
// GLOBAL: SWBF2 0x0093e9a0
float RedTerrain::_fLayerMetersPerTexture[16];
// GLOBAL: SWBF2 0x0093e9e0
unsigned int RedTerrain::_uiLayerTextureHash[16];
// GLOBAL: SWBF2 0x0093ea20
unsigned int RedTerrain::_uiDecalTextureHash[16];

// GLOBAL: SWBF2 0x0093ea60
float RedTerrain::_uiLayerTextureRotation[16];
// GLOBAL: SWBF2 0x0093eaa0
unsigned char RedTerrain::_uiLayerTextureOrientation[16];
// GLOBAL: SWBF2 0x0093eab0
RedTerrain::DecalTile* RedTerrain::_pDecalTile;

// GLOBAL: SWBF2 0x0093eab5
bool RedTerrain::_bTreeHoles;

// GLOBAL: SWBF2 0x007deda0
bool RedTerrain::RenderDetail = true;
// GLOBAL: SWBF2 0x0093eabc
RedTerrain::HeightExpand* RedTerrain::_pHeightExpand;
// GLOBAL: SWBF2 0x0093eac0
RedTerrain::HeightMinMax* RedTerrain::_pHeightMinMax;

// GLOBAL: SWBF2 0x0093eac4
bool RedTerrain::mTerrainColorDarkeningFlag;
// GLOBAL: SWBF2 0x0093eac8
unsigned char* RedTerrain::_Foliage;
// GLOBAL: SWBF2 0x0093eb50
int RedTerrain::_iPointsPerDimension;
// GLOBAL: SWBF2 0x0093eb54
int RedTerrain::_iPatchesPerDimension;
// GLOBAL: SWBF2 0x0093eb58
int RedTerrain::_iHeightPatchesPerDimension;
// GLOBAL: SWBF2 0x0093eb5c
int RedTerrain::_iPatchPointsPerDimension;
// GLOBAL: SWBF2 0x0093eb60
int RedTerrain::_iHeightPatchPointsPerDimension;
// GLOBAL: SWBF2 0x0093eb64
int RedTerrain::_iNumPoints;
// GLOBAL: SWBF2 0x0093eb68
int RedTerrain::_iNumPatches;
// GLOBAL: SWBF2 0x0093eb6c
int RedTerrain::_iNumHeightPatches;

// GLOBAL: SWBF2 0x0093eb70
int RedTerrain::_iNumPatchPoints;
// GLOBAL: SWBF2 0x0093eb74
int RedTerrain::_iNumHeightPatchPoints;

// GLOBAL: SWBF2 0x0093eab7
bool sForceHiDetail;

// FUNCTION: SWBF2 0x004108b0
float RedTerrain::GetHeight(int iX, int iZ)
{
    if (!_bEnable)
    {
        return -1000.0f;
    }
    return (float)RedTerrain::GetHeightUnscaled(iX, iZ) * _fHeightScale;
}

// FUNCTION: SWBF2 0x006c1690
float RedTerrain::GetHoleHeight(float fX, float fZ)
{
    const PblVector3 vPos(fX, GetInterpolatedHeight(fX, fZ), fZ);
    if (PointInHole(vPos))
    {
        return -300.0f;
    }
    return vPos.y;
}

// FUNCTION: SWBF2 0x006c1720
float RedTerrain::GetInterpolatedHeight(float fX, float fZ)
{
    const float fMax = (float)(_iPointsPerDimension - 1) * _fMetersPerPoint;
    const float fPosX = Clamp(fX + _fTerrainCenterX, 0.0f, fMax);
    const float fPosZ = Clamp(fZ + _fTerrainCenterZ, 0.0f, fMax);

    const float fGridX = fPosX * _fPointsPerMeter;
    const float fGridZ = fPosZ * _fPointsPerMeter;
    const int iX = (int)fGridX;
    const int iZ = (int)fGridZ;
    const float fFracX = fGridX - (float)iX;
    const float fFracZ = fGridZ - (float)iZ;

    if (iZ & 1)
    {
        if (1.0f - fFracZ > fFracX)
        {
            const float fH0 = GetHeight(iX, iZ);
            const float fH = (GetHeight(iX + 1, iZ) - fH0) * fFracX + fH0;
            return fH + (GetHeight(iX, iZ + 1) - fH0) * fFracZ;
        }
        const float fH0 = GetHeight(iX + 1, iZ + 1);
        const float fH = (GetHeight(iX, iZ + 1) - fH0) * (1.0f - fFracX) + fH0;
        return fH + (GetHeight(iX + 1, iZ) - fH0) * (1.0f - fFracZ);
    }
    if (fFracZ > fFracX)
    {
        const float fH0 = GetHeight(iX, iZ + 1);
        const float fH = (GetHeight(iX + 1, iZ + 1) - fH0) * fFracX + fH0;
        return fH + (GetHeight(iX, iZ) - fH0) * (1.0f - fFracZ);
    }
    const float fH0 = GetHeight(iX + 1, iZ);
    const float fH = _mm_cvtss_f32(
        _mm_mul_ss(_mm_sub_ss(_mm_set_ss(GetHeight(iX + 1, iZ + 1)), _mm_set_ss(fH0)), _mm_set_ss(fFracZ)));
    return fH + ((GetHeight(iX, iZ) - fH0) * (1.0f - fFracX) + fH0);
}

// FUNCTION: SWBF2 0x006c19b0
float RedTerrain::GetInterpolatedLight(float fX, float fZ)
{
    const float fHalf = _fMetersPerPoint * 0.5f;
    const int iZ = (int)((fZ - fHalf + _fTerrainCenterZ) * _fPointsPerMeter + 0.5f);
    const float fFracZ = (fZ - _CalcWorldPosZ(iZ)) * _fPointsPerMeter;

    const float fL00 = GetLight(fX - fHalf, fZ - fHalf);
    const float fL10 = GetLight(fX + fHalf, fZ - fHalf);
    const float fL01 = GetLight(fX - fHalf, fZ + fHalf);
    const float fL11 = GetLight(fX + fHalf, fZ + fHalf);

    const int iX = (int)((fX - fHalf + _fTerrainCenterX) * _fPointsPerMeter + 0.5f);
    const float fFracX = (fX - _CalcWorldPosX(iX)) * _fPointsPerMeter;
    const float fA = (fL01 - fL00) * fFracZ + fL00;
    const float fB = (fL11 - fL10) * fFracZ + fL10;
    return (fB - fA) * fFracX + fA;
}



// FUNCTION: SWBF2 0x006c1b50
const PblVector3 RedTerrain::GetInterpolatedNormal(float fX, float fZ)
{
    const float fMax = (float)(_iPointsPerDimension - 1) * _fMetersPerPoint;
    const float fPosX = Clamp(fX + _fTerrainCenterX, 0.0f, fMax);
    const float fPosZ = Clamp(fZ + _fTerrainCenterZ, 0.0f, fMax);

    const float fGridX = fPosX * _fPointsPerMeter;
    const float fGridZ = fPosZ * _fPointsPerMeter;
    const int iX = (int)fGridX;
    const int iZ = (int)fGridZ;
    const float fFracX = fGridX - (float)iX;
    const float fFracZ = fGridZ - (float)iZ;
    const float fM = _fMetersPerPoint;

    if (iZ & 1)
    {
        if (1.0f - fFracZ > fFracX)
        {
            const PblVector3 p0(0.0f, GetHeight(iX, iZ), 0.0f);
            const PblVector3 a = PblVector3(fM, GetHeight(iX + 1, iZ), 0.0f) - p0;
            const PblVector3 b = PblVector3(0.0f, GetHeight(iX, iZ + 1), fM) - p0;
            return Normalize(CrossProduct(b, a));
        }
        else
        {
            const PblVector3 p0(fM, GetHeight(iX + 1, iZ + 1), fM);
            const PblVector3 a = PblVector3(0.0f, GetHeight(iX, iZ + 1), fM) - p0;
            const PblVector3 b = PblVector3(fM, GetHeight(iX + 1, iZ), 0.0f) - p0;
            return Normalize(CrossProduct(b, a));
        }
    }
    else if (fFracZ > fFracX)
    {
        const PblVector3 p0(0.0f, GetHeight(iX, iZ + 1), fM);
        const PblVector3 a = PblVector3(0.0f, GetHeight(iX, iZ), 0.0f) - p0;
        const PblVector3 b = PblVector3(fM, GetHeight(iX + 1, iZ + 1), fM) - p0;
        return Normalize(CrossProduct(b, a));
    }
    else
    {
        const PblVector3 p0(fM, GetHeight(iX + 1, iZ), 0.0f);
        const PblVector3 a = PblVector3(fM, GetHeight(iX + 1, iZ + 1), fM) - p0;
        const PblVector3 b = PblVector3(0.0f, GetHeight(iX, iZ), 0.0f) - p0;
        return Normalize(CrossProduct(b, a));
    }
}

static int ClampToGrid(float fGrid)
{
    if (!(fGrid >= 0.0f))
    {
        return 0;
    }
    if (fGrid > (float)(RedTerrain::_iPointsPerDimension - 1))
    {
        return RedTerrain::_iPointsPerDimension - 1;
    }
    return (int)fGrid;
}

// FUNCTION: SWBF2 0x006c1ee0
float RedTerrain::GetLight(float fX, float fZ)
{
    const int iX = ClampToGrid((fX + _fTerrainCenterX) * _fPointsPerMeter + 0.5f);
    const int iZ = ClampToGrid((fZ + _fTerrainCenterZ) * _fPointsPerMeter + 0.5f);
    return (float)_puiLuminosity[iX * _iPointsPerDimension + iZ] * (1.0f / 255.0f);
}

// FUNCTION: SWBF2 0x006c1f50
bool RedTerrain::PointInHole(const PblVector3& vPos)
{
    if (!_bEnable)
    {
        return false;
    }

    // GLOBAL: SWBF2 0x0093eb80
    static PblVector3 vLastPos(FLT_MAX, FLT_MAX, FLT_MAX);
    // GLOBAL: SWBF2 0x0093eb90
    static bool bLastInHole;
    if (vPos == vLastPos)
    {
        return bLastInHole;
    }
    vLastPos = vPos;
    bLastInHole = false;

    const int iX = ClampToGrid((vPos.x + _fTerrainCenterX) * _fPointsPerMeter);
    const int iZ = ClampToGrid((vPos.z + _fTerrainCenterZ) * _fPointsPerMeter);
    const int iPatch = (iX >> _iHeightPatchPointsPerDimensionLog2) +
                       ((iZ >> _iHeightPatchPointsPerDimensionLog2) << _iHeightPatchesPerDimensionLog2);
    const unsigned char uiHoles = _pHeightExpand[iPatch].uiFlags & 0xf;
    if (!uiHoles)
    {
        return false;
    }
    const int iQuadrant = ((iX >> (_iHeightPatchPointsPerDimensionLog2 - 1)) & 1) |
                          ((iZ >> (_iHeightPatchPointsPerDimensionLog2 - 2)) & 2);
    if (!(uiHoles & (1 << iQuadrant)))
    {
        return false;
    }

    if (_bTreeHoles)
    {
        HoleCallbackData data;
        data.vPoint = vPos;
        data.fRadius = 0.01f;
        data.pbInHole = &bLastInHole;
        CollisionManager::GetTreeGrid()->CallbackOnSphere(vPos, 0.01f, RedTerrain::PointInHoleCallback, &data);
    }
    else
    {
        for (PblList<Hole>::Itor it = _HoleList.Begin(); it; ++it)
        {
            Hole* pHole = it;
            if (PointInSphere(vPos, pHole->GetCollisionCenter(), pHole->GetCollisionRadius()) &&
                PointInVolume(vPos, pHole->aPlanes, pHole->iNumPlanes))
            {
                bLastInHole = true;
                break;
            }
        }
    }
    return bLastInHole;
}

// FUNCTION: SWBF2 0x006c2230
bool PointInVolume(const PblVector3& vPoint, const PblVector4* aPlanes, int iNumPlanes)
{
    for (int i = 0; i < iNumPlanes; ++i)
    {
        if (aPlanes[i].y * vPoint.y + aPlanes[i].x * vPoint.x + aPlanes[i].z * vPoint.z + aPlanes[i].w > 0.0f)
        {
            return false;
        }
    }
    return true;
}

// FUNCTION: SWBF2 0x006c22a0
void RedTerrain::ProcessHoles(void (__cdecl *callbackfn)(Hole*))
{
    if (!_bTreeHoles)
    {
        return;
    }
    for (PblList<Hole>::Itor it = _HoleList.Begin(); it; ++it)
    {
        Hole* pHole = it;
        callbackfn(pHole);
    }
}

// FUNCTION: SWBF2 0x006c22e0
float RedTerrain::RayHit(const PblVector3& vStart, const PblVector3& vEnd, float fA, float fB)
{
    if (!_bEnable)
    {
        return 1.0f;
    }

    const float fDeltaX = vStart.x - vEnd.x;
    const float fDeltaZ = vStart.z - vEnd.z;
    if ((fDeltaX < 0.0f ? -fDeltaX : fDeltaX) < EPSILON && (fDeltaZ < 0.0f ? -fDeltaZ : fDeltaZ) < EPSILON)
    {
        if (vEnd.y >= vStart.y)
        {
            return 1.0f;
        }
        const float fHeight = GetInterpolatedHeight(vStart.x, vStart.z);
        if (fHeight > vStart.y)
        {
            return 1.0f;
        }
        const float fT = (fHeight - vStart.y) / (vEnd.y - vStart.y);
        if (fT > 1.0f)
        {
            return 1.0f;
        }
        if (PointInHole(PblVector3(vStart.x, fHeight, vStart.z)))
        {
            return 1.0f;
        }
        return fT;
    }

    bool bAbove = true;
    if (GetInterpolatedHeight(vStart.x, vStart.z) > vStart.y)
    {
        bAbove = false;
    }
    return _RayHit(vStart, vEnd, fA, fB, bAbove);
}

// FUNCTION: SWBF2 0x006c2460
void RedTerrain::Read(PblFileChunk& chunk)
{
    const int iHeap = GameMemory::RunTimeHeap;
    DecalTile* pDecalTile = 0;
    HeightExpand* pHeightExpand = 0;
    HeightMinMax* pHeightMinMax = 0;
    unsigned char* pHeight = 0;
    char acName[256];

    for (int i = 0; i < 16; ++i)
    {
        _uiLayerTextureOrientation[i] = 0;
        _uiLayerTextureRotation[i] = 0;
    }

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('I', 'N', 'F', 'O'):
        {
            _bEnable = true;
            _fMetersPerPoint = child.ReadFloat();
            _fHeightScale = child.ReadFloat();
            _fHeightMin = child.ReadFloat();
            _fHeightMax = child.ReadFloat();
            _iPointsPerDimension = child.Read16();
            _iPatchPointsPerDimension = child.Read16();
            _iHeightPatchPointsPerDimension = child.Read16();
            _iNumLayers = child.Read16();
            _iNumDecalTextures = child.Read16();
            _iNumDecalTiles = child.Read16();

            _iPatchesPerDimension = _iPointsPerDimension / _iPatchPointsPerDimension;
            _iHeightPatchesPerDimension = _iPointsPerDimension / _iHeightPatchPointsPerDimension;
            _iNumPoints = _iPointsPerDimension * _iPointsPerDimension;
            _iNumPatches = _iPatchesPerDimension * _iPatchesPerDimension;
            _iNumHeightPatches = _iHeightPatchesPerDimension * _iHeightPatchesPerDimension;
            _iNumPatchPoints = (_iPatchPointsPerDimension + 1) * (_iPatchPointsPerDimension + 1);
            _iNumHeightPatchPoints = _iHeightPatchPointsPerDimension * _iHeightPatchPointsPerDimension;

            _iPointsPerDimensionLog2 = Log2(_iPointsPerDimension);
            _iPatchesPerDimensionLog2 = Log2(_iPatchesPerDimension);
            _iHeightPatchesPerDimensionLog2 = Log2(_iHeightPatchesPerDimension);
            _iPatchPointsPerDimensionLog2 = Log2(_iPatchPointsPerDimension);
            _iHeightPatchPointsPerDimensionLog2 = Log2(_iHeightPatchPointsPerDimension);
            _iNumPointsLog2 = Log2(_iNumPoints);
            _iNumPatchesLog2 = Log2(_iNumPatches);
            _iNumHeightPatchesLog2 = Log2(_iNumHeightPatches);
            _iNumHeightPatchPointsLog2 = Log2(_iNumHeightPatchPoints);

            _iPointsPerDimensionMask = _iPointsPerDimension - 1;
            _iPatchesPerDimensionMask = _iPatchesPerDimension - 1;
            _iHeightPatchesPerDimensionMask = _iHeightPatchesPerDimension - 1;
            _iPatchPointsPerDimensionMask = _iPatchPointsPerDimension - 1;
            _iHeightPatchPointsPerDimensionMask = _iHeightPatchPointsPerDimension - 1;
            _iNumPointsMask = _iNumPoints - 1;
            _iNumPatchesMask = _iNumPatches - 1;
            _iNumHeightPatchesMask = _iNumHeightPatches - 1;
            _iNumHeightPatchPointsMask = _iNumHeightPatchPoints - 1;

            _fPointsPerMeter = 1.0f / _fMetersPerPoint;
            _fTerrainCenterX = (float)_iPointsPerDimension * _fMetersPerPoint * 0.5f;
            _fTerrainCenterZ = _fTerrainCenterX - _fMetersPerPoint;

            const int iOldHeap = RedSetCurrentHeap(iHeap);
            pHeightExpand = new HeightExpand[_iNumHeightPatches];
            AdjustTerrainMemoryUsage(_iNumHeightPatches * sizeof(HeightExpand));
            pHeightMinMax = new HeightMinMax[_iNumHeightPatches];
            AdjustTerrainMemoryUsage(_iNumHeightPatches * sizeof(HeightMinMax));
            pHeight = new unsigned char[_iPointsPerDimension * _iPointsPerDimension];
            AdjustTerrainMemoryUsage(_iPointsPerDimension * _iPointsPerDimension);
            unsigned int* puiScorch =
                new unsigned int[_iPointsPerDimension * _iPointsPerDimension / 4u];
            AdjustTerrainMemoryUsage(_iPointsPerDimension * _iPointsPerDimension * sizeof(unsigned int) / 4);
            if (_iNumDecalTiles)
            {
                pDecalTile = new DecalTile[_iNumDecalTiles];
                AdjustTerrainMemoryUsage(_iNumDecalTiles * sizeof(DecalTile));
            }
            else
            {
                pDecalTile = 0;
            }
            RedSetCurrentHeap(iOldHeap);

            _pHeightExpand = pHeightExpand;
            _pDecalTile = pDecalTile;
            _pHeightMinMax = pHeightMinMax;
            _puiHeight = pHeight;
            _puiScorch = puiScorch;
            for (int i = 0; i < (int)(_iPointsPerDimension * _iPointsPerDimension / 4u); ++i)
            {
                puiScorch[i] = 0;
            }
            break;
        }

        case CHUNK_ID('L', 'T', 'E', 'X'):
            for (int i = 0; i < _iNumLayers; ++i)
            {
                child.ReadString(acName, sizeof(acName));
                _uiLayerTextureHash[i] = PblHash(acName);
            }
            break;

        case CHUNK_ID('D', 'T', 'E', 'X'):
            for (int i = 0; i < _iNumLayers; ++i)
            {
                child.ReadString(acName, sizeof(acName));
                _uiDecalTextureHash[i] = PblHash(acName);
            }
            break;

        case CHUNK_ID('D', 'T', 'L', 'X'):
            child.ReadString(acName, sizeof(acName));
            _uiDetailTextureHash = PblHash(acName);
            break;

        case CHUNK_ID('S', 'C', 'A', 'L'):
            for (int i = 0; i < _iNumLayers; ++i)
            {
                _fLayerMetersPerTexture[i] = child.ReadFloat();
            }
            break;

        case CHUNK_ID('A', 'X', 'I', 'S'):
            for (int i = 0; i < _iNumLayers; ++i)
            {
                _uiLayerTextureOrientation[i] = child.Read8();
            }
            break;

        case CHUNK_ID('H', 'E', 'X', 'P'):
            for (int i = 0; i < _iNumHeightPatches; ++i)
            {
                pHeightExpand[i].uiShift = child.Read8();
                pHeightExpand[i].uiFlags = child.Read8();
                pHeightExpand[i].iOffset = child.Read16();
                pHeightMinMax[i].iMin = child.Read16();
                pHeightMinMax[i].iMax = child.Read16();
            }
            break;

        case CHUNK_ID('H', 'G', 'T', '8'):
        {
            unsigned char* p = pHeight;
            unsigned char* const pEnd = pHeight + _iNumPoints;
            while (p < pEnd)
            {
                const unsigned char uiCount = child.Read8();
                int iRun;
                if (uiCount & 0x80)
                {
                    const unsigned char uiValue = child.Read8();
                    iRun = uiCount - 127;
                    memset(p, uiValue, iRun);
                }
                else
                {
                    iRun = uiCount + 1;
                    child.ReadData(p, iRun);
                }
                p += iRun;
            }
            break;
        }

        case CHUNK_ID('F', 'O', 'L', 'G'):
        {
            const int iSize = child.Read32();
            const int iOldHeap = RedSetCurrentHeap(iHeap);
            _Foliage = new unsigned char[iSize];
            RedSetCurrentHeap(iOldHeap);
            AdjustTerrainMemoryUsage(iSize);
            child.ReadData(_Foliage, iSize);
            break;
        }

        case CHUNK_ID('W', 'A', 'T', 'R'):
            RedWater::Read(child);
            break;

        case CHUNK_ID('D', 'C', 'A', 'L'):
            for (int i = 0; i < _iNumDecalTiles; ++i)
            {
                DecalTile& tile = pDecalTile[i];
                tile.uiGridX = child.Read8();
                tile.uiGridZ = child.Read8();
                tile.uiTextureIndex = child.Read8();
                child.Read8();
                child.Read8();
                child.Read8();
                child.Read8();
                child.Read8();
                tile.uiLLv = child.Read16();
                tile.uiLLu = child.Read16();
                tile.uiLRv = child.Read16();
                tile.uiLRu = child.Read16();
                tile.uiULv = child.Read16();
                tile.uiULu = child.Read16();
                tile.uiURv = child.Read16();
                tile.uiURu = child.Read16();
            }
            break;

        case CHUNK_ID('C', 'U', 'T', 'S'):
            while (!child.IsDone())
            {
                PblFileChunk cut = child.ReadNextChild();
                switch (cut._iId)
                {
                case CHUNK_ID('I', 'N', 'F', 'O'):
                    _bTreeHoles = cut.Read32() > 10;
                    break;

                case CHUNK_ID('C', 'U', 'T', 'R'):
                {
                    int iOldHeap = RedSetCurrentHeap(iHeap);
                    Hole* pHole = new Hole;
                    AdjustTerrainMemoryUsage(sizeof(Hole));
                    RedSetCurrentHeap(iOldHeap);

                    while (!cut.IsDone())
                    {
                        PblFileChunk part = cut.ReadNextChild();
                        switch (part._iId)
                        {
                        case CHUNK_ID('I', 'N', 'F', 'O'):
                        {
                            pHole->iNumPlanes = part.Read32();
                            const float fRadius = part.ReadFloat();
                            PblVector3 vCenter;
                            vCenter.SetX(part.ReadFloat());
                            vCenter.SetY(part.ReadFloat());
                            vCenter.SetZ(part.ReadFloat());
                            pHole->SetCollisionCenter(vCenter);
                            pHole->SetCollisionRadius(fRadius);

                            iOldHeap = RedSetCurrentHeap(iHeap);
                            pHole->aPlanes = new PblVector4[pHole->iNumPlanes];
                            AdjustTerrainMemoryUsage(pHole->iNumPlanes * sizeof(PblVector4));
                            RedSetCurrentHeap(iOldHeap);
                            break;
                        }

                        case CHUNK_ID('P', 'L', 'N', 'S'):
                            for (int i = 0; i < pHole->iNumPlanes; ++i)
                            {
                                pHole->aPlanes[i].SetX(part.ReadFloat());
                                pHole->aPlanes[i].SetY(part.ReadFloat());
                                pHole->aPlanes[i].SetZ(part.ReadFloat());
                                pHole->aPlanes[i].SetW(part.ReadFloat());
                            }
                            break;
                        }
                    }

                    _HoleList.Add(&pHole->node, pHole);
                    if (_bTreeHoles)
                    {
                        CollisionManager::GetTreeGrid()->Add(
                            pHole, pHole->GetCollisionCenter(), pHole->GetCollisionRadius() * 2.0f);
                    }
                    break;
                }
                }
            }
            break;
        case CHUNK_ID('L', 'U', 'M', 'I'):
        {
            const int iSize = child._iSize;
            const int iOldHeap = RedSetCurrentHeap(iHeap);
            unsigned char* pLum = new unsigned char[iSize];
            AdjustTerrainMemoryUsage(iSize);
            RedSetCurrentHeap(iOldHeap);
            _puiLuminosity = pLum;
            child.ReadData(pLum, iSize);
            break;
        }

        case CHUNK_ID('P', 'C', 'H', 'S'):
            _ReadPatchInfo(child);
            break;

        }
    }
}

// FUNCTION: SWBF2 0x006c3130
void RedTerrain::SetForceHiDetail(bool bForce)
{
    sForceHiDetail = bForce;
}

// FUNCTION: SWBF2 0x006c3140
float RedTerrain::_RayHit(const PblVector3& vStart, const PblVector3& vEnd, float fTolerance)
{
    float fBase = 0.0f;
    float fStep = 0.5f;
    PblVector3 vFrom = vStart;
    PblVector3 vTo = vEnd;
    const float fTolerance2 = fTolerance * fTolerance;

    while (SquaredLength(vFrom - vTo) > fTolerance2)
    {
        const PblVector3 vMid = (vTo + vFrom) * 0.5f;
        const float fAbove = GetInterpolatedHeight(vMid.x, vMid.z) - vMid.y;
        if (fAbove > fTolerance)
        {
            vTo = vMid;
        }
        else if (-fTolerance > fAbove)
        {
            vFrom = vMid;
            fBase += fStep;
        }
        else
        {
            break;
        }
        fStep *= 0.5f;
    }
    return fStep + fBase;
}

// FUNCTION: SWBF2 0x006c32f0
float RedTerrain::_RayHit(const PblVector3& vStart, const PblVector3& vEnd, float fA, float fB, bool& bAbove)
{
    if (SquaredLengthXZ(vStart - vEnd) > fB * fB)
    {
        const float fFirst = _RayHit(vStart, (vEnd + vStart) * 0.5f, fA, fB, bAbove);
        if (1.0f > fFirst)
        {
            return fFirst * 0.5f;
        }
        return _RayHit((vEnd + vStart) * 0.5f, vEnd, fA, fB, bAbove) * 0.5f + 0.5f;
    }

    if (GetInterpolatedHeight(vEnd.x, vEnd.z) > vEnd.y)
    {
        if (bAbove)
        {
            const float fT = _RayHit(vStart, vEnd, fA);
            if (!PointInHole(Interpolate(vStart, vEnd, fT)))
            {
                return fT;
            }
        }
        bAbove = false;
    }
    else
    {
        bAbove = true;
    }
    return 1.0f;
}

// FUNCTION: SWBF2 0x006c3110
void RedTerrain::Reset()
{
    _bEnable = false;
    _HoleList.Clear();
    mTerrainBytesUsed = 0;
}

// TEMPLATE: SWBF2 0x006c1650
// PblList<RedTerrain::Hole>::Clear
template void PblList<RedTerrain::Hole>::Clear();

// FUNCTION: SWBF2 0x004108f0
int RedTerrain::GetHeightUnscaled(int iX, int iZ)
{
    const int iPatch =
        ((iX & RedTerrain::_iPointsPerDimensionMask) >> RedTerrain::_iHeightPatchPointsPerDimensionLog2) +
        (((iZ & RedTerrain::_iPointsPerDimensionMask) >> RedTerrain::_iHeightPatchPointsPerDimensionLog2)
         << RedTerrain::_iHeightPatchesPerDimensionLog2);
    const RedTerrain::HeightExpand& expand = RedTerrain::_pHeightExpand[iPatch];
    const unsigned char* puiPatch =
        RedTerrain::_puiHeight + (iPatch << RedTerrain::_iNumHeightPatchPointsLog2);
    const int iPoint = (iX & RedTerrain::_iHeightPatchPointsPerDimensionMask) +
                       ((iZ & RedTerrain::_iHeightPatchPointsPerDimensionMask)
                        << RedTerrain::_iHeightPatchPointsPerDimensionLog2);
    return (puiPatch[iPoint] << expand.uiShift) + expand.iOffset;
}

// FUNCTION: SWBF2 0x006c21b0
bool RedTerrain::PointInHoleCallback(const TreeGridObject* pObject, TreeGridCallbackData* pData, float& fDist)
{
    const RedTerrain::Hole* pHole = (const RedTerrain::Hole*)pObject;
    HoleCallbackData* pHoleData = (HoleCallbackData*)pData;
    if (PointInVolume(pHoleData->vPoint, pHole->aPlanes, pHole->iNumPlanes))
    {
        *pHoleData->pbInHole = true;
        return false;
    }
    return true;
}

// GLOBAL: SWBF2 0x007dedb4
PblList<RedTerrain::Hole> RedTerrain::_HoleList;
// GLOBAL: SWBF2 0x0093eab4
bool RedTerrain::_bEnable;
// GLOBAL: SWBF2 0x0093eacc
int RedTerrain::mTerrainBytesUsed;
