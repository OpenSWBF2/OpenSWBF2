#include <d3dx9math.h>
#include "NetPredict.h"
#include "NetGame.h"
#include "PebbleFL/Common/PblIndex.h"

NetInterpolator netInterpolator;


// GLOBAL: SWBF2 0x01ead160
unsigned int interpId[64];
// GLOBAL: SWBF2 0x01ead260
int interpCat[64];
// GLOBAL: SWBF2 0x01ead360
int interpNext;
// GLOBAL: SWBF2 0x01ead368
int interpCount[64];

extern PblVector3 interpPos[64][2];
extern PblMatrix interpMat[64][2];

// FUNCTION: SWBF2 0x005daa90
bool NetInterpolator::GetIndex(int& iIndex, int iId, unsigned int uHandleId)
{
    if (iIndex < 0 || iId != interpCat[iIndex] || uHandleId != interpId[iIndex])
    {
        iIndex = interpNext;
        if (++interpNext >= sizeof(interpId) / sizeof(interpId[0]))
        {
            interpNext = 0;
        }
        interpCount[iIndex] = 0;
        interpCat[iIndex] = iId;
        interpId[iIndex] = uHandleId;
    }
    return true;
}

// FUNCTION: SWBF2 0x005daaf0
void NetInterpolator::Prev(int iIndex)
{
    --interpCount[iIndex];
}

// FUNCTION: SWBF2 0x005dab10
void NetInterpolator::SetNext(int iIndex, const PblVector3* pVec)
{
    if (iIndex >= 0)
    {
        if (pVec)
        {
            interpPos[iIndex][interpCount[iIndex] & 1] = *pVec;
            ++interpCount[iIndex];
        }
        else
        {
            interpCount[iIndex] = 0;
        }
    }
}

// FUNCTION: SWBF2 0x005dac60
void NetInterpolator::SetNext(int iIndex, const PblMatrix* pMat)
{
    if (iIndex >= 0)
    {
        if (pMat)
        {
            interpMat[iIndex][interpCount[iIndex] & 1] = *pMat;
            ++interpCount[iIndex];
        }
        else
        {
            interpCount[iIndex] = 0;
        }
    }
}

// FUNCTION: SWBF2 0x005dab70
void NetInterpolator::GetVec(int iIndex, PblVector3& vVec)
{
    if (iIndex < 0 || interpCount[iIndex] < 1)
    {
        return;
    }
    const int iNew = (interpCount[iIndex] - 1) & 1;
    const PblVector3& vNew = interpPos[iIndex][iNew];
    if (interpCount[iIndex] < 2)
    {
        vVec = vNew;
        return;
    }
    const float fRatio = NetGame::GetTurnRatio();
    const PblVector3& vOld = interpPos[iIndex][interpCount[iIndex] & 1];
    vVec = Interpolate(vOld, vNew, fRatio);
}

// FUNCTION: SWBF2 0x005dacd0
void NetInterpolator::GetMat(int iIndex, PblMatrix& mMat)
{
    if (iIndex < 0 || interpCount[iIndex] < 1)
    {
        return;
    }
    const int iNew = (interpCount[iIndex] - 1) & 1;
    const PblMatrix& mNew = interpMat[iIndex][iNew];
    if (interpCount[iIndex] < 2)
    {
        mMat = mNew;
        return;
    }

    PblQuaternion qNew(mNew);
    const PblVector3 vNew = mNew.GetTranslation();
    const float fRatio = NetGame::GetTurnRatio();
    const PblMatrix& mOld = interpMat[iIndex][interpCount[iIndex] & 1];
    PblQuaternion qOld(mOld);
    const PblVector3 vOld = mNew.GetTranslation();
    const PblVector3 vBlend = Interpolate(vOld, vNew, fRatio);
    mMat = PblMatrix(Interpolate(qOld, qNew, fRatio), vBlend);
}

extern int gDebugCount;
extern PblVector3 gVectorZero;
extern bool netIsLocalTurn;
extern bool netIsLastUpdateTurn;

