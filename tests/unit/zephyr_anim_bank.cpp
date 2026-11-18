#include "third_party/doctest/doctest.h"

#include <string.h>
#include <vector>

#include "Zephyr/Source/ZephyrAnimBank.h"
#include "Zephyr/Source/ZephyrAnimJoint.h"
#include "Zephyr/Source/ZephyrFile.h"

TEST_CASE("zephyr anim bank keeps every joint and the frame data apart" * doctest::test_suite("regression"))
{
    const unsigned short u16NumAnims = 2;
    const unsigned short au16Joints[u16NumAnims] = { 2, 3 };
    const unsigned int u32NumTotalJoints = 5;
    const unsigned int u32DatSize = 128;

    ZephyrAnimFileHeaderD kHeader;
    memset(&kHeader, 0, sizeof(kHeader));
    kHeader.u16NumAnims = u16NumAnims;
    kHeader.u32NumTotalJoints = u32NumTotalJoints;
    kHeader.u32DatChunkSize = u32DatSize;

    unsigned int uDescSize = 0;
    unsigned int uRuntimeSize = 0;
    ZephyrAnimBank::GetRequiredMemSize(&kHeader, uDescSize, uRuntimeSize);
    CHECK(uRuntimeSize == u32NumTotalJoints * sizeof(ZephyrAnimJoint) + u16NumAnims * sizeof(ZephyrAnim));
    CHECK(uDescSize == 3 * sizeof(ZephyrAnimBlockHeader) + u16NumAnims * sizeof(ZephyrAnimD) + u32NumTotalJoints * sizeof(ZephyrAnimJointD));

    std::vector<unsigned long long> aDesc(uDescSize / 8 + 1, 0);
    char* pcDesc = (char*)&aDesc[0];
    ZephyrAnimBlockHeader* pkBlock = (ZephyrAnimBlockHeader*)pcDesc;
    pkBlock->uiID = 'ANIM';
    ZephyrAnimD* pkAnimD = (ZephyrAnimD*)(pkBlock + 1);
    for (unsigned short i = 0; i < u16NumAnims; ++i)
    {
        pkAnimD[i].u64TransFlags = 0x1;
        pkAnimD[i].u32CrcName = 0xA000u + i;
        pkAnimD[i].u16NumFrames = 4;
        pkAnimD[i].u16NumJoints = au16Joints[i];
    }
    pkBlock = (ZephyrAnimBlockHeader*)(pkAnimD + u16NumAnims);
    pkBlock->uiID = 'AJNT';
    ZephyrAnimJointD* pkJointD = (ZephyrAnimJointD*)(pkBlock + 1);
    for (unsigned int j = 0; j < u32NumTotalJoints; ++j)
    {
        pkJointD[j].u32CrcName = 0xB000u + j;
        for (unsigned int k = 0; k < 4; ++k)
        {
            pkJointD[j].u32Quat[k] = j * 24 + k * 2;
        }
        for (unsigned int k = 0; k < 3; ++k)
        {
            pkJointD[j].u32Tran[k] = j * 24 + 8 + k * 2;
            pkJointD[j].fTranOffset[k] = (float)(j + k);
        }
        pkJointD[j].fTranScale = 0.5f;
    }
    pkBlock = (ZephyrAnimBlockHeader*)(pkJointD + u32NumTotalJoints);
    pkBlock->uiID = 'ADAT';

    std::vector<unsigned long long> aMem((uRuntimeSize + u32DatSize) / 8 + 1, 0);
    char* pcMem = (char*)&aMem[0];
    char* pcData = pcMem + uRuntimeSize;
    for (unsigned int i = 0; i < u32DatSize; ++i)
    {
        pcData[i] = (char)(0x40 + i);
    }

    ZephyrAnimBank* pBank = new ZephyrAnimBank;
    REQUIRE(pBank->Open(pcMem, uRuntimeSize + u32DatSize, pcDesc, &kHeader));
    REQUIRE(pBank->m_pkAnims == (ZephyrAnim*)pcMem);

    ZephyrAnimJoint* pkFirst = (ZephyrAnimJoint*)(pBank->m_pkAnims + u16NumAnims);
    unsigned int j = 0;
    for (unsigned short i = 0; i < u16NumAnims; ++i)
    {
        const ZephyrAnim& rAnim = pBank->m_pkAnims[i];
        CHECK(rAnim.GetNameCRC() == 0xA000u + i);
        CHECK(rAnim.m_u16NumJoints == au16Joints[i]);
        CHECK(rAnim.m_pkJoints == pkFirst + j);
        CHECK(pBank->Find(0xA000u + i) == &rAnim);
        for (unsigned short n = 0; n < au16Joints[i]; ++n, ++j)
        {
            const ZephyrAnimJoint& rJoint = pkFirst[j];
            CHECK(rJoint.GetNameCRC() == 0xB000u + j);
            CHECK(rJoint.m_pkAnim == &rAnim);
            CHECK(rJoint.m_bTransDyn == (n == 0));
            for (unsigned int k = 0; k < 4; ++k)
            {
                CHECK(rJoint.m_pi8Quat[k] == pcData + j * 24 + k * 2);
            }
            for (unsigned int k = 0; k < 3; ++k)
            {
                CHECK(rJoint.m_pi8Tran[k] == (n == 0 ? pcData + j * 24 + 8 + k * 2 : (char*)0));
                CHECK(rJoint.m_fTranOffset[k] == (float)(j + k));
            }
            CHECK(rJoint.m_fTranScale == 0.5f);
        }
    }
    CHECK(j == u32NumTotalJoints);
    for (unsigned int i = 0; i < u32DatSize; ++i)
    {
        CHECK(pcData[i] == (char)(0x40 + i));
    }
    delete pBank;
}
