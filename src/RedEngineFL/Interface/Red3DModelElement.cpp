#include <d3dx9math.h>
#include "Red3DModelElement.h"
#include "Red3DModelAnimator.h"

#include <math.h>

#include "PebbleFL/Common/PblMath.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "RedEngineFL/Graphics/RedPose.h"


void __cdecl PblTraceP(const char* Format, ...);

PblPtrSet<256> Red3DModelElement::sAttachHosts;

namespace
{

inline PblQuaternion AxisAngle(const PblVector3& vAxis, float fAngle)
{
    const float fLen2 = vAxis.x * vAxis.x + vAxis.y * vAxis.y + vAxis.z * vAxis.z;
    if ((float)fabs(fLen2 - 1.0) > 0.0001f)
    {
        PblTraceP("axis ( %f, %f, %f ) is not of unit length!\n", vAxis.x, vAxis.y, vAxis.z);
    }

    return PblQuaternion(vAxis, fAngle);
}
}

// FUNCTION: SWBF2 0x006da8d0
Red3DModelElement::Red3DModelElement()
{

    PblMatrix mIdentity;
    mIdentity.Reset();
    m_DrawMatrix = mIdentity;
    m_PosedMatrix = mIdentity;

    m_bAmAttached = false;
    m_bHavePoseMatrix = false;
    m_bLightingEnabled = false;
    m_uiModelHash = 0;
    m_scale = 1.0f;
    m_uiHardpointHash = 0;
    m_AttachedObj = 0;
    m_Model = 0;
    m_OmegaY = 0.0f;
    m_Yaw = 0.0f;
    m_translation = PblVector3(0.0f, 0.0f, 0.0f);
    m_rotation = AxisAngle(PblVector3(0.0f, 0.0f, 1.0f), 0.0f);
    m_bApplyRotationScale = true;
    mAnimator = 0;
}

// FUNCTION: SWBF2 0x006dab70
void Red3DModelElement::EnsureAnimatorExists()
{
    if (!mAnimator)
    {
        mAnimator = new Red3DModelAnimator;
    }
}

// FUNCTION: SWBF2 0x006dabe0
void Red3DModelElement::RenderUsingContext(const PblMatrix& mContext,
                                           const RedColor& color)
{
    if (!FindModel())
    {
        return;
    }

    unsigned int uiFlags = GetRenderFlags();

    if (m_bAmAttached)
    {
        if (m_bHavePoseMatrix)
        {
            PblMatrix mWorld = m_PosedMatrix * m_DrawMatrix;
            m_Model->Render(&mWorld, 0, color, uiFlags, 0);
        }
        return;
    }

    RedPose pose;
    RedPose* pPose = 0;
    if (mAnimator)
    {
        pPose = mAnimator->UpdatePose();
    }

    PblMatrix mWorld = mContext;
    LocalToWorld(mWorld);

    if (m_bApplyRotationScale)
    {
        D3DXQUATERNION qRotation;
        PblMatrix mLocal;
        D3DXMatrixRotationQuaternion(reinterpret_cast<D3DXMATRIX*>(&mLocal), D3DXQuaternionNormalize(&qRotation, reinterpret_cast<const D3DXQUATERNION*>(&m_rotation)));
        mLocal.m[3][0] = m_translation.x;
        mLocal.m[3][1] = m_translation.y;
        mLocal.m[3][2] = m_translation.z;
        mLocal.m[3][3] = 1.0f;
        if (m_OmegaY != 0.0f)
        {
            mLocal.RotateY(m_Yaw * 0.017453292f);
            m_Yaw += m_OmegaY;
        }
        mLocal.Scale(m_scale);
        mWorld = mLocal * mWorld;
    }

    if (!pPose)
    {
        pPose = &pose;
        pose.Clear();
        pose.BindToSkeleton(0);
    }

    m_Model->SetupPose(*pPose);
    m_Model->Render(&mWorld, pPose, color, uiFlags, 0);

    if (m_AttachedObj && m_uiHardpointHash)
    {
        const PblMatrix* pHardpoint = pPose->Find(m_uiHardpointHash);
        if (pHardpoint)
        {
            m_AttachedObj->SetPosedMatrix(*pHardpoint);
            m_AttachedObj->SetDrawMatrix(mWorld);
        }
    }
}

// FUNCTION: SWBF2 0x006daee0
void Red3DModelElement::SetDrawMatrix(const PblMatrix& mDraw)
{
    m_DrawMatrix = mDraw;
}

// FUNCTION: SWBF2 0x006daf20
void Red3DModelElement::SetPosedMatrix(const PblMatrix& mPosed)
{
    m_PosedMatrix = mPosed;
    m_bHavePoseMatrix = true;
}

// FUNCTION: SWBF2 0x006da6d0
Red3DModelElement::Red3DModelElement(const char* pcName)
    : Red3DModelElementLite(pcName)
{

    PblMatrix mIdentity;
    mIdentity.Reset();
    m_DrawMatrix = mIdentity;
    m_PosedMatrix = mIdentity;

    m_bAmAttached = false;
    m_bHavePoseMatrix = false;
    m_bLightingEnabled = false;
    m_uiModelHash = 0;
    m_scale = 1.0f;
    m_uiHardpointHash = 0;
    m_AttachedObj = 0;
    m_Model = 0;
    m_OmegaY = 0.0f;
    m_Yaw = 0.0f;
    m_translation = PblVector3(0.0f, 0.0f, 0.0f);
    m_rotation = AxisAngle(PblVector3(0.0f, 0.0f, 1.0f), 0.0f);
    m_bApplyRotationScale = true;
    mAnimator = 0;
}

// FUNCTION: SWBF2 0x006daad0
Red3DModelElement::~Red3DModelElement()
{
    sAttachHosts.Remove(this);
    if (m_bAmAttached)
    {
        for (int i = 0; i < 256; ++i)
        {
            Red3DModelElement* pHost = (Red3DModelElement*)sAttachHosts.mSlots[i];
            if (pHost && pHost->m_AttachedObj == this)
            {
                pHost->m_AttachedObj = 0;
                pHost->m_uiHardpointHash = 0;
            }
        }
    }
    delete mAnimator;
}


