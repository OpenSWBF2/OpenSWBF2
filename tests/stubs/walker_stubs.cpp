#include "Battlefront2/Source/WalkerAnimation.h"

#include "PebbleFL/Common/PblBitVector.h"
#include "PebbleFL/Common/PblVector.h"
#include "Zephyr/Source/ZephyrAnimBank.h"
#include "Zephyr/Source/ZephyrAnimInst.h"
#include "Zephyr/Source/ZephyrAnimJoint.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrPoseStatic.h"

const ZephyrAnim* ZephyrAnimBank::Find(unsigned int uiCrc) const
{
    return 0;
}

void ZephyrAnimJoint::GetTranslationUncached(PblVector3& kTrans, unsigned short u16Frame) const
{
}

template <>
ZephyrAnimInst<64>::ZephyrAnimInst()
{
}

template <>
ZephyrPoseDyn<64>::ZephyrPoseDyn()
{
}

template <>
ZephyrPoseDyn<64>::~ZephyrPoseDyn()
{
}

template <>
void ZephyrPoseDyn<64>::Open(const ZephyrSkeleton<64>& kSkel)
{
}

template <>
void ZephyrPoseDyn<64>::Update(float fElapsed)
{
}

template <>
void ZephyrPoseDyn<64>::SetAnimation(const ZephyrAnim* pkAnim, float fAnimSpeed)
{
}

template <>
void ZephyrPoseDyn<64>::GetJointTransform(ZephyrTransform& kTransform, unsigned short u16JointNum) const
{
}

template <>
PblVector3 ZephyrPoseDyn<64>::GetRootTransDelta()
{
    return PblVector3(0.0f, 0.0f, 0.0f);
}

template <>
bool ZephyrPoseDyn<64>::SetFrameNumFloat(float fFrameNum)
{
    return false;
}

template <>
void ZephyrPoseStatic<64>::Set(const ZephyrPoseDyn<64>& kPose, const PblBitVector<64>& kMask, bool bForce)
{
}

template <>
void ZephyrPoseStatic<64>::Blend(const ZephyrPoseDyn<64>& kPose, const PblBitVector<64>& kMask, float fWeight)
{
}
