#include <d3dx9math.h>
#include "Red3DModelElementLite.h"

#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "RedEngineFL/Graphics/RedPose.h"

extern float gRedInterfaceTranslateZ;


// FUNCTION: SWBF2 0x006d6c90
Red3DModelElementLite::Red3DModelElementLite()
{
    Init();
}

// FUNCTION: SWBF2 0x006d6d60
void Red3DModelElementLite::Init()
{
    m_uiModelHash = 0;
    m_Model = 0;
    m_fDepth = 0.01f;
    m_searchForModel = false;
    m_bLightingEnabled = true;
    m_boneHash = 0;
}

// FUNCTION: SWBF2 0x006d6fc0
void Red3DModelElementLite::SetModel(unsigned int uiModelHash)
{
    m_uiModelHash = uiModelHash;
    m_Model = RedModel::_HashTable.Find(uiModelHash);
    m_searchForModel = m_uiModelHash && !m_Model;
}

// FUNCTION: SWBF2 0x006d7010
void Red3DModelElementLite::SetModel(const char* const pcModelName)
{
    SetModel(PblHash(pcModelName));
}

// GLOBAL: SWBF2 0x009982cc
bool Red3DModelElementLite::sPerspectiveHackEnabled;

// FUNCTION: SWBF2 0x006d7060
void Red3DModelElementLite::SetPerspectiveHack(bool bEnable)
{
    sPerspectiveHackEnabled = bEnable;
}

// FUNCTION: SWBF2 0x006d6d20
float Red3DModelElementLite::GetDepthMultiplier(bool bScreenScale)
{
    if (bScreenScale)
    {
        return gRedInterfaceTranslateZ;
    }
    return 1.0f;
}

// FUNCTION: SWBF2 0x006d6e40
void Red3DModelElementLite::RenderUsingContext(const PblMatrix& mContext,
                                               const RedColor& color)
{
    if (!FindModel())
    {
        return;
    }

    unsigned int uiFlags = GetRenderFlags();

    RedPose pose;
    pose.Clear();
    pose.BindToSkeleton(0);

    PblMatrix mWorld = mContext;
    PblMatrix mBone;
    if (m_boneHash && m_Model->GetBoneMatrix(m_boneHash, mBone))
    {
        mWorld *= mBone;
    }

    LocalToWorld(mWorld);
    m_Model->SetupPose(pose);
    m_Model->Render(&mWorld, &pose, color, uiFlags, 0);
}

// FUNCTION: SWBF2 0x006d6f90
void Red3DModelElementLite::SetBone(unsigned int uiBoneHash)
{
    m_boneHash = uiBoneHash;
}

// FUNCTION: SWBF2 0x006d6fa0
void Red3DModelElementLite::SetBone(char const* const pcBoneName)
{
    SetBone(PblTEMPHash(pcBoneName));
}

// FUNCTION: SWBF2 0x006d7030
void Red3DModelElementLite::SetModel(const RedModel* const pModel)
{
    m_Model = pModel;
    m_uiModelHash = pModel ? pModel->GetName() : 0;
    m_searchForModel = false;
}

// FUNCTION: SWBF2 0x006d6c60
Red3DModelElementLite::Red3DModelElementLite(const char* const pcName)
    : RedInterfaceElement(pcName)
{
    Init();
}

// FUNCTION: SWBF2 0x006d6ce0
bool Red3DModelElementLite::FindModel()
{
    if (m_searchForModel)
    {
        m_Model = RedModel::_HashTable.Find(m_uiModelHash);
        m_searchForModel = false;
    }
    return m_Model != 0;
}

// FUNCTION: SWBF2 0x006d6d40
unsigned int Red3DModelElementLite::GetRenderFlags() const
{
    unsigned int uFlags = 0x4000280;
    if (!m_bLightingEnabled)
    {
        uFlags = 0x4000290;
    }
    return uFlags;
}

// FUNCTION: SWBF2 0x006d6d90
void Red3DModelElementLite::LocalToWorld(PblMatrix& mMatrix) const
{
    mMatrix[1] = mMatrix[1] * -1.0f;
    mMatrix.m[1][3] = 0.0f;

    const float fScale = GetDepthMultiplier(sPerspectiveHackEnabled) * m_fDepth;
    mMatrix.SetTranslation(fScale * mMatrix.GetTranslation());
}
