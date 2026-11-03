#include "ZephyrPoseDyn.h"

// TEMPLATE: SWBF2 0x0072cc90
// ZephyrPoseDyn<32>::ZephyrPoseDyn<32>
template <int N>
ZephyrPoseDyn<N>::ZephyrPoseDyn()
    : m_pSkel(0),
      m_kRootTrans(0.0f, 0.0f, 0.0f),
      m_fAnimSpeed(30.0f),
      m_fCurT(0.0f),
      m_fLastT(0.0f),
      m_pEventList(0),
      m_bLoop(false),
      m_bPlayingBackward(false),
      m_bAnimFinished(false),
      m_bInterpolate(true)
{
}

// TEMPLATE: SWBF2 0x0072cd70
// ZephyrPoseDyn<32>::~ZephyrPoseDyn<32>
template <int N>
ZephyrPoseDyn<N>::~ZephyrPoseDyn()
{
    Close();
}

// TEMPLATE: SWBF2 0x0072d3f0
// ZephyrPoseDyn<32>::Open
template <int N>
void ZephyrPoseDyn<N>::Open(const ZephyrSkeleton<N>& kSkel)
{
    Close();

    m_pSkel = const_cast<ZephyrSkeleton<N>*>(&kSkel);
}

// TEMPLATE: SWBF2 0x0072cd90
// ZephyrPoseDyn<32>::Close
template <int N>
inline void ZephyrPoseDyn<N>::Close()
{
    m_kAnim.SetAnim(NULL, NULL);
}

// TEMPLATE: SWBF2 0x0072d430 SYMBOL
// ?SetAnimation@?$ZephyrPoseDyn@$0CA@@@QAEXPBVZephyrAnim@@M@Z
template <int N>
void ZephyrPoseDyn<N>::SetAnimation(const ZephyrAnim* pkAnim, float fSpeed)
{
    if (pkAnim)
    {
        m_kAnim.SetAnim(pkAnim, m_pSkel);
        m_fCurT = 0.0f;
        m_bAnimFinished = false;
        ZephyrTransform kTemp;
        GetJointTransform(kTemp, 0);
        m_kRootTrans = kTemp.kTrans;
    }
    else
    {
        m_kAnim.SetAnim(NULL, m_pSkel);
        m_fCurT = 0.0f;
        m_bAnimFinished = false;
    }

    m_fAnimSpeed = fSpeed;
}

// TEMPLATE: SWBF2 0x0072d5b0
// ZephyrPoseDyn<32>::SetFrameNum
template <int N>
bool ZephyrPoseDyn<N>::SetFrameNum(int iFrameNum)
{
    float fTime = (float)(GetNumFrames() - 1);

    if (fTime > 0.0f)
    {
        fTime = (float)iFrameNum / fTime;
    }

    SetCurTime(fTime);

    return true;
}

// TEMPLATE: SWBF2 0x0072d650
// ZephyrPoseDyn<32>::SetFrameNumFloat
template <int N>
bool ZephyrPoseDyn<N>::SetFrameNumFloat(float fFrameNum)
{
    float fTime = (float)(GetNumFrames() - 1);

    if (fTime > 0.0f)
    {
        fTime = fFrameNum / fTime;
    }

    SetCurTime(fTime);

    return true;
}

// TEMPLATE: SWBF2 0x0072d790 SYMBOL
// ?Update@?$ZephyrPoseDyn@$0CA@@@QAEXM@Z
template <int N>
void ZephyrPoseDyn<N>::Update(float fElapsed)
{
    m_fLastT = m_fCurT;

    m_iEventsIter = 0;

    unsigned short u16NumFrames = m_kAnim.GetNumFrames();
    if (u16NumFrames <= 1)
    {
        m_fCurT = 0.0f;
        if (!m_bLoop)
        {
            m_bAnimFinished = true;
        }
        return;
    }

    if (!m_bPlayingBackward)
    {

        if (m_bLoop || !m_bAnimFinished)
        {
            m_fCurT += m_fAnimSpeed * fElapsed / (float)(u16NumFrames - 1);
        }

        if (m_fCurT > 1.0f)
        {
            if (m_bLoop)
            {
                m_fCurT -= (float)(int)m_fCurT;
            }
            else
            {
                m_fCurT = 1.0f;
            }

            m_bAnimFinished = true;
            return;
        }
    }
    else
    {

        if (m_bLoop || !m_bAnimFinished)
        {
            m_fCurT -= m_fAnimSpeed * fElapsed / (float)(u16NumFrames - 1);
        }

        if (m_fCurT < 0.0f)
        {
            if (m_bLoop)
            {
                m_fCurT += (float)(1 - (int)m_fCurT);
            }
            else
            {
                m_fCurT = 0.0f;
            }

            m_bAnimFinished = true;
            return;
        }
    }

    if (m_bLoop)
    {
        m_bAnimFinished = false;
    }
}

