#pragma once

struct ZephyrAnimBlockHeader
{
    unsigned int uiID;
    unsigned int uiSize;
};
 

// 8-byte aligned: Steam's ZephyrAnimBankReader::ReadFile aligns its stack frame to keep its copy
// of the header at an 8-byte boundary, though no member needs it. Still 24 bytes.
struct __declspec(align(8)) ZephyrAnimFileHeaderD
{
    ZephyrAnimBlockHeader kHeader;
    unsigned short u16MajorVersion;
    unsigned short u16MinorVersion;
    unsigned int u32NumTotalJoints;
    unsigned int u32DatChunkSize;
    unsigned short u16NumAnims;
    unsigned char u8DebugLevel;
    unsigned char u8Padding;
};
static_assert(sizeof(ZephyrAnimFileHeaderD) == 24, "ZephyrAnimFileHeaderD is the file's 24-byte header");
 

struct ZephyrAnimD
{
    unsigned __int64 u64TransFlags;
    unsigned int u32CrcName;
    unsigned short u16NumFrames;
    unsigned short u16NumJoints;
};
 

struct ZephyrAnimJointD
{
    unsigned int u32CrcName;
    unsigned int u32Quat[4];
    unsigned int u32Tran[3];
    float fTranOffset[3];
    float fTranScale;
};
 

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
 