void Smooth(SmoothInfo& info, float fParam1, float fParam2);

// GLOBAL: SWBF2 0x01fa88e8
static float renderTime;

// FUNCTION: SWBF2 0x005d9c10
void NetPredict::SetRenderTime(float fTime)
{
    renderTime = fTime;
}
extern SmoothInfo smoothInfo[64];

// FUNCTION: SWBF2 0x005da0a0
void SmoothInfo::SetRenderMatrix(const PblMatrix& mat)
{
    lastRenderMat[3] = lastRenderMat[2];
    lastRenderMat[2] = lastRenderMat[1];
    lastRenderMat[1] = lastRenderMat[0];
    lastRenderMat[0] = mat;
    lastRenderTime[3] = lastRenderTime[2];
    lastRenderTime[2] = lastRenderTime[1];
    lastRenderTime[1] = lastRenderTime[0];
    lastRenderTime[0] = renderTime;
    ++renderCount;
}

// FUNCTION: SWBF2 0x005da1b0
PblMatrix& SmoothInfo::GetRenderMatrix()
{
    int i = 0;
    for (; i < sizeof(lastRenderTime) / sizeof(lastRenderTime[0]); ++i)
    {
        if (renderTime == lastRenderTime[i])
        {
            break;
        }
    }
    if (i >= sizeof(lastRenderTime) / sizeof(lastRenderTime[0]))
    {
        i = 0;
    }
    else if (i >= renderCount)
    {
        i = renderCount - 1;
    }
    return lastRenderMat[i];
}

// FUNCTION: SWBF2 0x005da540
void Jump(SmoothInfo& smooth)
{
    smooth.SetRenderMatrix(smooth.lastPredictMat[0]);
}

// FUNCTION: SWBF2 0x005da730
void SmoothSoldier(SmoothInfo& smooth)
{
    SmoothLocal(smooth);
}

// FUNCTION: SWBF2 0x005da740
void SmoothWalker(SmoothInfo& smooth)
{
    Filter(smooth);
}

// FUNCTION: SWBF2 0x005da210
void Smooth(SmoothInfo& info, float fSmallJump, float fLargeJump)
{
    if (&info - smoothInfo == 8)
    {
        ++gDebugCount;
    }
    PblVector3 diff = info.lastPredictMat[0].GetTranslation() - info.lastRenderMat[0].GetTranslation();
    float fDistanceSquared = SquaredLength(diff);
    if (fDistanceSquared >= fLargeJump * fLargeJump)
    {
        Jump(info);
        return;
    }
    if (fDistanceSquared >= fSmallJump * fSmallJump)
    {
        Filter(info);
        return;
    }
    if (NetGame::GetJoystickIndex(info.mPlayerId) >= 0)
    {
        SmoothLocal(info);
        return;
    }
    if (info.renderCount <= 3)
    {
        if (info.renderCount > 0)
        {
            Filter(info);
        }
        else
        {
            Jump(info);
        }
        return;
    }
    switch (info.type)
    {
    case ST_SOLDIER: SmoothSoldier(info); break;
    case ST_WALKER: SmoothWalker(info); break;
    case ST_VEHICLE: SmoothVehicle(info); break;
    }
}

inline float Filter(float target, float current, float rate)
{
    if (rate < 0.0001f)
    {
        return current;
    }
    if (rate >= 1.0f)
    {
        return target;
    }
    return current + (target - current) * rate;
}

