#pragma once

#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblHashTable.h"

class RedSkeleton;
class GameObject;
template <int N> class ZephyrSkeleton;
template <int N> class ZephyrPoseStatic;

class RedPose : public PblHashTable<const PblMatrix, 128>
{
public:

    // FUNCTION: SWBF2 0x00443940
    RedPose() : m_pSkeleton(0) {}

    template <int N> unsigned int ConvertFromZephyrPose(const ZephyrSkeleton<N>& kSkel);

    template <int N> unsigned int ConvertFromZephyrPose(const ZephyrPoseStatic<N>& kPose);

    void BindToSkeleton(RedSkeleton const* pSkeleton) const;
    void GetHardSkinMatrices(PblMatrix* pMatrices, unsigned char const* pIndices,
                             unsigned int uiCount) const;
    void GetSkinMatrices(PblMatrix* pMatrices, unsigned char const* pIndices,
                         unsigned int uiCount) const;

private:

    friend class RedModel;
    friend class RedStencilShadow;
    friend void RemoveChildren(const GameObject*, RedPose&, int);
    PblMatrix const& _GetBoneMatrix(int iBone) const;

    mutable const RedSkeleton* m_pSkeleton;
};

 