// TEMPLATE: SWBF2 0x0072cdc0
// ZephyrPoseDyn<32>::GetJointTransform
template <int N>
void ZephyrPoseDyn<N>::GetJointTransform(ZephyrTransform& kTransform, unsigned short u16JointNum, float fT) const
{
    int iCurFrame = (int)((float)(m_kAnim.GetNumFrames() - 1) * fT);
    m_kAnim.GetTransformCached(kTransform, u16JointNum, (unsigned short)iCurFrame);

    if (m_bInterpolate)
    {
        int nextFrame = iCurFrame + 1;
        ZephyrTransform nextTransform;

        if (nextFrame >= (int)m_kAnim.GetNumFrames())
        {
            if (m_bLoop)
            {
                m_kAnim.GetTransformCached(nextTransform, u16JointNum, 1);
                PblVector3 kLastFrameTrans;
                m_kAnim.GetTranslationCached(kLastFrameTrans, u16JointNum, m_kAnim.GetNumFrames() - 1);
                nextTransform.kTrans += kLastFrameTrans;
            }
            else
            {
                m_kAnim.GetTransformCached(nextTransform, u16JointNum, m_kAnim.GetNumFrames() - 1);
            }
        }
        else
        {
            m_kAnim.GetTransformCached(nextTransform, u16JointNum, (unsigned short)nextFrame);
        }

        float perc2NextFrame = fT * (m_kAnim.GetNumFrames() - 1);
        perc2NextFrame -= (float)(int)perc2NextFrame;

        kTransform.kQuat = Interpolate(kTransform.kQuat, nextTransform.kQuat, perc2NextFrame);
        kTransform.kTrans = Interpolate(kTransform.kTrans, nextTransform.kTrans, perc2NextFrame);
    }
}

// TEMPLATE: SWBF2 0x0072cda0
// ZephyrPoseDyn<32>::GetJointTransform
template <int N>
inline void ZephyrPoseDyn<N>::GetJointTransform(ZephyrTransform& kTransform, unsigned short u16JointNum) const
{
    GetJointTransform(kTransform, u16JointNum, m_fCurT);
}

// TEMPLATE: SWBF2 0x0072d180
// ZephyrPoseDyn<64>::GetRootTransDelta
template <int N>
PblVector3 ZephyrPoseDyn<N>::GetRootTransDelta()
{
    ZephyrTransform kTemp;

    PblVector3 kLastRootTrans;
    kLastRootTrans = m_kRootTrans;
    GetJointTransform(kTemp, 0);
    m_kRootTrans = kTemp.kTrans;

    if (m_bLoop)
    {
        if (!m_bPlayingBackward && (m_fLastT > m_fCurT))
        {
            GetJointTransform(kTemp, 0, 1.0f);
            PblVector3 kBeforeLoop = kTemp.kTrans - kLastRootTrans;
            GetJointTransform(kTemp, 0, 0.0f);
            return (m_kRootTrans - kTemp.kTrans) + kBeforeLoop;
        }
        else if (m_bPlayingBackward && (m_fLastT < m_fCurT))
        {
            GetJointTransform(kTemp, 0, 0.0f);
            PblVector3 kBeforeLoop = kTemp.kTrans - kLastRootTrans;
            GetJointTransform(kTemp, 0, 1.0f);
            return (m_kRootTrans - kTemp.kTrans) + kBeforeLoop;
        }
    }

    return m_kRootTrans - kLastRootTrans;
}

template class ZephyrPoseDyn<32>;
template class ZephyrPoseDyn<64>;

// TEMPLATE: SWBF2 0x0072cd80
// ZephyrPoseDyn<64>::~ZephyrPoseDyn<64>

// TEMPLATE: SWBF2 0x0072cf90
// ZephyrPoseDyn<64>::GetJointTransform

// TEMPLATE: SWBF2 0x0072cfb0
// ZephyrPoseDyn<64>::GetJointTransform

// TEMPLATE: SWBF2 0x0072d410
// ZephyrPoseDyn<64>::Open

// TEMPLATE: SWBF2 0x0072d4f0
// ZephyrPoseDyn<64>::SetAnimation

// TEMPLATE: SWBF2 0x0072d6f0
// ZephyrPoseDyn<64>::SetFrameNumFloat

// TEMPLATE: SWBF2 0x0072cd00
// ZephyrPoseDyn<64>::ZephyrPoseDyn<64>

// TEMPLATE: SWBF2 0x0072d910
// ZephyrPoseDyn<64>::Update
