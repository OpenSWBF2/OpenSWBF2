#pragma once

#include "ZephyrFile.h"

class ZephyrAnimJoint;

class ZephyrAnim
{
public:
    bool Open(const ZephyrAnimD* pkAnimD, const ZephyrAnimJointD* pkJointD, ZephyrAnimJoint* pkJoints, char* pcData);
    unsigned int GetNameCRC() const { return m_kCrcName; }

    unsigned short GetNumFrames() const { return m_u16NumFrames; }

    ZephyrAnimJoint* m_pkJoints;
    unsigned int m_kCrcName;
    unsigned short m_u16NumFrames;
    unsigned short m_u16NumJoints;
};
 
