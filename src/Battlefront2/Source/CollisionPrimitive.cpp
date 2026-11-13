#include "PebbleFL/Common/PblMath.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <d3dx9math.h>

#include "CollisionPrimitive.h"
#include "CollisionUtil.h"
#include "CollisionMesh.h"
#include "CollisionObject.h"
#include "EntitySoldier.h"
#include "RedEngineFL/Util/RedStack.h"
#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "RedEngineFL/Graphics/RedColor.h"


static inline PblVector3 ApplyRotation(const PblMatrix& m, const PblVector3& v)
{
    D3DXVECTOR3 r;
    D3DXVec3TransformNormal(&r, (const D3DXVECTOR3*)&v, (const D3DXMATRIX*)&m);
    return PblVector3(r.x, r.y, r.z);
}

static void AddCorners(PblVector3* pCorners, const PblVector3& x, const PblVector3& y, const PblVector3& z)
{
    for (int i = 0; i < 8; ++i)
    {
        float sx = (i & 4) ? -1.0f : 1.0f;
        float sy = (i & 2) ? -1.0f : 1.0f;
        float sz = (i & 1) ? -1.0f : 1.0f;
        pCorners[i].x += (sx * x.x + sy * y.x) + sz * z.x;
        pCorners[i].y += (sx * x.y + sy * y.y) + sz * z.y;
        pCorners[i].z += (sx * x.z + sy * y.z) + sz * z.z;
    }
}

// FUNCTION: SWBF2 0x004644b0
CollisionPrimitive::CollisionPrimitive()
    : mName(0)
{
}

// FUNCTION: SWBF2 0x004646a0
void CollisionPrimitive::SetSphere(float fRadius)
{
    mType = SPHERE;
    mHeight = fRadius * 2.0f;
    mRadius = fRadius;
    mMeshCollisionRadius = fRadius;
}

// FUNCTION: SWBF2 0x004646d0
void CollisionPrimitive::SetBubble(const PblVector3& vSize)
{
    mType = BUBBLE;
    mWidth = vSize.x;
    mHeight = vSize.y;
    mDepth = vSize.z;
    mRadius = mWidth;
    if (mHeight > mRadius)
    {
        mRadius = mHeight;
    }
    if (mDepth > mRadius)
    {
        mRadius = mDepth;
    }
    mMeshCollisionRadius = mRadius;
}

// FUNCTION: SWBF2 0x00464730
void CollisionPrimitive::SetAACylinder(float fRadius, float fHeight)
{
    mHeight = fHeight;
    mType = AXIS_ALIGNED_CYLINDER;
    mRadius = fRadius;
    mMeshCollisionRadius = fHeight * 0.5f;
}

// FUNCTION: SWBF2 0x00464750
void CollisionPrimitive::SetCylinder(float fRadius, float fHeight)
{
    mHeight = fHeight;
    mType = CYLINDER;
    mRadius = fRadius;
    float fHalfHeight = fHeight * 0.5f;
    mMeshCollisionRadius = (float)sqrt(fHalfHeight * fHalfHeight + fRadius * fRadius + fRadius * fRadius);
}

// FUNCTION: SWBF2 0x004647a0
void CollisionPrimitive::SetBox(float fWidth, float fHeight, float fDepth, float fRadius)
{
    mType = BOX;
    mWidth = fWidth;
    mDepth = fDepth;
    mHeight = fHeight;
    if (fRadius < 0.0f)
    {
        mRadius = (float)sqrt(fWidth * fWidth + fDepth * fDepth + fHeight * fHeight);
    }
    else
    {
        mRadius = fRadius;
    }
    mMeshCollisionRadius = (float)sqrt(fWidth * fWidth + fHeight * fHeight + fDepth * fDepth);
}

// FUNCTION: SWBF2 0x00464860
PblBox CollisionPrimitive::GetWorldspaceAABB(const PblMatrix& matrix) const
{
    float minx = FLT_MAX;
    float miny = FLT_MAX;
    float minz = FLT_MAX;
    float maxx = -FLT_MAX;
    float maxy = -FLT_MAX;
    float maxz = -FLT_MAX;
    PblVector3 corners[8];
    for (int i = 0; i < 8; ++i)
    {
        corners[i] = matrix.GetTranslation();
    }

    switch (mType)
    {
    case BOX:
    {
        PblVector3 x(mWidth, 0.0f, 0.0f);
        PblVector3 y(0.0f, mHeight, 0.0f);
        PblVector3 z(0.0f, 0.0f, mDepth);
        x = matrix.ApplyRotation(x);
        y = matrix.ApplyRotation(y);
        z = matrix.ApplyRotation(z);
        corners[0] += x + y + z;
        corners[1] += x + y + -z;
        corners[2] += x + -y + z;
        corners[3] += x + -y + -z;
        corners[4] += -x + y + z;
        corners[5] += -x + y + -z;
        corners[6] += -x + -y + z;
        corners[7] += -x + -y + -z;
        break;
    }
    case AXIS_ALIGNED_CYLINDER:
        break;
    case CYLINDER:
    {
        PblVector3 x(mRadius, 0.0f, 0.0f);
        PblVector3 y(0.0f, mHeight * 0.5f, 0.0f);
        PblVector3 z(0.0f, 0.0f, mRadius);
        x = matrix.ApplyRotation(x);
        y = matrix.ApplyRotation(y);
        z = matrix.ApplyRotation(z);
        corners[0] += x + y + z;
        corners[1] += x + y + -z;
        corners[2] += x + -y + z;
        corners[3] += x + -y + -z;
        corners[4] += -x + y + z;
        corners[5] += -x + y + -z;
        corners[6] += -x + -y + z;
        corners[7] += -x + -y + -z;
        break;
    }
    default:
    {
        PblVector3 vRadius;
        vRadius.SetAll(mRadius);
        corners[0] += vRadius;
        corners[1] -= vRadius;
        break;
    }
    }

    for (int i = 0; i < 8; ++i)
    {
        if (minx > corners[i].x)
        {
            minx = corners[i].x;
        }
        if (miny > corners[i].y)
        {
            miny = corners[i].y;
        }
        if (minz > corners[i].z)
        {
            minz = corners[i].z;
        }
        if (corners[i].x > maxx)
        {
            maxx = corners[i].x;
        }
        if (corners[i].y > maxy)
        {
            maxy = corners[i].y;
        }
        if (corners[i].z > maxz)
        {
            maxz = corners[i].z;
        }
    }

    PblBox worldaabb(minx, miny, minz, maxx, maxy, maxz);
    return worldaabb;
}

