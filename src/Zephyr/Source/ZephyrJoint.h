#pragma once

#include "PebbleFL/Common/PblMatrix.h"
#include "ZephyrCommon.h"

class ZephyrJointShared
{
public:
    PblMatrix m_kBaseInverse;
    ZephyrTransform m_kBaseTransform;
    unsigned int m_kCrc;
    char m_iParent;
    char m_iChild;
    char m_iSibling;
    unsigned char mTransformFlags;
    float mScale;
};

class ZephyrJointSet
{
public:
    unsigned char m_puJoints[32];
    unsigned char m_puSkinSets[16];
    unsigned char m_uNumJoints;
    unsigned char m_uNumSkinSets;
};
