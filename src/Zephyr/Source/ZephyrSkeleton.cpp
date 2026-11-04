#include <d3dx9math.h>
#include "ZephyrSkeleton.h"
#include "ZephyrSkeletonShared.h"
#include "ZephyrPoseStatic.h"
#include "PebbleFL/Common/PblBitVector.h"


// SYNTHETIC: SWBF2 0x0040b0f0
// `dynamic initializer for 'kuiDummyRootHash''

// GLOBAL: SWBF2 0x009cf404
// kuiDummyRootHash
static const unsigned int kuiDummyRootHash = PblTEMPHash("DummyRoot");

// TEMPLATE: SWBF2 0x0072bbf0
// ZephyrSkeleton<32>::ZephyrSkeleton<32>
template <int N>
ZephyrSkeleton<N>::ZephyrSkeleton()
{
    m_pShared = 0;
}

// TEMPLATE: SWBF2 0x0072bc10
// ZephyrSkeleton<32>::~ZephyrSkeleton<32>
template <int N>
ZephyrSkeleton<N>::~ZephyrSkeleton()
{
    m_pShared = 0;
}

// TEMPLATE: SWBF2 0x0072cc30
// ZephyrSkeleton<32>::Open
template <int N>
bool ZephyrSkeleton<N>::Open(const ZephyrSkeletonShared& kShared)
{
    m_pShared = const_cast<ZephyrSkeletonShared*>(&kShared);
    return true;
}

// TEMPLATE: SWBF2 0x0072cc40
// ZephyrSkeleton<32>::Open
template <int N>
bool ZephyrSkeleton<N>::Open(const ZephyrSkeletonShared& kShared, const ZephyrPoseStatic<N>& kPoseFinal)
{
    m_pShared = const_cast<ZephyrSkeletonShared*>(&kShared);
    m_pPoseFinal = const_cast<ZephyrPoseStatic<N>*>(&kPoseFinal);
    return true;
}

// TEMPLATE: SWBF2 0x0072cc20
// ZephyrSkeleton<32>::Close
template <int N>
void ZephyrSkeleton<N>::Close()
{
    m_pShared = 0;
}

// TEMPLATE: SWBF2 0x0072bc30
// ZephyrSkeleton<32>::BuildWorldMatrices
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatrices(const PblMatrix& kRootMatrix)
{
    const ZephyrTransform* pRoot = m_pPoseFinal->GetJointTransform(0);
    PblMatrix kLocal(pRoot->kQuat, PblVector3(0.0f, 0.0f, 0.0f));
    m_pWorldMatrices[0] = kLocal * kRootMatrix;

    char iChild = m_pShared->m_pJoints[0].m_iChild;
    if (iChild >= 0)
    {
        BuildWorldMatricesRecurseDown(iChild);
    }
}

// TEMPLATE: SWBF2 0x0072bd00
// ZephyrSkeleton<32>::BuildWorldMatrices
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatrices(const PblMatrix& kRootMatrix, PblBitVector<N>& kDirty)
{
    if (kDirty.IsBitSet(0))
    {
        const ZephyrTransform* pRoot = m_pPoseFinal->GetJointTransform(0);
        PblMatrix kLocal(pRoot->kQuat, PblVector3(0.0f, 0.0f, 0.0f));
        m_pWorldMatrices[0] = kLocal * kRootMatrix;
        kDirty.ClearBit(0);
    }

    char iChild = m_pShared->m_pJoints[0].m_iChild;
    if (iChild >= 0)
    {
        BuildWorldMatricesRecurseDown(iChild, kDirty);
    }
}

// TEMPLATE: SWBF2 0x0072bde0
// ZephyrSkeleton<32>::BuildWorldMatrices
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatrices(const PblMatrix& kRootMatrix, const PblBitVector<N>& kMask)
{
    PblBitVector<N> kDirty;
    kDirty.SetAll();

    const ZephyrTransform* pRoot = m_pPoseFinal->GetJointTransform(0);
    PblMatrix kLocal(pRoot->kQuat, PblVector3(0.0f, 0.0f, 0.0f));
    m_pWorldMatrices[0] = kLocal * kRootMatrix;
    kDirty.ClearBit(0);

    int iNumJoints = GetNumJoints();
    for (int i = 0; i < iNumJoints; i++)
    {
        if (kMask.IsBitSet(i) && kDirty.IsBitSet(i))
        {
            BuildWorldMatricesRecurseUp((char)i, kDirty);
        }
    }
}