// FUNCTION: SWBF2 0x005da300
void Filter(SmoothInfo& smooth)
{
    if (smooth.renderCount <= 0)
    {
        Jump(smooth);
        return;
    }
    float dt = renderTime - smooth.lastRenderTime[0];
    const PblMatrix* mats = smooth.lastPredictMat;
    PblQuaternion q0(mats[0]);
    PblQuaternion q1(smooth.lastRenderMat[0]);
    PblQuaternion q = Interpolate(q0, q1, dt * 10.0f);
    float tx = Filter(mats[0].GetTranslation().x, smooth.lastRenderMat[0].GetTranslation().x, dt * 5.0f);
    float ty = Filter(mats[0].GetTranslation().y, smooth.lastRenderMat[0].GetTranslation().y, dt * 5.0f);
    float tz = Filter(mats[0].GetTranslation().z, smooth.lastRenderMat[0].GetTranslation().z, dt * 5.0f);
    PblMatrix mat(q, PblVector3(tx, ty, tz));
    if (smooth.mErrTime > 0.0f)
    {
        PblVector3 posErr = mat.GetTranslation() - mats[0].GetTranslation();
        PblVector3 frontErr = mat.GetZAxis() - mats[0].GetZAxis();
        smooth.mPosErr = posErr;
        smooth.mFrontErr = frontErr;
        smooth.mRotErr = PblQuaternion();
    }
    smooth.SetRenderMatrix(mat);
}

// FUNCTION: SWBF2 0x005da550
void SmoothVehicle(SmoothInfo& smooth)
{
    float nextTime = renderTime + 0.25f;
    float deltaTime = renderTime - smooth.lastRenderTime[0];
    PblVector3 nextPos = smooth.lastPredictMat[0].GetTranslation() + smooth.mVelocity * (nextTime - renderTime);
    float ratio = deltaTime / (nextTime - smooth.lastRenderTime[0]);
    float x = Filter(nextPos.x, smooth.lastRenderMat[0].GetTranslation().x, ratio);
    float y = Filter(nextPos.y, smooth.lastRenderMat[0].GetTranslation().y, ratio);
    float z = Filter(nextPos.z, smooth.lastRenderMat[0].GetTranslation().z, ratio);
    PblQuaternion q0(smooth.lastRenderMat[0]);
    PblQuaternion q1(smooth.lastPredictMat[0]);
    PblQuaternion q = Interpolate(q0, q1, deltaTime * 10.0f);
    PblMatrix mat(q, PblVector3(x, y, z));
    smooth.SetRenderMatrix(mat);
}

// FUNCTION: SWBF2 0x005da750
void SmoothLocal(SmoothInfo& smooth)
{
    if (&smooth - smoothInfo == 8)
    {
        ++gDebugCount;
    }
    if (!smooth.mPlayerId)
    {
        ++gDebugCount;
    }
    float dt = renderTime - smooth.lastRenderTime[0];
    float errTime = smooth.mErrTime - dt;
    if (errTime < 0.0f)
    {
        errTime = 0.0f;
    }
    if (errTime <= 0.0f)
    {
        smooth.mPosErr.SetAll(0.0001f);
        smooth.mFrontErr.SetAll(0.0001f);
        smooth.SetRenderMatrix(smooth.lastPredictMat[0]);
    }
    else
    {
        float ratio = dt / smooth.mErrTime;
        smooth.mPosErr = Interpolate(smooth.mPosErr, gVectorZero, ratio);
        PblQuaternion z;
        smooth.mRotErr = Interpolate(smooth.mRotErr, z, ratio);
        PblMatrix r(smooth.mRotErr, PblVector3(0.0f, 0.0f, 0.0f));
        PblMatrix m = smooth.lastPredictMat[0];
        m.SetTranslation(0.0f, 0.0f, 0.0f, 1.0f);
        m = m * r;
        m.SetTranslation(smooth.lastPredictMat[0].GetTranslation() + smooth.mPosErr);
        smooth.SetRenderMatrix(m);
        smooth.mErrTime -= dt;
        smooth.mFrontErr *= 1.0f - ratio;
    }
    smooth.mErrTime = errTime;
}

// SYNTHETIC: SWBF2 0x004055f0
// `dynamic initializer for 'smoothInfo''

// GLOBAL: SWBF2 0x01ee8390
// smoothInfo
SmoothInfo smoothInfo[64];

static SmoothInfo sDiscardSmoothInfo;

// FUNCTION: SWBF2 0x005d9c00
SmoothInfo& NetPredict::GetSmoothInfo(int iIndex)
{
    if (PblInBounds(smoothInfo, iIndex))
    {
        return smoothInfo[iIndex];
    }
    return sDiscardSmoothInfo;
}

