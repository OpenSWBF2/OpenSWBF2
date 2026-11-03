#include "ZephyrAnimJoint.h"

#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "ZephyrAnim.h"

// FUNCTION: SWBF2 0x0072e440
void ZephyrAnimJoint::DecompTranToFrame(PblVector3& kTrans, unsigned short u16Frame,
                                        DecompTranState& kState) const
{
    if (!m_bTransDyn)
    {
        kTrans = PblVector3(m_fTranOffset[0], m_fTranOffset[1], m_fTranOffset[2]);
        return;
    }

    if (kState.u16Frame == u16Frame + 1)
    {
        kTrans = PblVector3((float)kState.i16PrevVal[0] * m_fTranScale + m_fTranOffset[0],
                            (float)kState.i16PrevVal[1] * m_fTranScale + m_fTranOffset[1],
                            (float)kState.i16PrevVal[2] * m_fTranScale + m_fTranOffset[2]);
        return;
    }

    if (kState.u16Frame > u16Frame)
    {
        for (int i = 0; i < 3; ++i)
        {
            kState.pi8Dat[i] = m_pi8Tran[i] + 2;
            kState.i16Val[i] = (short)(((unsigned char)m_pi8Tran[i][1] << 8) | (unsigned char)m_pi8Tran[i][0]);
            kState.u8RLECount[i] = 0;
        }
        kState.u16Frame = 0;
    }

    if (kState.u16Frame != u16Frame)
    {
        for (int i = 0; i < 3; ++i)
        {
            unsigned short u16Cur = kState.u16Frame;
            if (kState.u8RLECount[i])
            {
                u16Cur = (unsigned short)(u16Cur + kState.u8RLECount[i]);
                if (u16Cur >= u16Frame)
                {
                    kState.u8RLECount[i] = (unsigned char)(u16Cur - u16Frame);
                    continue;
                }
                kState.u8RLECount[i] = 0;
            }

            char* pi8Dat = kState.pi8Dat[i];
            short i16Val = kState.i16Val[i];
            short i16PrevVal = kState.i16PrevVal[i];
            while (u16Cur < u16Frame)
            {
                // 0x80 n: the value holds for n frames; 0x81 lo hi: a new 16-bit value;
                // anything else: a signed 8-bit delta to the value.
                switch (*pi8Dat)
                {
                case (char)0x80:
                    i16PrevVal = i16Val;
                    u16Cur = (unsigned short)(u16Cur + (unsigned char)pi8Dat[1]);
                    if (u16Cur > u16Frame)
                    {
                        kState.u8RLECount[i] = (unsigned char)(u16Cur - u16Frame);
                    }
                    pi8Dat += 2;
                    break;
                case (char)0x81:
                    i16PrevVal = i16Val;
                    ++u16Cur;
                    i16Val = (short)(((unsigned char)pi8Dat[2] << 8) | (unsigned char)pi8Dat[1]);
                    pi8Dat += 3;
                    break;
                default:
                    i16PrevVal = i16Val;
                    ++u16Cur;
                    i16Val = (short)(i16Val + *pi8Dat);
                    ++pi8Dat;
                    break;
                }
            }
            kState.pi8Dat[i] = pi8Dat;
            kState.i16Val[i] = i16Val;
            kState.i16PrevVal[i] = i16PrevVal;
        }
        kState.u16Frame = u16Frame;
    }

    kTrans = PblVector3((float)kState.i16Val[0] * m_fTranScale + m_fTranOffset[0],
                        (float)kState.i16Val[1] * m_fTranScale + m_fTranOffset[1],
                        (float)kState.i16Val[2] * m_fTranScale + m_fTranOffset[2]);
}

// FUNCTION: SWBF2 0x0072e6b0
void ZephyrAnimJoint::GetTranslationUncached(PblVector3& kTrans, unsigned short u16Frame) const
{
    DecompTranState kState;
    kState.Invalidate();
    DecompTranToFrame(kTrans, u16Frame, kState);
}

