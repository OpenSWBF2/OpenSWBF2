#pragma once

#include "PebbleFL/Common/PblMatrix.h"

class ZephyrSkeletonShared;
class ZephyrJointShared;
template <int N> class ZephyrPoseStatic;
template <int N> class PblBitVector;

template <int N>
class ZephyrSkeleton
{
public:

    ZephyrSkeleton();
    ~ZephyrSkeleton();

    bool Open(const ZephyrSkeletonShared& kShared, const ZephyrPoseStatic<N>& kPoseFinal);

    bool Open(const ZephyrSkeletonShared& kShared);
    void Close();

    void SetSharedSkeleton(const ZephyrSkeletonShared* pShared) { m_pShared = const_cast<ZephyrSkeletonShared*>(pShared); }
    void BuildWorldMatrices(const PblMatrix& kRootMatrix);
    void BuildWorldMatrices(const PblMatrix& kRootMatrix, int iJoint);
    void BuildWorldMatrices(const PblMatrix& kRootMatrix, PblBitVector<N>& kDirty);
    void BuildWorldMatrices(const PblMatrix& kRootMatrix, int iJoint, PblBitVector<N>& kDirty);
    void BuildWorldMatrices(const PblMatrix& kRootMatrix, const PblBitVector<N>& kMask);

    PblMatrix* GetWorldMatrix(int iJoint) { return &m_pWorldMatrices[iJoint]; }
    const PblMatrix* GetWorldMatrix(int iJoint) const { return &m_pWorldMatrices[iJoint]; }
    int GetNumJoints() const { return m_pShared->m_uNumJoints; }
    const ZephyrJointShared* GetJointShared(int iJoint) const { return &m_pShared->m_pJoints[iJoint]; }

private:
    void BuildWorldMatricesRecurseDown(char iJoint);
    void BuildWorldMatricesRecurseDown(char iJoint, PblBitVector<N>& kDirty);
    void BuildWorldMatricesRecurseUp(char iJoint);
    void BuildWorldMatricesRecurseUp(char iJoint, PblBitVector<N>& kDirty);

public:
    ZephyrSkeletonShared* m_pShared;
    ZephyrPoseStatic<N>* m_pPoseFinal;
    PblMatrix m_pWorldMatrices[N];
};