// TEMPLATE: SWBF2 0x0072bef0
// ZephyrSkeleton<32>::BuildWorldMatrices
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatrices(const PblMatrix& kRootMatrix, int iJoint)
{
    const ZephyrTransform* pRoot = m_pPoseFinal->GetJointTransform(0);
    PblMatrix kLocal(pRoot->kQuat, PblVector3(0.0f, 0.0f, 0.0f));
    m_pWorldMatrices[0] = kLocal * kRootMatrix;

    if (iJoint)
    {
        BuildWorldMatricesRecurseUp((char)iJoint);
    }
}

// TEMPLATE: SWBF2 0x0072bfb0
// ZephyrSkeleton<32>::BuildWorldMatrices
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatrices(const PblMatrix& kRootMatrix, int iJoint, PblBitVector<N>& kDirty)
{
    if (kDirty.IsBitSet(0))
    {
        const ZephyrTransform* pRoot = m_pPoseFinal->GetJointTransform(0);
        PblMatrix kLocal(pRoot->kQuat, PblVector3(0.0f, 0.0f, 0.0f));
        m_pWorldMatrices[0] = kLocal * kRootMatrix;
        kDirty.ClearBit(0);
    }

    if (iJoint)
    {
        BuildWorldMatricesRecurseUp((char)iJoint, kDirty);
    }
}

// TEMPLATE: SWBF2 0x0072c150
// ZephyrSkeleton<32>::BuildWorldMatricesRecurseDown
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatricesRecurseDown(char iJoint)
{
    const ZephyrJointShared* pJoint = &m_pShared->m_pJoints[iJoint];
    if (pJoint->m_kCrc == kuiDummyRootHash)
    {
        m_pWorldMatrices[iJoint] = m_pWorldMatrices[pJoint->m_iParent];
    }
    else
    {
        const ZephyrTransform& kPose = *m_pPoseFinal->GetJointTransform(iJoint);
        const ZephyrTransform& kBase = pJoint->m_kBaseTransform;
        PblVector3 kTrans = (pJoint->mTransformFlags & 1) ? kPose.kTrans : kBase.kTrans;
        if (pJoint->mTransformFlags & 2)
        {
            kTrans *= pJoint->mScale;
        }

        PblMatrix kLocal(kPose.kQuat, kTrans);
        m_pWorldMatrices[iJoint] = kLocal * m_pWorldMatrices[pJoint->m_iParent];
    }

    if (pJoint->m_iSibling >= 0)
    {
        BuildWorldMatricesRecurseDown(pJoint->m_iSibling);
    }

    if (pJoint->m_iChild >= 0)
    {
        BuildWorldMatricesRecurseDown(pJoint->m_iChild);
    }
}

// TEMPLATE: SWBF2 0x0072c360
// ZephyrSkeleton<32>::BuildWorldMatricesRecurseDown
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatricesRecurseDown(char iJoint, PblBitVector<N>& kDirty)
{
    const ZephyrJointShared* pJoint = &m_pShared->m_pJoints[iJoint];
    if (kDirty.IsBitSet(iJoint))
    {
        if (pJoint->m_kCrc == kuiDummyRootHash)
        {
            m_pWorldMatrices[iJoint] = m_pWorldMatrices[pJoint->m_iParent];
        }
        else
        {
            const ZephyrTransform& kPose = *m_pPoseFinal->GetJointTransform(iJoint);
            const ZephyrTransform& kBase = pJoint->m_kBaseTransform;
            PblVector3 kTrans = (pJoint->mTransformFlags & 1) ? kPose.kTrans : kBase.kTrans;
            if (pJoint->mTransformFlags & 2)
            {
                kTrans *= pJoint->mScale;
            }

            PblMatrix kLocal(kPose.kQuat, kTrans);
            m_pWorldMatrices[iJoint] = kLocal * m_pWorldMatrices[pJoint->m_iParent];
        }
        kDirty.ClearBit(iJoint);
    }

    if (pJoint->m_iSibling >= 0)
    {
        BuildWorldMatricesRecurseDown(pJoint->m_iSibling, kDirty);
    }

    if (pJoint->m_iChild >= 0)
    {
        BuildWorldMatricesRecurseDown(pJoint->m_iChild, kDirty);
    }
}

