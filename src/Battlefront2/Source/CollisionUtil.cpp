#include <math.h>
#include <d3dx9math.h>

#include "CollisionUtil.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblMatrix.h"

// FUNCTION: SWBF2 0x0046c020
void CollisionUtil::RenderBox(const PblMatrix&, float, float, float, RedColor)
{
}

// FUNCTION: SWBF2 0x00469060
bool CollisionUtil::CollideSphereAABB(const PblVector3& rCenter, float fRadius,
                                      const PblVector3& rMin, const PblVector3& rMax)
{
    float fDist2 = 0.0f;
    if (rCenter.x < rMin.x)
    {
        fDist2 += (rCenter.x - rMin.x) * (rCenter.x - rMin.x);
    }
    else if (rCenter.x > rMax.x)
    {
        fDist2 += (rCenter.x - rMax.x) * (rCenter.x - rMax.x);
    }
    if (rCenter.y < rMin.y)
    {
        fDist2 += (rCenter.y - rMin.y) * (rCenter.y - rMin.y);
    }
    else if (rCenter.y > rMax.y)
    {
        fDist2 += (rCenter.y - rMax.y) * (rCenter.y - rMax.y);
    }
    if (rCenter.z < rMin.z)
    {
        fDist2 += (rCenter.z - rMin.z) * (rCenter.z - rMin.z);
    }
    else if (rCenter.z > rMax.z)
    {
        fDist2 += (rCenter.z - rMax.z) * (rCenter.z - rMax.z);
    }
    return fRadius * fRadius >= fDist2;
}