// FUNCTION: SWBF2 0x005d9d20
void SmoothInfo::SetMat(const PblMatrix& mat, float fParam1, float fParam2)
{
    if (this - smoothInfo == 8)
    {
        ++gDebugCount;
    }

    if (netIsLocalTurn)
    {
        lastPredictMat[1] = lastPredictMat[0];
        lastPredictMat[0] = mat;
        ++predictCount;
        Smooth(*this, fParam1, fParam2);
        return;
    }
    if (!netIsLastUpdateTurn)
    {
        return;
    }

    if (this - smoothInfo == 8)
    {
        ++gDebugCount;
    }
    if (!mPlayerId)
    {
        ++gDebugCount;
    }

    bool bLocal = NetGame::GetJoystickIndex(mPlayerId) >= 0;
    if (!bLocal && type != ST_SOLDIER)
    {
        return;
    }
    if ((unsigned int)renderCount < 4)
    {
        return;
    }

    unsigned int i = 0;
    for (; i < 4; ++i)
    {
        if (renderTime == lastRenderTime[i])
        {
            break;
        }
    }
    if (i >= 4)
    {
        return;
    }

    const PblMatrix& rendered = lastRenderMat[i];
    PblVector3 vPosErr(rendered.m[3][0] - mat.m[3][0], rendered.m[3][1] - mat.m[3][1],
                       rendered.m[3][2] - mat.m[3][2]);
    PblVector3 vFrontErr(rendered.m[2][0] - mat.m[2][0], rendered.m[2][1] - mat.m[2][1],
                         rendered.m[2][2] - mat.m[2][2]);
    if (vPosErr.y * vPosErr.y + vPosErr.x * vPosErr.x + vPosErr.z * vPosErr.z >
            mPosErr.x * mPosErr.x + mPosErr.y * mPosErr.y + mPosErr.z * mPosErr.z ||
        vFrontErr.x * vFrontErr.x + vFrontErr.y * vFrontErr.y + vFrontErr.z * vFrontErr.z >
            mFrontErr.x * mFrontErr.x + mFrontErr.y * mFrontErr.y + mFrontErr.z * mFrontErr.z)
    {
        mErrTime = bLocal ? 0.25f : 0.5f;
        mPosErr = vPosErr;
        mFrontErr = vFrontErr;

        PblMatrix oldRot = rendered;
        oldRot.m[3][0] = 0.0f;
        oldRot.m[3][1] = 0.0f;
        oldRot.m[3][2] = 0.0f;
        oldRot.m[3][3] = 1.0f;
        PblMatrix newRot = mat;
        newRot.m[3][0] = 0.0f;
        newRot.m[3][1] = 0.0f;
        newRot.m[3][2] = 0.0f;
        newRot.m[3][3] = 1.0f;
        PblMatrix newInverse = newRot.GetInverseFast();
        PblMatrix relMatrix;
        relMatrix = newInverse * oldRot;
        mRotErr = PblQuaternion(relMatrix);
    }
}

// FUNCTION: SWBF2 0x005d9c20
void SmoothInfo::ResetMat(const PblMatrix& mat)
{
    PblQuaternion z;
    lastPredictMat[0] = mat;
    predictCount = 1;
    lastRenderMat[0] = mat;
    renderCount = 1;
    lastRenderTime[0] = renderTime;
    mErrTime = 0.0f;
    mPosErr.x = mPosErr.y = mPosErr.z = 0.0001f;
    mFrontErr.x = mFrontErr.y = mFrontErr.z = 0.0001f;
    mRotErr = z;
}

// SYNTHETIC: SWBF2 0x00405680
// `dynamic initializer for 'interpPos''

// GLOBAL: SWBF2 0x01ee7d90
// interpPos
PblVector3 interpPos[64][2];

// SYNTHETIC: SWBF2 0x004056a0
// `dynamic initializer for 'interpMat''

// GLOBAL: SWBF2 0x01eeff90
// interpMat
PblMatrix interpMat[64][2];