// FUNCTION: SWBF2 0x004652c0
bool CollisionPrimitive::CheckForCollision(const PblMatrix& mMatrix, const PblVector3& vVelocity,
                                           const CollisionPrimitive& other, const PblMatrix& otherMatrix,
                                           const PblVector3& otherVelocity, PblVector3& vNormal,
                                           float& fDepth) const
{
    switch (mType)
    {
    case SPHERE:
        return other.CollideSphere(otherVelocity, vVelocity, otherMatrix, mMatrix.GetTranslation(),
                                   mRadius, vNormal, fDepth);
    case BOX:
        return other.CollideBox(otherVelocity, vVelocity, otherMatrix, mMatrix,
                                mWidth, mHeight, mDepth, vNormal, fDepth);
    case AXIS_ALIGNED_CYLINDER:
        return other.CollideAACylinder(otherVelocity, vVelocity, otherMatrix, mMatrix.GetTranslation(),
                                       mRadius, mHeight, vNormal, fDepth);
    case CYLINDER:
        return other.CollideCylinder(otherVelocity, vVelocity, otherMatrix, mMatrix,
                                     mRadius, mHeight, vNormal, fDepth);
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x00465550
float CollisionPrimitive::RayHit(const PblMatrix& matrix, const PblVector3& vStart, const PblVector3& vDir,
                                 float fLength, PblVector3* pNormal, float fRadius)
{
    if (fRadius > 0.0f)
    {
        PblVector3 vS = vStart - vDir * fRadius;
        switch (mType)
        {
        case SPHERE:
            return CollisionUtil::RayHitSphere(vS, vDir, fLength, matrix.GetTranslation(), mRadius + fRadius,
                                               pNormal);
        case BUBBLE:
        {
            PblVector3 vSize(mWidth + fRadius, mHeight + fRadius, mDepth + fRadius);
            return CollisionUtil::RayHitBubble(vS, vDir, fLength, matrix, vSize, pNormal);
        }
        case BOX:
            return CollisionUtil::RayHitBox(vS, vDir, fLength, matrix, mWidth + fRadius, mDepth + fRadius, mHeight + fRadius,
                              pNormal);
        case AXIS_ALIGNED_CYLINDER:
            return CollisionUtil::RayHitAxisAlignedCylinder(vS, vDir, fLength, matrix.GetTranslation(), mRadius + fRadius,
                              mHeight * 0.5f + fRadius, pNormal);
        case CYLINDER:
            return CollisionUtil::RayHitCylinder(vS, vDir, fLength, matrix, mRadius + fRadius, mHeight * 0.5f + fRadius, pNormal);
        default:
            return 0.0f;
        }
    }

    switch (mType)
    {
    case SPHERE:
        return CollisionUtil::RayHitSphere(vStart, vDir, fLength, matrix.GetTranslation(), mRadius, pNormal);
    case BUBBLE:
    {
        PblVector3 vSize(mWidth, mHeight, mDepth);
        return CollisionUtil::RayHitBubble(vStart, vDir, fLength, matrix, vSize, pNormal);
    }
    case BOX:
        return CollisionUtil::RayHitBox(vStart, vDir, fLength, matrix, mWidth, mDepth, mHeight, pNormal);
    case AXIS_ALIGNED_CYLINDER:
        return CollisionUtil::RayHitAxisAlignedCylinder(vStart, vDir, fLength, matrix.GetTranslation(), mRadius, mHeight * 0.5f, pNormal);
    case CYLINDER:
        return CollisionUtil::RayHitCylinder(vStart, vDir, fLength, matrix, mRadius, mHeight * 0.5f, pNormal);
    default:
        return 0.0f;
    }
}



// GLOBAL: SWBF2 0x01eb9290
static PblMatrix sWorld2Collision;
// GLOBAL: SWBF2 0x01eb92d0
static PblMatrix sHostWorld2Collision;
// GLOBAL: SWBF2 0x01eb83d0
static PblMatrix sClientWorld2Collision;
// GLOBAL: SWBF2 0x01eb5394
static PblVector3 sLocalPos;
// GLOBAL: SWBF2 0x01eb3384
static PblVector3 sHostLocalPos;
// GLOBAL: SWBF2 0x01eb753c
static PblVector3 sClientLocalPos;
// GLOBAL: SWBF2 0x01f9c258
static float sRadius;
// GLOBAL: SWBF2 0x01f9c25c
static float sHostRadius;
// GLOBAL: SWBF2 0x01f9c260
static float sClientRadius;
// GLOBAL: SWBF2 0x01eb6680
static PblMatrix sPrimitiveInCollision;

extern PolyListContainer<100> sHostCollTriList;
extern PolyListContainer<100> sHostTriList;
extern PolyListContainer<100> sClientCollTriList;
extern PolyListContainer<100> sClientTriList;
// GLOBAL: SWBF2 0x007e6308
PolyListContainer<100>* sCollTriList = &sHostCollTriList;
// GLOBAL: SWBF2 0x007e630c
PolyListContainer<100>* sTriList = &sHostTriList;

// FUNCTION: SWBF2 0x004644c0
void CollisionPrimitive::EnterClient()
{
    sHostWorld2Collision = sWorld2Collision;
    sHostLocalPos = sLocalPos;
    sHostRadius = sRadius;
    sWorld2Collision = sClientWorld2Collision;
    sLocalPos = sClientLocalPos;
    sRadius = sClientRadius;
    sTriList = &sClientTriList;
    sCollTriList = &sClientCollTriList;
}

// FUNCTION: SWBF2 0x004645b0
void CollisionPrimitive::LeaveClient()
{
    sClientWorld2Collision = sWorld2Collision;
    sClientLocalPos = sLocalPos;
    sClientRadius = sRadius;
    sWorld2Collision = sHostWorld2Collision;
    sLocalPos = sHostLocalPos;
    sRadius = sHostRadius;
    sTriList = &sHostTriList;
    sCollTriList = &sHostCollTriList;
}

// FUNCTION: SWBF2 0x004653e0
bool CollisionPrimitive::RayTest(const PblMatrix& matrix, const PblVector3& vStart,
                                 const PblVector3& vDir, float fLength)
{
    switch (mType)
    {
    case SPHERE:
        return CollisionUtil::RayHitSphere(vStart, vDir, fLength, matrix.GetTranslation(), mRadius, 0) < 1.0f;
    case BUBBLE:
    {
        PblVector3 vSize(mWidth, mHeight, mDepth);
        return CollisionUtil::RayHitBubble(vStart, vDir, fLength, matrix, vSize, 0) < 1.0f;
    }
    case BOX:
        return CollisionUtil::RayHitBox(vStart, vDir, fLength, matrix, mWidth, mDepth, mHeight, 0) < 1.0f;
    case AXIS_ALIGNED_CYLINDER:
        return CollisionUtil::RayHitAxisAlignedCylinder(vStart, vDir, fLength, matrix.GetTranslation(), mRadius, mHeight * 0.5f, 0) < 1.0f;
    case CYLINDER:
        return CollisionUtil::RayHitCylinder(vStart, vDir, fLength, matrix, mRadius, mHeight * 0.5f, 0) < 1.0f;
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x004685b0
void CollisionPrimitive::Render(const PblMatrix& matrix) const
{
    switch (mType)
    {
    case BOX:
        CollisionUtil::RenderBox(matrix, mWidth, mHeight, mDepth, RedColor(255, 0, 255, 255));
        break;
    }
}

// FUNCTION: SWBF2 0x00465cf0
bool CollisionPrimitive::CollideSphere(const PblVector3&, const PblVector3&, const PblMatrix& matrix,
                                       const PblVector3& vCenter, float fRadius, PblVector3& vNormal,
                                       float& fDepth) const
{
    switch (mType)
    {
    case SPHERE:
        return CollisionUtil::CollideSpheres(matrix.GetTranslation(), mRadius, vCenter, fRadius, vNormal, fDepth);
    case BOX:
        return CollisionUtil::CollideBoxSphere(matrix, mWidth, mDepth, mHeight, vCenter, fRadius, vNormal, fDepth);
    case AXIS_ALIGNED_CYLINDER:
    {
        PblMatrix aligned = gMatrixIdentity;
        memcpy(aligned.m[3], matrix.m[3], 3 * sizeof(float));
        aligned.m[3][3] = 1.0f;
        return CollisionUtil::CollideCylinderSphere(aligned, mRadius, mHeight * 0.5f, vCenter, fRadius, vNormal, fDepth);
    }
    case CYLINDER:
        return CollisionUtil::CollideCylinderSphere(matrix, mRadius, mHeight * 0.5f, vCenter, fRadius, vNormal, fDepth);
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x004658a0
bool CollisionPrimitive::CollideCylinder(const PblVector3& vVelocity, const PblVector3& otherVelocity,
                                         const PblMatrix& matrix, const PblMatrix& cylinderMatrix, float fRadius,
                                         float fHeight, PblVector3& vNormal, float& fDepth) const
{
    switch (mType)
    {
    case SPHERE:
        return CollisionUtil::CollideSphereCylinder(matrix.GetTranslation(), mRadius, cylinderMatrix, fRadius,
                                                    fHeight, vNormal, fDepth);
    case BOX:
        return CollisionUtil::CollideBoxCylinder(vVelocity, matrix, mWidth, mDepth, mHeight, otherVelocity,
                                                 cylinderMatrix, fRadius, fHeight * 0.5f, vNormal, fDepth);
    case AXIS_ALIGNED_CYLINDER:
    {
        PblMatrix aligned = gMatrixIdentity;
        memcpy(aligned.m[3], matrix.m[3], 3 * sizeof(float));
        aligned.m[3][3] = 1.0f;
        return CollisionUtil::CollideCylinders(aligned, mRadius, mHeight * 0.5f, cylinderMatrix, fRadius, fHeight * 0.5f, vNormal,
                          fDepth);
    }
    case CYLINDER:
        return CollisionUtil::CollideCylinders(matrix, mRadius, mHeight * 0.5f, cylinderMatrix, fRadius, fHeight * 0.5f, vNormal,
                          fDepth);
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x00465a70
bool CollisionPrimitive::CollideAACylinder(const PblVector3& vVelocity, const PblVector3& otherVelocity,
                                           const PblMatrix& matrix, const PblVector3& vCenter, float fRadius,
                                           float fHeight, PblVector3& vNormal, float& fDepth) const
{
    switch (mType)
    {
    case SPHERE:
    {
        PblMatrix aligned = gMatrixIdentity;
        aligned.SetTranslation(vCenter);
        return CollisionUtil::CollideSphereCylinder(matrix.GetTranslation(), mRadius, aligned, fRadius, fHeight,
                                                    vNormal, fDepth);
    }
    case BOX:
    {
        PblMatrix aligned = gMatrixIdentity;
        aligned.SetTranslation(vCenter);
        return CollisionUtil::CollideBoxCylinder(vVelocity, matrix, mWidth, mDepth, mHeight, otherVelocity, aligned,
                                                 fRadius, fHeight * 0.5f, vNormal, fDepth);
    }
    case AXIS_ALIGNED_CYLINDER:
        vNormal = vCenter - matrix.GetTranslation();
        return CollisionUtil::CollideAxisAlignedCylinders(vNormal, mRadius, fRadius, mHeight, fHeight, fDepth);
    case CYLINDER:
    {
        PblMatrix aligned = gMatrixIdentity;
        aligned.SetTranslation(vCenter);
        return CollisionUtil::CollideCylinders(matrix, mRadius, mHeight * 0.5f, aligned, fRadius, fHeight * 0.5f, vNormal, fDepth);
    }
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x00465e70
bool CollisionPrimitive::CollideBox(const PblVector3& vVelocity, const PblVector3& otherVelocity,
                                    const PblMatrix& matrix, const PblMatrix& boxMatrix, float fWidth, float fHeight,
                                    float fDepth, PblVector3& vNormal, float& fPenetration) const
{
    switch (mType)
    {
    case SPHERE:
        return CollisionUtil::CollideSphereBox(matrix.GetTranslation(), mRadius, boxMatrix, fWidth, fDepth, fHeight, vNormal,
                          fPenetration);
    case BOX:
        return CollisionUtil::CollideBoxes(vVelocity, matrix, mWidth, mHeight, mDepth, otherVelocity, boxMatrix,
                                           fWidth, fHeight, fDepth, vNormal, fPenetration);
    case AXIS_ALIGNED_CYLINDER:
    {
        PblMatrix aligned = gMatrixIdentity;
        memcpy(aligned.m[3], matrix.m[3], 3 * sizeof(float));
        aligned.m[3][3] = 1.0f;
        return CollisionUtil::CollideCylinderBox(vVelocity, aligned, mRadius, mHeight * 0.5f, otherVelocity,
                                                 boxMatrix, fWidth, fDepth, fHeight, vNormal, fPenetration);
    }
    case CYLINDER:
        return CollisionUtil::CollideCylinderBox(vVelocity, matrix, mRadius, mHeight * 0.5f, otherVelocity,
                                                 boxMatrix, fWidth, fDepth, fHeight, vNormal, fPenetration);
    default:
        return false;
    }
}

// FUNCTION: SWBF2 0x00467090
bool CollisionPrimitive::InitMeshCollision(const PblMatrix& matrix, const CollisionMesh* pMesh,
                                           const PblMatrix& meshMatrix, CollisionObject* pObject,
                                           CollisionMeshCache*& pCache) const
{
    if (mType != AXIS_ALIGNED_CYLINDER && mType != SPHERE && mType != CYLINDER && mType != BOX)
    {
        return false;
    }

    sRadius = mMeshCollisionRadius;
    sWorld2Collision = meshMatrix.GetInverseFast();
    PblVector3 vTransformed;
    D3DXVec3TransformCoord((D3DXVECTOR3*)&vTransformed, (const D3DXVECTOR3*)&matrix.GetTranslation(),
                           (const D3DXMATRIX*)&sWorld2Collision);
    sLocalPos = vTransformed;
    D3DXVec3TransformNormal((D3DXVECTOR3*)&vTransformed, (const D3DXVECTOR3*)&matrix.m[0][0], (const D3DXMATRIX*)&sWorld2Collision);
    sPrimitiveInCollision.SetXAxis(vTransformed);
    D3DXVec3TransformNormal((D3DXVECTOR3*)&vTransformed, (const D3DXVECTOR3*)&matrix.m[1][0], (const D3DXMATRIX*)&sWorld2Collision);
    sPrimitiveInCollision.SetYAxis(vTransformed);
    PblVector3 vRow2;
    D3DXVec3TransformNormal((D3DXVECTOR3*)&vRow2, (const D3DXVECTOR3*)&matrix.m[2][0], (const D3DXMATRIX*)&sWorld2Collision);
    sPrimitiveInCollision.SetZAxis(vRow2);
    sPrimitiveInCollision.SetTranslation(sLocalPos);

    if (!CollisionObject::sCollidingWithMesh)
    {
        PblVector3 vBoxMin = pObject->mAABB._vMin;
        PblVector3 vBoxMax = pObject->mAABB._vMax;
        PblVector3 aCorners[8];
        int i;
        aCorners[0].x = vBoxMin.x;
        aCorners[0].y = vBoxMin.y;
        aCorners[0].z = vBoxMin.z;
        aCorners[1].x = vBoxMin.x;
        aCorners[1].y = vBoxMin.y;
        aCorners[1].z = vBoxMax.z;
        aCorners[2].x = vBoxMin.x;
        aCorners[2].y = vBoxMax.y;
        aCorners[2].z = vBoxMin.z;
        aCorners[3].x = vBoxMin.x;
        aCorners[3].y = vBoxMax.y;
        aCorners[3].z = vBoxMax.z;
        aCorners[4].x = vBoxMax.x;
        aCorners[4].y = vBoxMin.y;
        aCorners[4].z = vBoxMin.z;
        aCorners[5].x = vBoxMax.x;
        aCorners[5].y = vBoxMin.y;
        aCorners[5].z = vBoxMax.z;
        aCorners[6].x = vBoxMax.x;
        aCorners[6].y = vBoxMax.y;
        aCorners[6].z = vBoxMin.z;
        aCorners[7].x = vBoxMax.x;
        aCorners[7].y = vBoxMax.y;
        aCorners[7].z = vBoxMax.z;
        for (i = 0; i < 8; ++i)
        {
            D3DXVec3TransformCoord((D3DXVECTOR3*)&vTransformed, (const D3DXVECTOR3*)&aCorners[i],
                                   (const D3DXMATRIX*)&sWorld2Collision);
            aCorners[i] = vTransformed;
        }
        PblVector3 vMin = aCorners[0];
        PblVector3 vMax = aCorners[0];
        for (i = 1; i < 8; ++i)
        {
            if (vMin.x > aCorners[i].x)
            {
                vMin.x = aCorners[i].x;
            }
            else if (aCorners[i].x > vMax.x)
            {
                vMax.x = aCorners[i].x;
            }
            if (vMin.y > aCorners[i].y)
            {
                vMin.y = aCorners[i].y;
            }
            else if (aCorners[i].y > vMax.y)
            {
                vMax.y = aCorners[i].y;
            }
            if (vMin.z > aCorners[i].z)
            {
                vMin.z = aCorners[i].z;
            }
            else if (aCorners[i].z > vMax.z)
            {
                vMax.z = aCorners[i].z;
            }
        }

        sTriList->Clear();
        sCollTriList->Clear();
        EntitySoldier* pSoldier = NULL;
        if (pObject->GetEntity() && pObject->GetEntity()->IsRtti(EntitySoldier::rttiHashEntitySoldier))
        {
            pSoldier = (EntitySoldier*)pObject->GetEntity();
        }

        float fRadius = pObject->GetCollisionRadius();
        D3DXVec3TransformCoord((D3DXVECTOR3*)&vTransformed, (const D3DXVECTOR3*)&pObject->GetCollisionCenter(),
                               (const D3DXMATRIX*)&sWorld2Collision);
        pCache = ((CollisionMesh*)pMesh)->GetTriHits(vTransformed, fRadius, vMin, vMax, *sTriList, pSoldier);
    }

    for (int iPoly = 0; iPoly < sCollTriList->mPolyCount; ++iPoly)
    {
        int iNumVerts;
        const PblVector3* pVerts;
        sCollTriList->GetPolygon(iPoly, iNumVerts, pVerts);
        sTriList->AddPolygon((unsigned char)iNumVerts, pVerts);
    }
    sCollTriList->Clear();
    return sTriList->mPolyCount > 0;
}

// FUNCTION: SWBF2 0x00467550
void CollisionPrimitive::MoveMeshTestPos(const PblVector3& vMove)
{
    PblVector3 v = ApplyRotation(sWorld2Collision, vMove);
    sLocalPos.x += v.x;
    sLocalPos.y += v.y;
    sLocalPos.z += v.z;
}

// TEMPLATE: SWBF2 0x004685f0
// PolyListContainer<100>::GetPolygon
template void PolyListContainer<100>::GetPolygon(int, int&, const PblVector3*&);

// TEMPLATE: SWBF2 0x00468670
// PolyListContainer<100>::RemovePolygon
template void PolyListContainer<100>::RemovePolygon(int);

// FUNCTION: SWBF2 0x00466060
int CollisionPrimitive::GetClosestPoint(PblVector3& aMidPoint, float& aMaxDist,
    PolyListContainer<100>& aTriList, PblVector3& aOutPoint, PblVector3& aClosestCylPoint,
    const PblMatrix&, const PblMatrix&) const
{
    float trueMaxDist = -1.0f;
    int triangleId = -1;
    StackArray<PblVector3, 100> vertArray;
    if (mType == BOX)
    {
        aMaxDist = 1.0f;
    }

    for (int t = 0; t < aTriList.mPolyCount; ++t)
    {
        int numVertices;
        const PblVector3* corners;
        aTriList.GetPolygon(t, numVertices, corners);
        numVertices = Min(numVertices, 100);
        float cylDist = mRadius;
        if (mType == BOX)
        {
            PblVector3* verts = vertArray.GetPtr();
            memcpy(verts, corners, numVertices * sizeof(PblVector3));
            for (int s = 0; s < numVertices; ++s)
            {
                PblVector3 cornerDiff = verts[s] - aMidPoint;
                float dotY = DotProduct(cornerDiff, sPrimitiveInCollision.GetYAxis()) / mHeight;
                float dotX = DotProduct(cornerDiff, sPrimitiveInCollision.GetXAxis()) / mWidth;
                float dotZ = DotProduct(cornerDiff, sPrimitiveInCollision.GetZAxis()) / mDepth;
                verts[s] = dotX * sPrimitiveInCollision.GetXAxis() +
                    dotY * sPrimitiveInCollision.GetYAxis() +
                    dotZ * sPrimitiveInCollision.GetZAxis() + aMidPoint;
            }
            corners = verts;
        }

        PblVector3 outPos;
        if (!CollisionUtil::GetClosestPointInPolygon(aMidPoint, numVertices, corners, outPos, aMaxDist))
        {
            continue;
        }

        switch (mType)
        {
        case BOX:
        {
            PblVector3 outDiff = outPos - aMidPoint;
            float distortedLength = Length(outDiff);
            if (aMaxDist > distortedLength && distortedLength < 1.0f)
            {
                aMaxDist = distortedLength;
                float outDotX = DotProduct(outDiff, sPrimitiveInCollision.GetXAxis()) * mWidth;
                float outDotY = DotProduct(outDiff, sPrimitiveInCollision.GetYAxis()) * mHeight;
                float outDotZ = DotProduct(outDiff, sPrimitiveInCollision.GetZAxis()) * mDepth;
                aOutPoint = outDotX * sPrimitiveInCollision.GetXAxis() +
                    outDotY * sPrimitiveInCollision.GetYAxis() +
                    outDotZ * sPrimitiveInCollision.GetZAxis() + aMidPoint;
                trueMaxDist = Length(aOutPoint - aMidPoint);
                triangleId = t;
            }
            break;
        }
        case CYLINDER:
        {
            PblVector3 closestPoint;
            CollisionUtil::GetClosestPoint_SegmentPoint(
                sLocalPos - (mHeight * 0.5f) * sPrimitiveInCollision.GetYAxis(),
                (mHeight * 0.5f) * sPrimitiveInCollision.GetYAxis() + sLocalPos,
                outPos, closestPoint);
            PblVector3 outDiff = closestPoint - outPos;
            PblVector3 yAxis = sPrimitiveInCollision.GetYAxis();
            if (fabsf(DotProduct(outDiff, yAxis)) < 0.0001f &&
                cylDist * cylDist > SquaredLength(outDiff))
            {
                aClosestCylPoint = closestPoint;
                aOutPoint = outPos;
                triangleId = t;
                continue;
            }

            PblVector3 testDir = outPos - sLocalPos;
            if (DotProduct(testDir, yAxis) < 0.0f)
            {
                yAxis = -yAxis;
            }
            PblVector3 crossProd = CrossProduct(yAxis, testDir);
            PblVector3 radAxis = Normalize(CrossProduct(crossProd, yAxis));
            if (DotProduct(testDir, radAxis) < 0.0f)
            {
                radAxis = -radAxis;
            }
            PblVector3 endPointCyl = (mHeight * 0.5f) * yAxis + sLocalPos + mRadius * radAxis;
            PblVector3 testAxis = Normalize(CrossProduct(testDir, crossProd));
            PblVector3 endPointTest = DotProduct(endPointCyl - sLocalPos, testAxis) * testAxis + outPos;
            PblVector3 cylOut;
            PblVector3 testOut;
            CollisionUtil::GetClosestPoints_SegmentSegment(sLocalPos, endPointCyl, outPos,
                endPointTest, cylOut, testOut);
            if (!(SquaredLength(cylOut - testOut) < 0.0001f))
            {
                continue;
            }

            PblVector3 polyNorm = CrossProduct(corners[1] - corners[0], corners[2] - corners[0]);
            bool bInside = true;
            int vertNum = 0;
            while (bInside && vertNum < numVertices - 1)
            {
                if (0.0f > DotProduct(CrossProduct(corners[vertNum] - cylOut,
                    corners[vertNum + 1] - cylOut), polyNorm))
                {
                    bInside = false;
                }
                else
                {
                    ++vertNum;
                }
            }
            if (!bInside)
            {
                continue;
            }

            CollisionUtil::GetClosestPoint_SegmentPoint(
                sLocalPos - (mHeight * 0.5f) * sPrimitiveInCollision.GetYAxis(),
                (mHeight * 0.5f) * sPrimitiveInCollision.GetYAxis() + sLocalPos,
                cylOut, closestPoint);
            aClosestCylPoint = closestPoint;
            aOutPoint = cylOut;
            triangleId = t;
            break;
        }
        default:
            if (triangleId < 0 || aMaxDist * aMaxDist > SquaredLength(outPos - aMidPoint))
            {
                aMaxDist = Length(outPos - aMidPoint);
                aOutPoint = outPos;
                triangleId = t;
            }
            break;
        }
    }

    if (mType == BOX)
    {
        aMaxDist = trueMaxDist;
    }
    return triangleId;
}

// FUNCTION: SWBF2 0x004675b0
bool CollisionPrimitive::CollideMesh(const PblMatrix& matrix, const CollisionMesh* pMesh,
    const PblMatrix& meshMatrix, PblVector3& vCollisionDir, float& fCollisionOverlap,
    float& fClosest) const
{
    fClosest = sRadius;
    PblVector3 closestPos;
    PblVector3 closestCylPos;
    int collTriId = GetClosestPoint(sLocalPos, fClosest, *sTriList, closestPos, closestCylPos,
        matrix, meshMatrix);
    if (collTriId < 0)
    {
        return false;
    }

    if (mType == BOX)
    {
        vCollisionDir = sLocalPos - closestPos;
        float dotX1 = DotProduct(vCollisionDir, sPrimitiveInCollision.GetXAxis());
        float dotY1 = DotProduct(vCollisionDir, sPrimitiveInCollision.GetYAxis());
        float dotZ1 = DotProduct(vCollisionDir, sPrimitiveInCollision.GetZAxis());
        PblVector3 unitDiff = (dotX1 / mWidth) * sPrimitiveInCollision.GetXAxis() +
            (dotY1 / mHeight) * sPrimitiveInCollision.GetYAxis() +
            (dotZ1 / mDepth) * sPrimitiveInCollision.GetZAxis();
        float dotX2 = fabsf(DotProduct(unitDiff, sPrimitiveInCollision.GetXAxis()));
        float dotY2 = fabsf(DotProduct(unitDiff, sPrimitiveInCollision.GetYAxis()));
        float dotZ2 = fabsf(DotProduct(unitDiff, sPrimitiveInCollision.GetZAxis()));
        if (dotX2 > dotY2 && dotX2 > dotZ2)
        {
            vCollisionDir = dotX1 < 0.0f ? -sPrimitiveInCollision.GetXAxis() :
                sPrimitiveInCollision.GetXAxis();
            fCollisionOverlap = mWidth - fabsf(dotX1);
        }
        else if (dotY2 > dotZ2)
        {
            vCollisionDir = dotY1 < 0.0f ? -sPrimitiveInCollision.GetYAxis() :
                sPrimitiveInCollision.GetYAxis();
            fCollisionOverlap = mHeight - fabsf(dotY1);
        }
        else
        {
            vCollisionDir = dotZ1 < 0.0f ? -sPrimitiveInCollision.GetZAxis() :
                sPrimitiveInCollision.GetZAxis();
            fCollisionOverlap = mDepth - fabsf(dotZ1);
        }
    }
    else if (mType == CYLINDER)
    {
        float radiusOverlap = mRadius - Length(closestCylPos - closestPos);
        float heightOverlap = mHeight * 0.5f - fabsf(DotProduct(closestPos - sLocalPos,
            sPrimitiveInCollision.GetYAxis()));
        if (!(mHeight * 0.5f - heightOverlap < 0.0001f))
        {
            vCollisionDir = sLocalPos - closestPos;
            float dist = Length(vCollisionDir);
            float tmpOverlap = dist / (mHeight * 0.5f - heightOverlap) * heightOverlap;
            float testSq = (mHeight * 0.25f * mHeight + mRadius * mRadius) * 0.8f;
            if (!((tmpOverlap + dist) * (tmpOverlap + dist) <= testSq))
            {
                fCollisionOverlap = radiusOverlap;
                vCollisionDir = Normalize(closestCylPos - closestPos);
            }
            else
            {
                fCollisionOverlap = tmpOverlap;
                vCollisionDir /= dist;
            }
        }
        else
        {
            fCollisionOverlap = radiusOverlap;
            vCollisionDir = Normalize(closestCylPos - closestPos);
        }
    }
    else
    {
        vCollisionDir = sLocalPos - closestPos;
        fCollisionOverlap = Length(vCollisionDir);
        if (fCollisionOverlap < 0.0001f)
        {
            return false;
        }
        vCollisionDir /= fCollisionOverlap;
        fCollisionOverlap = sRadius - fCollisionOverlap;
    }

    vCollisionDir = meshMatrix.ApplyRotation(vCollisionDir);
    if (CollisionObject::sCollideTrisWithAllPrimitives)
    {
        int numVertices;
        const PblVector3* vertices = NULL;
        sTriList->GetPolygon(collTriId, numVertices, vertices);
        sCollTriList->AddPolygon((unsigned char)numVertices, vertices);
    }
    sTriList->RemovePolygon(collTriId);
    return true;
}


// FUNCTION: SWBF2 0x00467dd0
bool CollisionPrimitive::CollideTerrain(const PblMatrix& matrix, PblVector3& vNormal, float& fDepth,
                                        PblVector3& vPos) const
{
    const PblVector3& vTrans = matrix.GetTranslation();
    float collHeight = mHeight * 0.5f;
    float terrainHeight;
    PblVector3 radiusDir(0.0f, 0.0f, 0.0f);
    vPos = vTrans - PblVector3(0.0f, collHeight, 0.0f);
    PblVector3 pos = vPos;

    if (mType == SPHERE)
    {
        collHeight = mRadius;
        vPos = vTrans - PblVector3(0.0f, mRadius, 0.0f);
    }
    else if (mType == CYLINDER)
    {
        const float fHalfHeight = mHeight * 0.5f;
        const float fA = matrix.m[0][1] * mRadius;
        const float fB = matrix.m[1][1] * fHalfHeight;
        const float fC = matrix.m[2][1] * mRadius;
        PblVector3 tmp(fA, fB, fC);
        collHeight = sqrtf(DotProduct(tmp, tmp));

        PblVector3 vDir = matrix.GetYAxis();
        if (vDir.y > 0.0f)
        {
            vDir = -vDir;
        }
        vPos = vTrans + fHalfHeight * vDir;

        radiusDir = CrossProduct(CrossProduct(vDir, PblVector3(0.0f, -1.0f, 0.0f)), vDir);
        if (DotProduct(radiusDir, radiusDir) > 0.01f)
        {
            radiusDir = Normalize(radiusDir);
        }
        vPos += mRadius * radiusDir;
        pos = vPos;
        terrainHeight = RedTerrain::GetHoleHeight(pos.x, pos.z);
        fDepth = terrainHeight - vTrans.y + collHeight;
        if (fDepth < 0.0f || fDepth > collHeight * 4.0f)
        {
            const PblVector3 vRim = vPos;
            vPos -= mHeight * vDir;
            pos = vPos;
            terrainHeight = RedTerrain::GetHoleHeight(pos.x, pos.z);
            fDepth = terrainHeight - vTrans.y + collHeight;
            if (fDepth < 0.0f || fDepth > collHeight * 4.0f)
            {
                const float fDiameter = mRadius * 2.0f;
                vPos = vRim - fDiameter * radiusDir;
                pos = vPos;
                terrainHeight = RedTerrain::GetHoleHeight(pos.x, pos.z);
                fDepth = terrainHeight - vTrans.y + collHeight;
            }
        }
    }
    else if (mType == BOX)
    {

        PblVector3 xAxis = matrix.GetXAxis();
        PblVector3 yAxis = matrix.GetYAxis();
        PblVector3 zAxis = matrix.GetZAxis();
        PblVector3 dirs(0.0f, 0.0f, 0.0f);
        if (xAxis.y < -0.0001f)
        {
            dirs.x = mWidth;
        }
        else if (xAxis.y > 0.0001f)
        {
            dirs.x = -mWidth;
        }
        if (yAxis.y < -0.0001f)
        {
            dirs.y = mHeight;
        }
        else if (yAxis.y > 0.0001f)
        {
            dirs.y = -mHeight;
        }
        if (zAxis.y < -0.0001f)
        {
            dirs.z = mDepth;
        }
        else if (zAxis.y > 0.0001f)
        {
            dirs.z = -mDepth;
        }
        vPos = vTrans + dirs.x * xAxis + dirs.y * yAxis + dirs.z * zAxis;
        collHeight = vTrans.y - vPos.y;
    }

    if (mType != CYLINDER)
    {
        pos = vPos;
        terrainHeight = RedTerrain::GetHoleHeight(pos.x, pos.z);
        fDepth = terrainHeight - vTrans.y + collHeight;
    }
    if (fDepth < 0.0f || fDepth > collHeight * 2.0f)
    {
        return false;
    }

    vNormal = RedTerrain::GetInterpolatedNormal(pos.x, pos.z);
    if (mType == CYLINDER)
    {

        const float fDot = DotProduct(matrix.GetYAxis(), vNormal);
        if (1.0f - fabsf(fDot) < 0.0001f)
        {
            vPos -= mRadius * radiusDir;
        }
    }
    fDepth = DotProduct(vNormal, PblVector3(0.0f, fDepth, 0.0f));
    return true;
}

// SYNTHETIC: SWBF2 0x004024e0
// `dynamic initializer for 'sHostTriList''

// GLOBAL: SWBF2 0x01eb7548
// sHostTriList
PolyListContainer<100> sHostTriList;

// SYNTHETIC: SWBF2 0x004024f0
// `dynamic initializer for 'sHostCollTriList''

// GLOBAL: SWBF2 0x01eb5800
// sHostCollTriList
PolyListContainer<100> sHostCollTriList;

// SYNTHETIC: SWBF2 0x00402500
// `dynamic initializer for 'sClientTriList''

// GLOBAL: SWBF2 0x01eb66c0
// sClientTriList
PolyListContainer<100> sClientTriList;

// SYNTHETIC: SWBF2 0x00402510
// `dynamic initializer for 'sClientCollTriList''

// GLOBAL: SWBF2 0x01eb8410
// sClientCollTriList
PolyListContainer<100> sClientCollTriList;