// FUNCTION: SWBF2 0x004690f0
bool CollisionUtil::CollideSphereRay(const PblVector3& rCenter, float fRadius,
                                     const PblVector3& rFrom, const PblVector3& rDir, float fLength)
{
    PblVector3 vDelta = rCenter - rFrom;
    PblVector3 vDir = rDir;
    float fProj = DotProduct(vDelta, vDir);
    float fDist2 = DotProduct(vDelta, vDelta);
    float fRadius2 = fRadius * fRadius;

    if (fRadius2 > fDist2)
    {
        return true;
    }
    if (fProj < 0.0f && fDist2 > fRadius2)
    {
        return false;
    }
    float fPerp2 = fDist2 - fProj * fProj;
    if (fPerp2 > fRadius2)
    {
        return false;
    }
    float fHalfChord = (float)sqrt((double)(fRadius2 - fPerp2));
    float fHit = fDist2 > fRadius2 ? fProj - fHalfChord : fProj + fHalfChord;
    if (fHit < 0.0f)
    {
        return false;
    }
    if (fHit > fLength)
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x0046a810
bool CollisionUtil::CollideSpheres2(const PblVector3& rCenter1, float fRadius1,
                                    const PblVector3& rCenter2, float fRadius2)
{
    PblVector3 vDelta = rCenter2 - rCenter1;
    float fRadii = fRadius1 + fRadius2;
    float fDist2 = SquaredLength(vDelta);
    if (fDist2 >= fRadii * fRadii)
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x0046ae60
int FindAACenteredBoxRayIntersection(const PblVector3& vStart, const PblVector3& vDir, float fLength,
                                     float fWidth, float fHeight, float fDepth, float& fHit)
{
    bool bInside = true;
    PblVector3 vExtent(fWidth, fHeight, fDepth);
    float aPlane[3];
    int aSide[3];
    int i;
    for (i = 0; i < 3; i++)
    {
        if (-vExtent[i] > vStart[i])
        {
            aSide[i] = -1;
            aPlane[i] = -vExtent[i];
            bInside = false;
        }
        else if (vStart[i] > vExtent[i])
        {
            aSide[i] = 1;
            aPlane[i] = vExtent[i];
            bInside = false;
        }
        else
        {
            aSide[i] = 0;
            aPlane[i] = vExtent[i];
        }
    }
    if (bInside)
    {
        return 0;
    }

    PblVector3 vRay = vDir * fLength;
    float aT[3];
    for (i = 0; i < 3; i++)
    {
        if (aSide[i] != 0 && fabsf(vRay[i]) > 0.0001f)
        {
            aT[i] = (aPlane[i] - vStart[i]) / vRay[i];
        }
        else
        {
            aT[i] = -1.0f;
        }
    }

    for (i = 0; i < 3; i++)
    {
        if (aT[i] > 0.0f)
        {
            PblVector3 vHit = vStart + vRay * aT[i];
            for (int j = 0; j < 3; j++)
            {
                if (j != i && (-vExtent[j] > vHit[j] || vHit[j] > vExtent[j]))
                {
                    aT[i] = -1.0f;
                    break;
                }
            }
        }
    }

    int nPlane = 0;
    if (aT[1] > 0.0f && (aT[nPlane] > aT[1] || aT[nPlane] < 0.0f))
    {
        nPlane = 1;
    }
    if (aT[2] > 0.0f && (aT[nPlane] > aT[2] || aT[nPlane] < 0.0f))
    {
        nPlane = 2;
    }
    if (aT[nPlane] < 0.0f)
    {
        return 0;
    }
    fHit = aT[nPlane];
    return aSide[nPlane] * (nPlane + 1);
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x00469670
bool IsPointInsideAxisAlignedCylinder(const PblVector3& vOffset, float fRadius, float fHalfHeight)
{
    float fY = vOffset.y;
    float fAbsY = fY < 0.0f ? -fY : fY;
    if (fAbsY > fHalfHeight)
    {
        return false;
    }
    return SquaredLengthXZ(vOffset) < fRadius * fRadius;
}
}

static inline PblVector3 ToLocal(const PblMatrix& m, const PblVector3& v)
{
    return PblVector3(m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z,
                      m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z,
                      m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z);
}

static inline PblVector3 ToWorld(const PblMatrix& m, float x, float y, float z)
{
    return PblVector3(m.m[0][0] * x + m.m[1][0] * y + m.m[2][0] * z,
                      m.m[0][1] * x + m.m[1][1] * y + m.m[2][1] * z,
                      m.m[0][2] * x + m.m[1][2] * y + m.m[2][2] * z);
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046a700
bool CollideSpheres(const PblVector3& vCenter, float fRadius, const PblVector3& vSphereCenter, float fSphereRadius,
                PblVector3& vNormal, float& fDepth)
{
    float fDx = vSphereCenter.x - vCenter.x;
    float fDy = vSphereCenter.y - vCenter.y;
    float fDz = vSphereCenter.z - vCenter.z;
    PblVector3 vDelta(fDx, fDy, fDz);
    float fRadii = fRadius + fSphereRadius;
    float fDist = SquaredLength(vDelta);
    if (fDist >= fRadii * fRadii)
    {
        return false;
    }
    if (fDist > 0.0f)
    {
        fDist = sqrtf(fDist);
    }
    fDepth = fRadii - fDist;
    if (fDist > 0.0f)
    {
        vNormal = vDelta / fDist;
    }
    else
    {
        vNormal.x = 0.0f;
        vNormal.y = 0.0f;
        vNormal.z = 1.0f;
    }
    return true;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x00469e90
bool CollideBoxSphere(const PblMatrix& matrix, float fWidth, float fDepth, float fHeight, const PblVector3& vSphereCenter,
                float fSphereRadius, PblVector3& vNormal, float& fPenetration)
{
    PblVector3 vAxis[3] = {matrix.GetXAxis(), matrix.GetYAxis(), matrix.GetZAxis()};
    PblVector3 vOffset = vSphereCenter - matrix.GetTranslation();
    PblVector3 vLocal(DotProduct(vAxis[0], vOffset), DotProduct(vAxis[1], vOffset), DotProduct(vAxis[2], vOffset));
    bool bInside = true;
    int iSideX, iSideY, iSideZ;
    PblVector3 vClamp;

    if (-fWidth > vLocal.x)
    {
        iSideX = -1;
        vClamp.x = -fWidth;
        bInside = false;
    }
    else if (vLocal.x > fWidth)
    {
        iSideX = 1;
        vClamp.x = fWidth;
        bInside = false;
    }
    else
    {
        iSideX = 0;
    }
    if (-fHeight > vLocal.y)
    {
        iSideY = -1;
        vClamp.y = -fHeight;
        bInside = false;
    }
    else if (vLocal.y > fHeight)
    {
        iSideY = 1;
        vClamp.y = fHeight;
        bInside = false;
    }
    else
    {
        iSideY = 0;
    }
    if (-fDepth > vLocal.z)
    {
        iSideZ = -1;
        vClamp.z = -fDepth;
        bInside = false;
    }
    else if (vLocal.z > fDepth)
    {
        iSideZ = 1;
        vClamp.z = fDepth;
        bInside = false;
    }
    else
    {
        iSideZ = 0;
    }
    if (!bInside)
    {
        float fDZ = 0.0f;
        float fDY = 0.0f;
        float fDX = 0.0f;
        if (iSideX)
        {
            fDX = (float)fabs((double)(vClamp.x - vLocal.x));
        }
        if (iSideY)
        {
            fDY = (float)fabs((double)(vClamp.y - vLocal.y));
        }
        if (iSideZ)
        {
            fDZ = (float)fabs((double)(vClamp.z - vLocal.z));
        }
        float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
        if (fSphereRadius * fSphereRadius > fDist2)
        {
            float fDist = (float)sqrt((double)fDist2);
            fPenetration = fSphereRadius - fDist;
            vNormal = vAxis[0] * ((float)iSideX * fDX) + vAxis[1] * ((float)iSideY * fDY) + vAxis[2] * ((float)iSideZ * fDZ);
            float fInv = 1.0f / fDist;
            vNormal = PblVector3(vNormal.x * fInv, vNormal.y * fInv, vNormal.z * fInv);
            return true;
        }
    }
    return false;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x00469e20
bool CollideSphereBox(const PblVector3& vCenter, float fRadius, const PblMatrix& matrix, float fWidth, float fDepth,
                float fHeight, PblVector3& vNormal, float& fPenetration)
{
    bool bCollided = CollisionUtil::CollideBoxSphere(matrix, fWidth, fDepth, fHeight, vCenter, fRadius, vNormal, fPenetration);
    vNormal = PblVector3(vNormal.x * -1.0f, vNormal.y * -1.0f, vNormal.z * -1.0f);
    return bCollided;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046a280
bool CollideCylinderSphere(const PblMatrix& matrix, float fRadius, float fHalfHeight, const PblVector3& vSphereCenter,
                float fSphereRadius, PblVector3& vNormal, float& fDepth)
{
    PblVector3 faceNormals[3] = { matrix.GetXAxis(), matrix.GetYAxis(), matrix.GetZAxis() };
    PblVector3 kDiff = vSphereCenter - matrix.GetTranslation();
    PblVector3 vLocal(DotProduct(kDiff, faceNormals[0]),
                      DotProduct(kDiff, faceNormals[1]),
                      DotProduct(kDiff, faceNormals[2]));
    float fDist2 = vLocal.x * vLocal.x + vLocal.z * vLocal.z;

    if (!(vLocal.y > fHalfHeight || -fHalfHeight > vLocal.y))
    {

        float fDist = sqrtf(fDist2);
        fDepth = (fRadius + fSphereRadius) - fDist;
        if (!(fDepth > 0.0f))
        {
            return false;
        }
        if (fDist > 0.0001f)
        {
            PblVector3 tmpDir = vLocal;
            tmpDir.y = 0.0f;
            tmpDir = tmpDir / fDist;
            vNormal = tmpDir.x * matrix.GetXAxis() +
                      tmpDir.y * matrix.GetYAxis() +
                      tmpDir.z * matrix.GetZAxis();
        }
        else
        {
            fDepth = Max(fSphereRadius, fRadius);
            vNormal = matrix.GetZAxis();
        }
        return true;
    }

    if (fRadius * fRadius > fDist2)
    {

        fDepth = fSphereRadius - (fabsf(vLocal.y) - fHalfHeight);
        if (!(fDepth > 0.0f))
        {
            return false;
        }
        if (0.0f > vLocal.y)
        {
            vNormal = -1.0f * faceNormals[1];
        }
        else
        {
            vNormal = faceNormals[1];
        }
        return true;
    }

    float xzDist = sqrtf(fDist2);
    PblVector3 tmpDir = vLocal;
    tmpDir.y = 0.0f;
    tmpDir = (fRadius / xzDist) * tmpDir;
    tmpDir.y = 0.0f > vLocal.y ? -fHalfHeight : fHalfHeight;
    tmpDir = vLocal - tmpDir;
    float fEdgeDist2 = DotProduct(tmpDir, tmpDir);
    if (!(fSphereRadius * fSphereRadius > fEdgeDist2))
    {
        return false;
    }
    float fEdgeDist = sqrtf(fEdgeDist2);
    fDepth = fSphereRadius - fEdgeDist;
    tmpDir = tmpDir / fEdgeDist;
    vNormal = tmpDir.x * matrix.GetXAxis() +
              tmpDir.y * matrix.GetYAxis() +
              tmpDir.z * matrix.GetZAxis();
    return true;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x004696b0
bool GetClosestPointInPolygon(PblVector3& vPos, int iNumCorners, const PblVector3* pCorners, PblVector3& vOutPos,
                float fMaxDist)
{
    PblVector3 Out0 = pCorners[0];
    PblVector3 Out1 = pCorners[1];
    PblVector3 Out2 = pCorners[2];
    PblVector3 e10 = Out1 - Out0;
    PblVector3 e21 = Out2 - Out1;
    PblVector3 normal = CrossProduct(e10, e21);
    float mag2 = SquaredLength(normal);
    if (mag2 < 0.0001f)
    {
        return false;
    }
    PblVector3 p10 = vPos - Out0;
    float dist = DotProduct(p10, normal);
    if (dist < 0.0f)
    {
        return false;
    }
    float maxDist2 = fMaxDist * fMaxDist;
    if (dist * dist > mag2 * maxDist2)
    {
        return false;
    }
    PblVector3 pointOut = vPos - normal * (dist / mag2);
    float side01 = DotProduct(e10, pointOut - Out0);
    float side10 = -DotProduct(e10, pointOut - Out1);
    if (side01 >= 0.0f && side10 >= 0.0f &&
        DotProduct(CrossProduct(normal, e10), pointOut - Out0) < 0.0f)
    {
        PblVector3 pointOutNew = Out0 + e10 * (side01 / SquaredLength(e10));
        PblVector3 vDist = vPos - pointOutNew;
        if (SquaredLength(vDist) <= maxDist2)
        {
            vOutPos = pointOutNew;
            return true;
        }
        return false;
    }
    for (int i = 1; i < iNumCorners; ++i)
    {
        int i2 = i + 1;
        if (i2 == iNumCorners)
        {
            i2 = 0;
        }
        Out2 = pCorners[i2];
        PblVector3 e21New = Out2 - Out1;
        float side12 = DotProduct(e21New, pointOut - Out1);
        float side21 = -DotProduct(e21New, pointOut - Out2);
        if (side12 >= 0.0f && side21 >= 0.0f &&
            DotProduct(CrossProduct(normal, e21New), pointOut - Out1) < 0.0f)
        {
            PblVector3 pointOutNew = Out1 + e21New * (side12 / SquaredLength(e21New));
            PblVector3 vDist = vPos - pointOutNew;
            if (SquaredLength(vDist) > maxDist2)
            {
                return false;
            }
            vOutPos = pointOutNew;
            return true;
        }
        if (side10 <= 0.0f && side12 <= 0.0f)
        {
            PblVector3 vDist = vPos - Out1;
            if (SquaredLength(vDist) > maxDist2)
            {
                return false;
            }
            vOutPos = Out1;
            return true;
        }
        Out1 = pCorners[i2];
        side10 = side21;
    }
    if (side10 > 0.0f || side01 > 0.0f)
    {
        vOutPos = pointOut;
        return true;
    }
    if (SquaredLength(p10) > maxDist2)
    {
        return false;
    }
    vOutPos = Out0;
    return true;
}
}

// FUNCTION: SWBF2 0x00468810
bool CollisionUtil::CollideBoxes(const PblVector3& vVelocityA, const PblMatrix& mA, float fWidthA, float fHeightA,
                                 float fDepthA, const PblVector3& vVelocityB, const PblMatrix& mB, float fWidthB,
                                 float fHeightB, float fDepthB, PblVector3& vNormal, float& fDepth)
{
    fDepth = fWidthA + fHeightA + fDepthA + fWidthB + fHeightB + fDepthB;

    PblMatrix mRel = mB * mA.GetInverseFast();
    PblVector3 relBoxMove2 = Normalize(vVelocityB - vVelocityA);
    PblVector3 xAxis21 = mRel.GetXAxis();
    PblVector3 yAxis21 = mRel.GetYAxis();
    PblVector3 zAxis21 = mRel.GetZAxis();
    PblVector3 pos21 = mRel.GetTranslation();
    PblVector3 boxSize1(fWidthA, fHeightA, fDepthA);
    PblVector3 boxSize2(fWidthB, fHeightB, fDepthB);
    int i;

    for (i = 0; i < 3; ++i)
    {
        float fT = pos21[i];
        float fSep = fabsf(xAxis21[i]) * fWidthB + fabsf(yAxis21[i]) * fHeightB +
                     fabsf(zAxis21[i]) * fDepthB + boxSize1[i] - fabsf(fT);
        if (0.0f > fSep)
        {
            return false;
        }
        if (fDepth > fSep)
        {
            fDepth = fSep;
            switch (i)
            {
            case 0: vNormal = mA.GetXAxis(); break;
            case 1: vNormal = mA.GetYAxis(); break;
            case 2: vNormal = mA.GetZAxis(); break;
            }
            if (0.0f > fT)
            {
                vNormal = -vNormal;
            }
        }
    }

    PblVector3 v;
    for (i = 0; i < 3; ++i)
    {
        switch (i)
        {
        case 0: v = xAxis21; break;
        case 1: v = yAxis21; break;
        case 2: v = zAxis21; break;
        }
        float fProj = DotProduct(pos21, v);
        float fSep = fabsf(v.x) * fWidthA + fabsf(v.y) * fHeightA + fabsf(v.z) * fDepthA + boxSize2[i] - fabsf(fProj);
        if (0.0f > fSep)
        {
            return false;
        }
        if (fDepth > fSep)
        {
            fDepth = fSep;
            switch (i)
            {
            case 0: vNormal = mB.GetXAxis(); break;
            case 1: vNormal = mB.GetYAxis(); break;
            case 2: vNormal = mB.GetZAxis(); break;
            }
            if (0.0f > fProj)
            {
                vNormal = -vNormal;
            }
        }
    }


    if (0.0f > (fabsf(xAxis21.y) * fDepthA + fabsf(xAxis21.z) * fHeightA) +
                   (fabsf(yAxis21.x) * fDepthB + fabsf(zAxis21.x) * fHeightB) - fabsf(xAxis21.y * pos21.z - xAxis21.z * pos21.y))
    {
        return false;
    }

    if (0.0f > (fabsf(yAxis21.y) * fDepthA + fabsf(yAxis21.z) * fHeightA) +
                   (fabsf(zAxis21.x) * fWidthB + fabsf(xAxis21.x) * fDepthB) - fabsf(yAxis21.y * pos21.z - yAxis21.z * pos21.y))
    {
        return false;
    }

    if (0.0f > (fabsf(zAxis21.y) * fDepthA + fabsf(zAxis21.z) * fHeightA) +
                   (fabsf(yAxis21.x) * fWidthB + fabsf(xAxis21.x) * fHeightB) - fabsf(zAxis21.y * pos21.z - zAxis21.z * pos21.y))
    {
        return false;
    }

    if (0.0f > (fabsf(xAxis21.z) * fWidthA + fabsf(xAxis21.x) * fDepthA) +
                   (fabsf(zAxis21.y) * fHeightB + fabsf(yAxis21.y) * fDepthB) - fabsf(xAxis21.z * pos21.x - xAxis21.x * pos21.z))
    {
        return false;
    }

    if (0.0f > (fabsf(yAxis21.x) * fDepthA + fabsf(yAxis21.z) * fWidthA) +
                   (fabsf(xAxis21.y) * fDepthB + fabsf(zAxis21.y) * fWidthB) - fabsf(yAxis21.z * pos21.x - yAxis21.x * pos21.z))
    {
        return false;
    }

    if (0.0f > (fabsf(zAxis21.x) * fDepthA + fabsf(zAxis21.z) * fWidthA) +
                   (fabsf(xAxis21.y) * fHeightB + fabsf(yAxis21.y) * fWidthB) - fabsf(zAxis21.z * pos21.x - zAxis21.x * pos21.z))
    {
        return false;
    }

    if (0.0f > (fabsf(xAxis21.y) * fWidthA + fabsf(xAxis21.x) * fHeightA) +
                   (fabsf(zAxis21.z) * fHeightB + fabsf(yAxis21.z) * fDepthB) - fabsf(xAxis21.x * pos21.y - xAxis21.y * pos21.x))
    {
        return false;
    }

    if (0.0f > (fabsf(yAxis21.x) * fHeightA + fabsf(yAxis21.y) * fWidthA) +
                   (fabsf(xAxis21.z) * fDepthB + fabsf(zAxis21.z) * fWidthB) - fabsf(yAxis21.x * pos21.y - yAxis21.y * pos21.x))
    {
        return false;
    }

    if (0.0f > (fabsf(zAxis21.x) * fHeightA + fabsf(zAxis21.y) * fWidthA) +
                   (fabsf(xAxis21.z) * fHeightB + fabsf(yAxis21.z) * fWidthB) - fabsf(zAxis21.x * pos21.y - zAxis21.y * pos21.x))
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x00468740
bool CollisionUtil::CollideCylinderBox(const PblVector3& vVelocityA, const PblMatrix& mA, float fRadius, float fHeight,
                                       const PblVector3& vVelocityB, const PblMatrix& mB, float fWidth, float fDepthB,
                                       float fHeightB, PblVector3& vNormal, float& fDepth)
{
    return CollideBoxes(vVelocityA, mA, fRadius, fHeight, fRadius, vVelocityB, mB, fWidth, fHeightB, fDepthB, vNormal,
                        fDepth);
}

// FUNCTION: SWBF2 0x00468790
bool CollisionUtil::CollideBoxCylinder(const PblVector3& vVelocityA, const PblMatrix& mA, float fWidth, float fDepthA,
                                       float fHeightA, const PblVector3& vVelocityB, const PblMatrix& mB, float fRadius,
                                       float fHeight, PblVector3& vNormal, float& fDepth)
{
    return CollideBoxes(vVelocityA, mA, fWidth, fHeightA, fDepthA, vVelocityB, mB, fRadius, fHeight, fRadius, vNormal,
                        fDepth);
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x004687e0
bool CollideCylinders(const PblMatrix& m1, float fRadius1, float fHalfHeight1, const PblMatrix& m2, float fRadius2,
                float fHalfHeight2, PblVector3& vNormal, float& fDepth)
{
    return CollisionUtil::CollideCapsules(m1, fRadius1, fHalfHeight1, m2, fRadius2, fHalfHeight2, vNormal, fDepth);
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x00469200
bool CollideCapsules(const PblMatrix& m1, float fRadius1, float fHalfHeight1, const PblMatrix& m2, float fRadius2,
                float fHalfHeight2, PblVector3& vNormal, float& fDepth)
{
    PblVector3 vStart1, vEnd1, vStart2, vEnd2;
    vStart1 = m1.GetTranslation() - fHalfHeight1 * m1.GetYAxis();
    vEnd1 = m1.GetTranslation() + fHalfHeight1 * m1.GetYAxis();
    vStart2 = m2.GetTranslation() - fHalfHeight2 * m2.GetYAxis();
    vEnd2 = m2.GetTranslation() + fHalfHeight2 * m2.GetYAxis();
    PblVector3 vPoint1, vPoint2, vDelta;
    CollisionUtil::GetClosestPoints_SegmentSegment(vStart1, vEnd1, vStart2, vEnd2, vPoint1, vPoint2);

    vDelta = vPoint2 - vPoint1;
    float fDist2 = DotProduct(vDelta, vDelta);
    if (0.0001f > fDist2)
    {
        vNormal.Set(1.0f, 0.0f, 0.0f);
        if (0.0001f > 1.0f - DotProduct(vNormal, m1.GetYAxis()))
        {
            vNormal.Set(0.0f, 0.0f, 1.0f);
        }
        fDepth = Max(fRadius1, fRadius2);
        return true;
    }
    float fDist = sqrtf(fDist2);
    fDepth = fRadius1 + fRadius2 - fDist;
    if (fDepth > 0.0001f)
    {
        vNormal = vDelta / fDist;
        return true;
    }
    return false;
}
}

// FUNCTION: SWBF2 0x004694d0
bool CollisionUtil::CollideAxisAlignedCylinders(PblVector3& vDelta, float fRadiusA, float fRadiusB, float fHeightA,
                                                float fHeightB, float& fDepth)
{
    float fAbsY = fabsf(vDelta.y);
    float fDistXZ = PblMath::Sqrt(vDelta.x * vDelta.x + vDelta.z * vDelta.z);
    float fRadii = fRadiusA + fRadiusB;
    if (fDistXZ >= fRadii)
    {
        return false;
    }
    float fHalfHeight = (fHeightA + fHeightB) * 0.5f;
    if (fAbsY >= fHalfHeight)
    {
        return false;
    }

    float fHorizontalOverlap = fRadii - fDistXZ;
    float fVerticalOverlap = fHalfHeight - fAbsY;
    if (fHorizontalOverlap < fVerticalOverlap)
    {
        vDelta.y = 0.0f;
        fDepth = fDistXZ;
        if (fDistXZ != 0.0f)
        {
            float fInv = 1.0f / fDistXZ;
            vDelta.x = vDelta.x * fInv;
            vDelta.y = fInv * vDelta.y;
            vDelta.z = vDelta.z * fInv;
        }
        else
        {
            vDelta.z = -1.0f;
        }
        fDepth = fRadii - fDepth;
    }
    else
    {
        vDelta.x = 0.0f;
        vDelta.z = 0.0f;
        fDepth = fAbsY;
        if (fAbsY != 0.0f)
        {
            float fInv = 1.0f / fAbsY;
            vDelta.x = vDelta.x * fInv;
            vDelta.y = fInv * vDelta.y;
            vDelta.z = vDelta.z * fInv;
        }
        else
        {
            vDelta.y = 1.0f;
        }
        fDepth = fHalfHeight - fDepth;
    }
    return true;
}

// FUNCTION: SWBF2 0x0046a210
bool CollisionUtil::CollideSphereCylinder(const PblVector3& vCenter, float fSphereRadius, const PblMatrix& matrix,
                                          float fRadius, float fHalfHeight, PblVector3& vNormal, float& fDepth)
{
    bool bHit = CollisionUtil::CollideCylinderSphere(matrix, fRadius, fHalfHeight, vCenter, fSphereRadius, vNormal, fDepth);
    vNormal = PblVector3(vNormal.x * -1.0f, vNormal.y * -1.0f, vNormal.z * -1.0f);
    return bHit;
}

// FUNCTION: SWBF2 0x0046a850
float CollisionUtil::RayHitSphere(const PblVector3& vStart, const PblVector3& vDir, float fLength,
                                  const PblVector3& vCenter, float fRadius, PblVector3* pNormal)
{
    if (fRadius != 0.0f)
    {
        PblVector3 vToCenter = vCenter - vStart;
        float fProj = DotProduct(vDir, vToCenter);
        if (-fRadius > fProj)
        {
            return 1.0f;
        }
        if (fProj > fLength + fRadius)
        {
            return 1.0f;
        }

        PblVector3 vNear = vStart + vDir * fProj;
        PblVector3 vNearToCenter = vNear - vCenter;
        float fDist2 = vNearToCenter.x * vNearToCenter.x + vNearToCenter.y * vNearToCenter.y +
                       vNearToCenter.z * vNearToCenter.z;
        float fRadius2 = fRadius * fRadius;
        if (fDist2 > fRadius2)
        {
            return 1.0f;
        }

        float fHalfChord2 = fRadius2 - fDist2;
        if (fHalfChord2 > fProj * fProj)
        {
            if (pNormal)
            {
                float fLen2 = vToCenter.x * vToCenter.x + vToCenter.y * vToCenter.y + vToCenter.z * vToCenter.z;
                if (fLen2 > 0.0001f)
                {
                    float fLen = (float)sqrt((double)fLen2);
                    if (fLen > 0.0001f)
                    {
                        float fInv = 1.0f / fLen;
                        *pNormal = PblVector3(fInv * vToCenter.x, fInv * vToCenter.y, fInv * vToCenter.z);
                        return 0.0f;
                    }
                }
                *pNormal = vDir;
            }
            return 0.0f;
        }

        if (0.0f > fProj)
        {
            return 1.0f;
        }
        float fHalfChord = (float)sqrt((double)fHalfChord2);
        float fHit = fProj - fHalfChord;
        if (fHit > fLength)
        {
            return 1.0f;
        }
        if (pNormal)
        {
            *pNormal = (vNear - vDir * fHalfChord - vCenter) * (1.0f / fRadius);
        }
        return fHit / fLength;
    }
    return 1.0f;
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046ab20
float RayHitBubble(const PblVector3& vStart, const PblVector3& vDir, float fLength, const PblMatrix& matrix,
                 const PblVector3& vSize, PblVector3* pNormal)
{
    PblVector3 vSizeInv(1.0f / vSize.x, 1.0f / vSize.y, 1.0f / vSize.z);
    PblVector3 vStartPosRel = vStart - matrix.GetTranslation();
    PblVector3 vStartPosLocal(DotProduct(matrix.GetXAxis(), vStartPosRel) * vSizeInv.x,
                             DotProduct(vStartPosRel, matrix.GetYAxis()) * vSizeInv.y,
                             DotProduct(matrix.GetZAxis(), vStartPosRel) * vSizeInv.z);
    float fDistSqr = SquaredLength(vStartPosLocal);
    if (fDistSqr < 1.0f)
    {
        return 1.0f;
    }

    PblVector3 vRayLocal(DotProduct(matrix.GetXAxis(), vDir) * (vSizeInv.x * fLength),
                        DotProduct(matrix.GetYAxis(), vDir) * (vSizeInv.y * fLength),
                        DotProduct(matrix.GetZAxis(), vDir) * (vSizeInv.z * fLength));
    float fRayDotR = DotProduct(vRayLocal, vStartPosLocal);
    if (fRayDotR >= 0.0f)
    {
        return 1.0f;
    }

    float fQ = fRayDotR * fRayDotR - (fDistSqr - 1.0f) * SquaredLength(vRayLocal);
    if (fQ <= 0.0f)
    {
        return 1.0f;
    }
    float fRaySqr = SquaredLength(vRayLocal);
    float fEnd = fRaySqr + fRayDotR;
    if (fEnd < 0.0f && fEnd * fEnd > fQ)
    {
        return 1.0f;
    }

    float fT = (-fRayDotR - sqrtf(fQ)) / fRaySqr;
    if (pNormal)
    {
        PblVector3 vNormal = (vStartPosLocal + vRayLocal * fT) * vSizeInv;
        *pNormal = Normalize(matrix.ApplyRotation(vNormal));
    }
    return fT;
}
}


namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046b130
float RayHitBox(const PblVector3& vStart, const PblVector3& vDir, float fLength, const PblMatrix& matrix,
                 float fWidth, float fDepth, float fHeight, PblVector3* pNormal)
{
    PblVector3 kDiff = vStart - matrix.GetTranslation();
    PblVector3 vLocalStart(DotProduct(kDiff, matrix.GetXAxis()),
                          DotProduct(kDiff, matrix.GetYAxis()),
                          DotProduct(kDiff, matrix.GetZAxis()));
    PblVector3 vLocalDir(DotProduct(vDir, matrix.GetXAxis()),
                        DotProduct(vDir, matrix.GetYAxis()),
                        DotProduct(vDir, matrix.GetZAxis()));

    float fHit = 0.0f;
    int face = FindAACenteredBoxRayIntersection(vLocalStart, vLocalDir, fLength, fWidth, fHeight, fDepth, fHit);
    if (face == 0)
    {
        return 1.0f;
    }
    if (pNormal)
    {
        switch (face)
        {
        case 1: *pNormal = matrix.GetXAxis(); break;
        case 2: *pNormal = matrix.GetYAxis(); break;
        case 3: *pNormal = matrix.GetZAxis(); break;
        case -1:
            *pNormal = -1.0f * matrix.GetXAxis();
            break;
        case -2:
            *pNormal = -1.0f * matrix.GetYAxis();
            break;
        case -3:
            *pNormal = -1.0f * matrix.GetZAxis();
            break;
        default:
            pNormal->x = 0.0f;
            pNormal->y = 1.0f;
            pNormal->z = 0.0f;
            break;
        }
    }
    return fHit;
}
}

// FUNCTION: SWBF2 0x0046b3e0
float RayHitAACylinder(const PblVector3& vStart, const PblVector3& vDir, float fLength, const PblVector3& vCenter,
                 float fRadius, float fHalfHeight, PblVector3* pNormal)
{
    PblVector3 vOffset(vStart.x - vCenter.x, vStart.y - vCenter.y, vStart.z - vCenter.z);
    if (CollisionUtil::IsPointInsideAxisAlignedCylinder(vOffset, fRadius, fHalfHeight))
    {
        if (pNormal)
        {
            if (vStart.y > vCenter.y)
            {
                pNormal->Set(0.0f, 1.0f, 0.0f);
            }
            else
            {
                pNormal->Set(0.0f, -1.0f, 0.0f);
            }
        }
        return 1.0f;
    }

    float fClipped = fLength;
    float fEndY = vDir.y * fLength + vStart.y;
    float fBottom = vCenter.y - fHalfHeight;
    float fTop = vCenter.y + fHalfHeight;
    if (fBottom > fEndY)
    {
        fClipped = (vStart.y - fBottom) / (vStart.y - fEndY) * fLength;
    }
    else if (fEndY > fTop)
    {
        fClipped = (vStart.y - fTop) / (vStart.y - fEndY) * fLength;
    }

    PblVector3 vFlat(vDir.x * fClipped, 0.0f, vDir.z * fClipped);
    float fFlatLen = Length(vFlat);
    if (fFlatLen < 0.0001f)
    {
        return 1.0f;
    }
    PblVector3 vFlatDir = vFlat * (1.0f / fFlatLen);

    float fProj = DotProduct(vCenter - vStart, vFlatDir);
    if (fProj < 0.0f || fProj > fFlatLen + fRadius)
    {
        return 1.0f;
    }
    float fDx = vStart.x + vFlatDir.x * fProj - vCenter.x;
    float fDz = vStart.z + vFlatDir.z * fProj - vCenter.z;
    float fDist2 = fDx * fDx + fDz * fDz;
    if (fDist2 > fRadius * fRadius)
    {
        return 1.0f;
    }
    float fEnter = fProj - sqrtf(fRadius * fRadius - fDist2);
    if (fEnter > fFlatLen)
    {
        return 1.0f;
    }

    float fHit = fEnter / fFlatLen * fClipped;
    if (pNormal)
    {
        *pNormal = vStart + vDir * fHit - vCenter;
        pNormal->y = 0.0f;
        PblVector3 vUnit;
        D3DXVec3Normalize((D3DXVECTOR3*)&vUnit, (const D3DXVECTOR3*)pNormal);
        *pNormal = vUnit;
    }
    return fHit / fLength;
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046b710
float RayHitAxisAlignedCylinder(const PblVector3& vStart, const PblVector3& vDir, float fLength, const PblVector3& vCenter,
                 float fRadius, float fHalfHeight, PblVector3* pNormal)
{
    PblVector3 vEnd = vStart + vDir * fLength;
    float fTop = vCenter.y + fHalfHeight;
    if (vStart.y > fTop)
    {
        if (fTop > vEnd.y)
        {
            float t = (vStart.y - fTop) / (vStart.y - vEnd.y);
            PblVector3 vIntersect = vStart + vDir * (t * fLength);
            if (fRadius * fRadius > SquaredLengthXZ(vIntersect - vCenter))
            {
                if (pNormal)
                {
                    pNormal->Set(0.0f, 1.0f, 0.0f);
                }
                return t;
            }

            t += 0.0001f;
            float fStep = t * fLength;
            PblVector3 vS = vStart + vDir * fStep;
            float fRest = 1.0f - t;
            float fHit = RayHitAACylinder(vS, vDir, fRest * fLength, vCenter, fRadius, fHalfHeight, pNormal);
            if (fHit < 1.0f)
            {
                return fRest * fHit + t;
            }
        }
        return 1.0f;
    }

    float fBottom = vCenter.y - fHalfHeight;
    if (fBottom > vStart.y)
    {
        if (vEnd.y > fBottom)
        {
            float t = (vStart.y - fBottom) / (vStart.y - vEnd.y);
            PblVector3 vIntersect = vStart + vDir * (t * fLength);
            if (fRadius * fRadius > SquaredLengthXZ(vIntersect - vCenter))
            {
                if (pNormal)
                {
                    pNormal->Set(0.0f, -1.0f, 0.0f);
                }
                return t;
            }

            t += 0.0001f;
            float fStep = t * fLength;
            PblVector3 vS = vStart + vDir * fStep;
            float fRest = 1.0f - t;
            float fHit = RayHitAACylinder(vS, vDir, fRest * fLength, vCenter, fRadius, fHalfHeight, pNormal);
            if (fHit < 1.0f)
            {
                return fRest * fHit + t;
            }
        }
        return 1.0f;
    }
    return RayHitAACylinder(vStart, vDir, fLength, vCenter, fRadius, fHalfHeight, pNormal);
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046b9c0
float RayHitCylinder(const PblVector3& vStart, const PblVector3& vDir, float fLength, const PblMatrix& matrix,
                 float fRadius, float fHalfHeight, PblVector3* pNormal)
{
    PblVector3 kDiff = vStart - matrix.GetTranslation();
    PblVector3 vLocalStart(DotProduct(kDiff, matrix.GetXAxis()),
                          DotProduct(kDiff, matrix.GetYAxis()),
                          DotProduct(kDiff, matrix.GetZAxis()));
    PblVector3 vLocalDir(DotProduct(vDir, matrix.GetXAxis()),
                        DotProduct(vDir, matrix.GetYAxis()),
                        DotProduct(vDir, matrix.GetZAxis()));
    PblVector3 vCenter(0.0f, 0.0f, 0.0f);
    PblVector3 vLocalNormal;

    float fHit = CollisionUtil::RayHitAxisAlignedCylinder(vLocalStart, vLocalDir, fLength, vCenter, fRadius, fHalfHeight, &vLocalNormal);
    if (fHit < 1.0f && pNormal)
    {
        *pNormal = vLocalNormal.x * matrix.GetXAxis() +
                   vLocalNormal.y * matrix.GetYAxis() +
                   vLocalNormal.z * matrix.GetZAxis();
    }
    return fHit;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046bf30
void GetClosestPoint_SegmentPoint(const PblVector3& vSegStart, const PblVector3& vSegEnd, const PblVector3& vPoint,
                PblVector3& vClosest)
{
    PblVector3 vDelta = vSegEnd - vSegStart;
    PblVector3 vOffset = vPoint - vSegStart;
    float fProj = DotProduct(vOffset, vDelta);
    if (fProj <= 0.0f)
    {
        vClosest = vSegStart;
        return;
    }
    float fLen2 = DotProduct(vDelta, vDelta);
    if (fProj >= fLen2)
    {
        vClosest = vSegEnd;
        return;
    }
    float fT = fProj / fLen2;
    vClosest = vSegStart + fT * vDelta;
}
}

namespace CollisionUtil
{

// FUNCTION: SWBF2 0x0046bc00
void GetClosestPoints_SegmentSegment(const PblVector3& vSStart, const PblVector3& vSEnd, const PblVector3& vTStart,
                const PblVector3& vTEnd, PblVector3& vSPoint, PblVector3& vTPoint)
{
    const float SMALL = 0.0001f;
    PblVector3 u = vSEnd - vSStart;
    PblVector3 v = vTEnd - vTStart;
    PblVector3 w = vSStart - vTStart;
    float a = DotProduct(u, u);
    float b = DotProduct(v, u);
    float c = DotProduct(v, v);
    float d = DotProduct(w, u);
    float e = DotProduct(w, v);
    float D = c * a - b * b;
    float sN, sD = D, tN, tD = D;

    if (D < SMALL)
    {
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
        tD = c;
    }
    else
    {
        sN = e * b - d * c;
        tN = e * a - d * b;
        if (sN < 0.0f)
        {
            sN = 0.0f;
            tN = e;
            tD = c;
        }
        else if (sN > sD)
        {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0f)
    {
        tN = 0.0f;
        if (-d < 0.0f)
        {
            sN = 0.0f;
        }
        else if (-d > a)
        {
            sN = sD;
        }
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD)
    {
        tN = tD;
        if (b - d < 0.0f)
        {
            sN = 0.0f;
        }
        else if (b - d > a)
        {
            sN = sD;
        }
        else
        {
            sN = b - d;
            sD = a;
        }
    }

    float sc = fabsf(sN) < SMALL ? 0.0f : sN / sD;
    float tc = fabsf(tN) < SMALL ? 0.0f : tN / tD;
    vSPoint = vSStart + u * sc;
    vTPoint = vTStart + v * tc;
}
}