// The four rotation channels hold the quaternion's x, y, z and s as fixed-point values (1.0 = 2047).
static inline PblQuaternion QuatFromChannels(const short* pi16Val)
{
    const float fScale = 1.0f / 2047.0f;
    return PblQuaternion((float)pi16Val[3] * fScale, (float)pi16Val[0] * fScale,
                         (float)pi16Val[1] * fScale, (float)pi16Val[2] * fScale);
}

// FUNCTION: SWBF2 0x0072e210
void ZephyrAnimJoint::DecompQuatToFrame(PblQuaternion& kQuat, unsigned short u16Frame, DecompQuatState& kState) const
{
    if (kState.u16Frame == u16Frame + 1)
    {
        kQuat = QuatFromChannels(kState.i16PrevVal);
        return;
    }

    if (kState.u16Frame > u16Frame)
    {
        for (int i = 0; i < 4; ++i)
        {
            kState.pi8Dat[i] = m_pi8Quat[i] + 2;
            kState.i16Val[i] = (short)(((unsigned char)m_pi8Quat[i][1] << 8) | (unsigned char)m_pi8Quat[i][0]);
            kState.u8RLECount[i] = 0;
        }
        kState.u16Frame = 0;
    }

    if (kState.u16Frame != u16Frame)
    {
        for (int i = 0; i < 4; ++i)
        {
            unsigned short u16Cur = kState.u16Frame;
            if (kState.u8RLECount[i])
            {
                u16Cur = (unsigned short)(u16Cur + kState.u8RLECount[i]);
                if (u16Cur >= u16Frame)
                {
                    kState.u8RLECount[i] = (unsigned char)(u16Cur - u16Frame);
                    continue;
                }
                kState.u8RLECount[i] = 0;
            }

            char* pi8Dat = kState.pi8Dat[i];
            short i16Val = kState.i16Val[i];
            short i16PrevVal = kState.i16PrevVal[i];
            while (u16Cur < u16Frame)
            {
                // 0x80 n: the value holds for n frames; 0x81 lo hi: a new 16-bit value;
                // anything else: a signed 8-bit delta to the value.
                switch (*pi8Dat)
                {
                case (char)0x80:
                    i16PrevVal = i16Val;
                    u16Cur = (unsigned short)(u16Cur + (unsigned char)pi8Dat[1]);
                    if (u16Cur > u16Frame)
                    {
                        kState.u8RLECount[i] = (unsigned char)(u16Cur - u16Frame);
                    }
                    pi8Dat += 2;
                    break;
                case (char)0x81:
                    i16PrevVal = i16Val;
                    ++u16Cur;
                    i16Val = (short)(((unsigned char)pi8Dat[2] << 8) | (unsigned char)pi8Dat[1]);
                    pi8Dat += 3;
                    break;
                default:
                    i16PrevVal = i16Val;
                    ++u16Cur;
                    i16Val = (short)(i16Val + *pi8Dat);
                    ++pi8Dat;
                    break;
                }
            }
            kState.pi8Dat[i] = pi8Dat;
            kState.i16Val[i] = i16Val;
            kState.i16PrevVal[i] = i16PrevVal;
        }
        kState.u16Frame = u16Frame;
    }

    kQuat = QuatFromChannels(kState.i16Val);
}

// FUNCTION: SWBF2 0x0072e6f0
bool ZephyrAnimJoint::Open(const ZephyrAnim* pkAnim, const ZephyrAnimJointD* pkJointD, char* pcData, bool bTransDyn)
{
    m_pkAnim = const_cast<ZephyrAnim*>(pkAnim);
    m_bTransDyn = bTransDyn;
    m_kCrcName = pkJointD->u32CrcName;

    for (int i = 0; i < 4; i++)
    {
        m_pi8Quat[i] = pcData + pkJointD->u32Quat[i];
    }

    for (int i = 0; i < 3; i++)
    {
        m_pi8Tran[i] = m_bTransDyn ? pcData + pkJointD->u32Tran[i] : 0;
        m_fTranOffset[i] = pkJointD->fTranOffset[i];
    }
    m_fTranScale = pkJointD->fTranScale;
    return true;
}