// TEMPLATE: SWBF2 0x0072c7d0
// ZephyrSkeleton<32>::BuildWorldMatricesRecurseUp
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatricesRecurseUp(char iJoint)
{
    const ZephyrJointShared* pJoint = &m_pShared->m_pJoints[iJoint];
    if (pJoint->m_iParent)
    {
        BuildWorldMatricesRecurseUp(pJoint->m_iParent);
    }

    if (pJoint->m_kCrc == kuiDummyRootHash)
    {
        m_pWorldMatrices[iJoint] = m_pWorldMatrices[pJoint->m_iParent];
    }
    else
    {
        const ZephyrTransform& kPose = *m_pPoseFinal->GetJointTransform(iJoint);
        const ZephyrTransform& kBase = pJoint->m_kBaseTransform;
        PblVector3 kTrans = (pJoint->mTransformFlags & 1) ? kPose.kTrans : kBase.kTrans;
        if (pJoint->mTransformFlags & 2)
        {
            kTrans *= pJoint->mScale;
        }

        PblMatrix kLocal(kPose.kQuat, kTrans);
        m_pWorldMatrices[iJoint] = kLocal * m_pWorldMatrices[pJoint->m_iParent];
    }
}

// TEMPLATE: SWBF2 0x0072c9e0
// ZephyrSkeleton<32>::BuildWorldMatricesRecurseUp
template <int N>
void ZephyrSkeleton<N>::BuildWorldMatricesRecurseUp(char iJoint, PblBitVector<N>& kDirty)
{
    kDirty.ClearBit(iJoint);

    const ZephyrJointShared* pJoint = &m_pShared->m_pJoints[iJoint];
    if (kDirty.IsBitSet(pJoint->m_iParent))
    {
        BuildWorldMatricesRecurseUp(pJoint->m_iParent, kDirty);
    }

    if (pJoint->m_kCrc == kuiDummyRootHash)
    {
        m_pWorldMatrices[iJoint] = m_pWorldMatrices[pJoint->m_iParent];
    }
    else
    {
        const ZephyrTransform& kPose = *m_pPoseFinal->GetJointTransform(iJoint);
        const ZephyrTransform& kBase = pJoint->m_kBaseTransform;
        PblVector3 kTrans = (pJoint->mTransformFlags & 1) ? kPose.kTrans : kBase.kTrans;
        if (pJoint->mTransformFlags & 2)
        {
            kTrans *= pJoint->mScale;
        }

        PblMatrix kLocal(kPose.kQuat, kTrans);
        m_pWorldMatrices[iJoint] = kLocal * m_pWorldMatrices[pJoint->m_iParent];
    }
}

template class ZephyrSkeleton<32>;
template class ZephyrSkeleton<64>;

// TEMPLATE: SWBF2 0x0072bc00
// ZephyrSkeleton<64>::ZephyrSkeleton<64>

// TEMPLATE: SWBF2 0x0072bc20
// ZephyrSkeleton<64>::~ZephyrSkeleton<64>

// TEMPLATE: SWBF2 0x0072cc60
// ZephyrSkeleton<64>::Open

// TEMPLATE: SWBF2 0x0072cc70
// ZephyrSkeleton<64>::Open

// TEMPLATE: SWBF2 0x0072c080
// ZephyrSkeleton<64>::BuildWorldMatrices

// TEMPLATE: SWBF2 0x0072c5c0
// ZephyrSkeleton<64>::BuildWorldMatricesRecurseDown
