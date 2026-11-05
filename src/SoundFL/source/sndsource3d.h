#pragma once

#include "sndglobals.h"
#include "PebbleFL/Common/PblVector.h"

class PblFileChunk;

#include "win/sndlistener.h"

namespace Snd
{

struct GainMatrix;

class Source3D
{
public:
    // FUNCTION: SWBF2 0x00537ef0
    ~Source3D()
    {
    }
    static float sPitchMultiplier;
    enum Flags
    {

        flags_pos3DChanged = 1,
        flags_vel3DChanged = 2,

        flags_updateListener = 4
    };

    class Properties
    {
    public:
        enum Flags
        {

            flags_minDistanceChanged = 4,
            flags_muteDistanceChanged = 8,
            flags_maxDistanceChanged = 16,

            flags_dopplerMode = 64,

            flags_centrePeakMode = 128
        };

        Properties();

        enum
        {
            flags_updateRollIn = 1,
            flags_updateRollInDist = 2,
            flags_updateRollOff = 32
        };

        void SetRollIn(float fRollIn)
        {
            mFlags |= flags_updateRollIn;
            mRollInFactor = fRollIn;
        }
        void SetRollInDistance(float fDistance)
        {
            mFlags |= flags_updateRollInDist;
            mRollInDistance = fDistance;
        }
        void SetRollOff(float fRollOff)
        {
            mFlags |= flags_updateRollOff;
            mRollOffFactor = fRollOff;
        }

        bool GetDopplerMode() const { return (mFlags & flags_dopplerMode) != 0; }
        bool GetCentrePeakMode() const { return (mFlags & flags_centrePeakMode) != 0; }

    protected:

        void CopyParameters(const Properties& rOther);
        friend class Source3D;
    public:
        void SetDopplerMode(bool bDoppler)
        {
            if (bDoppler)
            {
                mFlags |= flags_dopplerMode;
            }
            else
            {
                mFlags &= (unsigned char)~(unsigned int)flags_dopplerMode;
            }
        }

    protected:

        bool Read(PblFileChunk& chunk, unsigned int uiPropertyID);

    public:

        void SetMinDistance(float fDistance)
        {
            mFlags |= flags_minDistanceChanged;
            mMinDistance = fDistance;
        }

        void SetMuteDistance(float fDistance)
        {
            mFlags |= flags_muteDistanceChanged;
            mMuteDistance = fDistance;
        }

        void SetSpaceDistance(float fDistance) { mSpaceDistance = fDistance; }
        void SetMaxDistance(float fDistance)
        {
            mFlags |= flags_maxDistanceChanged;
            mMaxDistance = fDistance;
        }

        float mRollInFactor;
        float mRollInDistance;
        float mMinDistance;
        float mMuteDistance;
        float mMaxDistance;
        float mRollOffFactor;
        float mOffCentreGain;
        float mSpaceDistance;
        unsigned char mFlags;
    };

    explicit Source3D(Properties* pProps = 0);

    void SetPosVel3D(::PblVector3* pPos, ::PblVector3* pVel)
    {
        SetPosition(pPos);
        SetVelocity(pVel);
    }

    void SetPosition(const ::PblVector3* pPos)
    {
        mFlags |= flags_pos3DChanged;
        mPos3D = pPos ? const_cast< ::PblVector3*>(pPos) : &Globals::smNullVector;
    }
    void SetVelocity(const ::PblVector3* pVel)
    {
        mFlags |= flags_vel3DChanged;
        mVel3D = pVel ? const_cast< ::PblVector3*>(pVel) : &Globals::smNullVector;
    }

    void SetProperties(Properties* pProps) { mProps = pProps; }
    void AssignClosestListener(Snd::Listener* pListener, const ::PblVector3& rvDistance,
                               const float& rfDistanceLength)
    {
        mClosestListener = pListener;
        mDistance = rvDistance;
        mDistanceLength = rfDistanceLength;
        mFlags &= (unsigned char)~(unsigned int)flags_updateListener;
    }

    void CalculateGain(float& rfGain) const;

    void CalculateGain(float fGain, float fReverbGain, GainMatrix& rMatrix) const;

    float CalculatePitch() const;

    void DetermineClosestListener();

    void CopyParameters(const Source3D& rOther);

protected:

    void CalculateNormalizedDirection(::PblVector3& rvDirection) const;

public:
    Snd::Listener* mClosestListener;
    ::PblVector3 mDistance;
    float mDistanceLength;
    ::PblVector3* mPos3D;
    ::PblVector3* mVel3D;
    Properties* mProps;

    mutable float mPrevAngle;
    mutable unsigned char mFlags;
};

 
 

}
