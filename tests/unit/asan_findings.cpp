#include "third_party/doctest/doctest.h"

#include <limits.h>
#include <new>
#include <string.h>
#include <string>

#include "testutil.h"
#include "PebbleFL/Common/PblIndex.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/ext/Allocator.h"
#include "RedEngineFL/Util/ext/ListAllocator.h"
#include "Battlefront2/Source/CollisionMesh.h"
#include "Battlefront2/Source/SoldierAnimation.h"
#include "Battlefront2/Source/WalkerAnimation.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "PebbleFL/Common/PblHandle.h"
#include "SoundFL/source/sndreleasevoice.h"
#include "PebbleFL/Common/PblListDelete.h"
#include "Battlefront2/Source/EffectAttach.h"
#include "Battlefront2/Source/SquadronFormation.h"
#include "Battlefront2/Source/WalkerLegPairBounds.h"
#include "Battlefront2/Source/HeroSlots.h"

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

TEST_CASE("pbl in bounds accepts only indices inside the array" * doctest::test_suite("regression"))
{
    int aiValues[3] = {};
    CHECK(PblInBounds(aiValues, 0));
    CHECK(PblInBounds(aiValues, 1));
    CHECK(PblInBounds(aiValues, 2));
    CHECK(!PblInBounds(aiValues, -1));
    CHECK(!PblInBounds(aiValues, 3));
    CHECK(!PblInBounds(aiValues, INT_MIN));
}

struct HashTableWithSentinel
{
    PblHashTable<int, 4> mTable;
    unsigned int mSentinel;
};

TEST_CASE("hash table end converts to null" * doctest::test_suite("regression"))
{
    HashTableWithSentinel kHolder;
    memset(&kHolder, 0, sizeof(kHolder));
    kHolder.mSentinel = 0xDEADBEEF;
    CHECK((int*)kHolder.mTable.End() == (int*)0);
    CHECK(kHolder.mTable.End().operator->() == (int*)0);

    int iValue = 9;
    REQUIRE(HashStoreNew(kHolder.mTable, PblHash("one"), &iValue));
    int iCount = 0;
    for (PblHashTable<int, 4>::Itor it = kHolder.mTable.Begin(); it != kHolder.mTable.End(); ++it)
    {
        CHECK(*it == 9);
        ++iCount;
        if (iCount > 4)
        {
            break;
        }
    }
    CHECK(iCount == 1);
}

TEST_CASE("hash table iteration visits null valued entries" * doctest::test_suite("regression"))
{
    PblHashTable<int, 4> kTable;
    kTable.Clear();
    int iValue = 9;
    REQUIRE(HashStoreNew(kTable, 1, (int*)0));
    REQUIRE(HashStoreNew(kTable, 2, &iValue));
    int iCount = 0;
    int iFound = 0;
    for (PblHashTable<int, 4>::Itor it = kTable.Begin(); it != kTable.End(); ++it)
    {
        ++iCount;
        if ((int*)it == &iValue)
        {
            ++iFound;
        }
        if (iCount > 4)
        {
            break;
        }
    }
    CHECK(iCount == 2);
    CHECK(iFound == 1);
}

TEST_CASE("remove polygon keeps the remaining polygons" * doctest::test_suite("regression"))
{
    PolyListContainer<4> kList;
    kList.Clear();
    PblVector3 avFirst[3] = {PblVector3(1.0f, 0.0f, 0.0f), PblVector3(2.0f, 0.0f, 0.0f),
                             PblVector3(3.0f, 0.0f, 0.0f)};
    PblVector3 avSecond[2] = {PblVector3(4.0f, 0.0f, 0.0f), PblVector3(5.0f, 0.0f, 0.0f)};
    PblVector3 avThird[3] = {PblVector3(6.0f, 0.0f, 0.0f), PblVector3(7.0f, 0.0f, 0.0f),
                             PblVector3(8.0f, 0.0f, 0.0f)};
    kList.AddPolygon(3, avFirst);
    kList.AddPolygon(2, avSecond);
    kList.AddPolygon(3, avThird);
    REQUIRE(kList.mPolyCount == 3);

    kList.RemovePolygon(1);
    CHECK(kList.mPolyCount == 2);
    CHECK(kList.mVertCount == 6);

    int iNumVerts = 0;
    const PblVector3* pVerts = 0;
    kList.GetPolygon(0, iNumVerts, pVerts);
    REQUIRE(iNumVerts == 3);
    for (int i = 0; i < 3; ++i)
    {
        CHECK(pVerts[i].x == avFirst[i].x);
    }
    kList.GetPolygon(1, iNumVerts, pVerts);
    REQUIRE(iNumVerts == 3);
    for (int i = 0; i < 3; ++i)
    {
        CHECK(pVerts[i].x == avThird[i].x);
    }
}

TEST_CASE("remove polygon leaves the slot past the count alone" * doctest::test_suite("regression"))
{
    PolyListContainer<4> kList;
    memset(&kList, 0, sizeof(kList));
    kList.Clear();
    PblVector3 avVerts[4] = {PblVector3(1.0f, 0.0f, 0.0f), PblVector3(2.0f, 0.0f, 0.0f),
                             PblVector3(3.0f, 0.0f, 0.0f), PblVector3(4.0f, 0.0f, 0.0f)};
    kList.AddPolygon(1, avVerts);
    kList.AddPolygon(2, avVerts);
    kList.AddPolygon(3, avVerts);
    kList.AddPolygon(4, avVerts);
    REQUIRE(kList.mPolyCount == 4);
    REQUIRE(kList.mVertCount == 10);

    kList.RemovePolygon(0);
    CHECK(kList.mPolyCount == 3);
    CHECK(kList.mVertCount == 9);
    CHECK(kList.mNumVerticesPerPoly[0] == 2);
    CHECK(kList.mNumVerticesPerPoly[1] == 3);
    CHECK(kList.mNumVerticesPerPoly[2] == 4);
    CHECK(kList.mNumVerticesPerPoly[3] == 4);
}

struct HeroIdSlotsProbe
{
    int aiBefore[1];
    int aiSlots[3];
    int aiAfter[1];
};

TEST_CASE("hero slot match ignores teams outside the slot array" * doctest::test_suite("regression"))
{
    HeroIdSlotsProbe probe = { { 9 }, { 4, 5, 6 }, { 9 } };
    CHECK(HeroSlotIs(probe.aiSlots, 1, 5));
    CHECK_FALSE(HeroSlotIs(probe.aiSlots, 1, 6));
    CHECK_FALSE(HeroSlotIs(probe.aiSlots, -1, 9));
    CHECK_FALSE(HeroSlotIs(probe.aiSlots, 3, 9));
}

struct HeroUnlockSlotsProbe
{
    bool abBefore[1];
    bool abSlots[3];
    bool abAfter[1];
};

TEST_CASE("hero unlock check ignores teams outside the slot array" * doctest::test_suite("regression"))
{
    HeroUnlockSlotsProbe probe = { { true }, { false, true, false }, { true } };
    CHECK(HeroSlotSet(probe.abSlots, 1));
    CHECK_FALSE(HeroSlotSet(probe.abSlots, 0));
    CHECK_FALSE(HeroSlotSet(probe.abSlots, -1));
    CHECK_FALSE(HeroSlotSet(probe.abSlots, 3));
}

TEST_CASE("hero slot read outside the slot array returns the fallback" * doctest::test_suite("regression"))
{
    HeroIdSlotsProbe probe = { { 9 }, { 4, 5, 6 }, { 9 } };
    CHECK(HeroSlotOr(probe.aiSlots, 0, -1) == 4);
    CHECK(HeroSlotOr(probe.aiSlots, 2, -1) == 6);
    CHECK(HeroSlotOr(probe.aiSlots, -1, -1) == -1);
    CHECK(HeroSlotOr(probe.aiSlots, 3, -1) == -1);
}

struct HeroKillerSlotsProbe
{
    int aiBefore[1];
    int aiSlots[4];
    int aiAfter[1];
};

TEST_CASE("hero killer write outside the slot array is dropped" * doctest::test_suite("regression"))
{
    HeroKillerSlotsProbe probe = { { 0 }, { -1, -1, -1, -1 }, { 0 } };
    SetHeroSlot(probe.aiSlots, -1, 7);
    SetHeroSlot(probe.aiSlots, 4, 7);
    CHECK(probe.aiBefore[0] == 0);
    CHECK(probe.aiAfter[0] == 0);
    for (int i = 0; i < 4; ++i)
    {
        CHECK(probe.aiSlots[i] == -1);
    }
    SetHeroSlot(probe.aiSlots, 3, 7);
    CHECK(probe.aiSlots[3] == 7);
    CHECK(probe.aiAfter[0] == 0);
}

TEST_CASE("game rules index outside the rules arrays is rejected" * doctest::test_suite("regression"))
{
    int aiNumBots[4] = { -1, -1, 16, 16 };
    CHECK(HasRulesSlot(aiNumBots, 0));
    CHECK(HasRulesSlot(aiNumBots, 3));
    CHECK_FALSE(HasRulesSlot(aiNumBots, 4));
    CHECK_FALSE(HasRulesSlot(aiNumBots, -1));
    CHECK_FALSE(HasRulesSlot(aiNumBots, INT_MIN));
}

TEST_CASE("stack allocator starts at the first size slot" * doctest::test_suite("regression"))
{
    alignas(util::StackAllocator) unsigned char acBuf[sizeof(util::StackAllocator)];
    memset(acBuf, 0x55, sizeof(acBuf));
    util::StackAllocator* pAlloc = new (acBuf) util::StackAllocator(false, true, 16);
    CHECK(pAlloc->bpos_ == 0);
    pAlloc->~StackAllocator();
}

TEST_CASE("stack allocator free after a full size ring keeps the pool" * doctest::test_suite("regression"))
{
    void* pBlock = RedAllocFromHeap(0, 256, 0);
    REQUIRE(pBlock);
    {
        util::StackAllocator alloc(false, true, 16);
        util::Buffer pool(pBlock, 256);
        alloc.Set(pool);
        util::Buffer bufs[8];
        for (int i = 0; i < 8; ++i)
        {
            bufs[i] = util::Buffer(0, 16);
            alloc.Allocate(bufs[i]);
            REQUIRE(bufs[i].data_ == (char*)pBlock + 16 * i);
        }
        alloc.Deallocate(bufs[7]);
        CHECK(alloc.bpos_ >= 0);
        CHECK(alloc.bpos_ < 8);
        util::Buffer again(0, 16);
        alloc.Allocate(again);
        CHECK(again.data_ == (char*)pBlock + 16 * 7);
        CHECK(alloc.pool_.size_ == 256);
        CHECK(alloc.bpos_ >= 0);
        CHECK(alloc.bpos_ < 8);
        util::Buffer rest(0, 128);
        alloc.Allocate(rest);
        CHECK(rest.data_ == (char*)pBlock + 128);
        alloc.Deallocate(rest);
        alloc.Deallocate(again);
        for (int i = 6; i >= 0; --i)
        {
            alloc.Deallocate(bufs[i]);
        }
        CHECK(alloc.pos_ == pBlock);
    }
    RedFreeToHeap(0, pBlock, 0);
}

TEST_CASE("list allocator serves requests of a page or more" * doctest::test_suite("regression"))
{
    util::ListAllocator* pAlloc = util::ListAllocator::Create(0x2000, 0x10000);
    REQUIRE(pAlloc);
    const unsigned int auiSizes[] = { 0x1fe0, 0x2000, 0x3000 };
    for (unsigned int i = 0; i < sizeof(auiSizes) / sizeof(auiSizes[0]); ++i)
    {
        util::Buffer buf(0, auiSizes[i]);
        pAlloc->Allocate(buf);
        char* p = (char*)buf.data_;
        REQUIRE(p);
        CHECK(buf.size_ >= auiSizes[i]);
        memset(p, (int)i + 1, auiSizes[i]);
        CHECK(p[auiSizes[i] - 1] == (char)(i + 1));
        pAlloc->Deallocate(buf);
    }
    delete pAlloc;
}

TEST_CASE("soldier animation fits its pool slot" * doctest::test_suite("regression"))
{
    CHECK(sizeof(SoldierAnimation) == 0xc);
}

TEST_CASE("walker idle setup stays inside its animation list" * doctest::test_suite("regression"))
{
    WalkerAnimationIdle kIdle(0, 0, "idle idle_leftup idle_rightup", false);
    CHECK(!kIdle.m_bHasTiltAnims);
    REQUIRE(kIdle.mAnimations);
    CHECK(!kIdle.mAnimations[0]);

    alignas(WalkerBlend) unsigned char acBlend[sizeof(WalkerBlend)] = {};
    WalkerBlend& rBlend = *reinterpret_cast<WalkerBlend*>(acBlend);
    rBlend.mCount = 5;
    rBlend.mTimeTotal = 7.0f;
    kIdle.Setup(rBlend, 0.5f, 0.25f, true, false);
    CHECK(rBlend.mCount == 0);
    CHECK(rBlend.mTimeTotal == 0.0f);
    CHECK(rBlend.mBlendFactor == 0.0f);
    CHECK(rBlend.mFrameBlendFactor == 0.0f);

    kIdle.Setup(rBlend, 0.5f, 0.0f, false, false);
    CHECK(rBlend.mCount == 0);
    CHECK(rBlend.mBlendFactor == 1.0f);
    CHECK(rBlend.mFrameBlendFactor == 1.0f);
}

extern int gTerrainStubSphereCalls;

struct TerrainStatics
{
    bool bEnable;
    bool bTreeHoles;
    int iPointsPerDimension;
    int iHeightPatchPointsPerDimensionLog2;
    int iHeightPatchesPerDimensionLog2;
    float fMetersPerPoint;
    float fPointsPerMeter;
    float fTerrainCenterX;
    float fTerrainCenterZ;
    RedTerrain::HeightExpand* pHeightExpand;
    unsigned char* puiLuminosity;
};

static TerrainStatics SaveTerrainStatics()
{
    TerrainStatics k;
    k.bEnable = RedTerrain::_bEnable;
    k.bTreeHoles = RedTerrain::_bTreeHoles;
    k.iPointsPerDimension = RedTerrain::_iPointsPerDimension;
    k.iHeightPatchPointsPerDimensionLog2 = RedTerrain::_iHeightPatchPointsPerDimensionLog2;
    k.iHeightPatchesPerDimensionLog2 = RedTerrain::_iHeightPatchesPerDimensionLog2;
    k.fMetersPerPoint = RedTerrain::_fMetersPerPoint;
    k.fPointsPerMeter = RedTerrain::_fPointsPerMeter;
    k.fTerrainCenterX = RedTerrain::_fTerrainCenterX;
    k.fTerrainCenterZ = RedTerrain::_fTerrainCenterZ;
    k.pHeightExpand = RedTerrain::_pHeightExpand;
    k.puiLuminosity = RedTerrain::_puiLuminosity;
    return k;
}

static void RestoreTerrainStatics(const TerrainStatics& k)
{
    RedTerrain::_bEnable = k.bEnable;
    RedTerrain::_bTreeHoles = k.bTreeHoles;
    RedTerrain::_iPointsPerDimension = k.iPointsPerDimension;
    RedTerrain::_iHeightPatchPointsPerDimensionLog2 = k.iHeightPatchPointsPerDimensionLog2;
    RedTerrain::_iHeightPatchesPerDimensionLog2 = k.iHeightPatchesPerDimensionLog2;
    RedTerrain::_fMetersPerPoint = k.fMetersPerPoint;
    RedTerrain::_fPointsPerMeter = k.fPointsPerMeter;
    RedTerrain::_fTerrainCenterX = k.fTerrainCenterX;
    RedTerrain::_fTerrainCenterZ = k.fTerrainCenterZ;
    RedTerrain::_pHeightExpand = k.pHeightExpand;
    RedTerrain::_puiLuminosity = k.puiLuminosity;
}

static void SetTinyTerrain()
{
    RedTerrain::_bEnable = true;
    RedTerrain::_bTreeHoles = true;
    RedTerrain::_iPointsPerDimension = 8;
    RedTerrain::_iHeightPatchPointsPerDimensionLog2 = 2;
    RedTerrain::_iHeightPatchesPerDimensionLog2 = 1;
    RedTerrain::_fMetersPerPoint = 1.0f;
    RedTerrain::_fPointsPerMeter = 1.0f;
    RedTerrain::_fTerrainCenterX = 4.0f;
    RedTerrain::_fTerrainCenterZ = 3.0f;
}

static int HoleProbes(float fX, float fZ)
{
    const int iBefore = gTerrainStubSphereCalls;
    CHECK(!RedTerrain::PointInHole(PblVector3(fX, 0.0f, fZ)));
    return gTerrainStubSphereCalls - iBefore;
}

static void CheckHoleTable()
{
    for (int iGridZ = 0; iGridZ < 8; ++iGridZ)
    {
        for (int iGridX = 0; iGridX < 8; ++iGridX)
        {
            const int iExpected = (iGridX >= 4 && iGridZ >= 4) ? 1 : 0;
            CHECK(HoleProbes((float)iGridX - 4.0f + 0.5f, (float)iGridZ - 3.0f + 0.5f) == iExpected);
        }
    }
    const float afFar[] = {-12.0f, -4.5f, 4.5f, 13.0f};
    for (int iX = 0; iX < 4; ++iX)
    {
        for (int iZ = 0; iZ < 4; ++iZ)
        {
            const bool bHighX = afFar[iX] > 0.0f;
            const bool bHighZ = afFar[iZ] > 0.0f;
            CHECK(HoleProbes(afFar[iX], afFar[iZ] - 1.0f) == ((bHighX && bHighZ) ? 1 : 0));
        }
    }
    CHECK(HoleProbes(40.0f, 0.5f) == 0);
    CHECK(HoleProbes(-3.5f, 40.0f) == 0);
    CHECK(HoleProbes(-40.0f, 2.5f) == 0);
    CHECK(HoleProbes(2.5f, 40.0f) == 1);
}

TEST_CASE("terrain hole test stays inside the patch table" * doctest::test_suite("regression"))
{
    const TerrainStatics kSaved = SaveTerrainStatics();
    SetTinyTerrain();

    RedTerrain::HeightExpand akPadded[4 + 2 * 64];
    memset(akPadded, 0, sizeof(akPadded));
    for (int i = 0; i < 64; ++i)
    {
        akPadded[i].uiFlags = 0xf;
        akPadded[64 + 4 + i].uiFlags = 0xf;
    }
    akPadded[64 + 3].uiFlags = 0xf;
    RedTerrain::_pHeightExpand = &akPadded[64];
    CheckHoleTable();

    RedTerrain::HeightExpand* pExact = new RedTerrain::HeightExpand[4];
    memset(pExact, 0, 4 * sizeof(RedTerrain::HeightExpand));
    pExact[3].uiFlags = 0xf;
    RedTerrain::_pHeightExpand = pExact;
    CheckHoleTable();
    delete[] pExact;

    unsigned char aucLight[128 + 64 + 128];
    memset(aucLight, 0xff, sizeof(aucLight));
    for (int i = 0; i < 64; ++i)
    {
        aucLight[128 + i] = (unsigned char)i;
    }
    RedTerrain::_puiLuminosity = &aucLight[128];
    CHECK(RedTerrain::GetLight(-2.0f, -1.0f) == 18.0f * (1.0f / 255.0f));
    CHECK(RedTerrain::GetLight(-2.0f, 4.6f) == 23.0f * (1.0f / 255.0f));
    CHECK(RedTerrain::GetLight(3.6f, 4.6f) == 63.0f * (1.0f / 255.0f));
    CHECK(RedTerrain::GetLight(30.0f, 30.0f) == 63.0f * (1.0f / 255.0f));
    CHECK(RedTerrain::GetLight(-30.0f, -30.0f) == 0.0f);
    CHECK(RedTerrain::GetLight(-30.0f, 30.0f) == 7.0f * (1.0f / 255.0f));
    CHECK(RedTerrain::GetLight(30.0f, -30.0f) == 56.0f * (1.0f / 255.0f));

    RestoreTerrainStatics(kSaved);
}

static std::string ReadRepoTextLf(const char* pcRelPath)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRelPath, sText));
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] != '\r')
        {
            sOut += sText[i];
        }
    }
    return sOut;
}

struct HandleProbe : PblHandled
{
    static MemoryPool* spPool;

    static void* operator new(size_t uSize) { return spPool->Allocate((unsigned int)uSize); }

    static void operator delete(void* pBlock) { spPool->Free(pBlock); }
};

MemoryPool* HandleProbe::spPool = 0;

TEST_CASE("handle to a freed pool object resolves to null" * doctest::test_suite("regression"))
{
    MemoryPool pool("handleprobe", sizeof(HandleProbe));
    pool.Create(2, sizeof(HandleProbe), 1);
    HandleProbe::spPool = &pool;

    HandleProbe* pProbe = new HandleProbe;
    REQUIRE(pProbe);
    const unsigned int uiId = pProbe->GetHandleId();
    REQUIRE(uiId != 0);
    PblHandle<HandleProbe> h(pProbe);
    PblHandle<HandleProbe> hStale(pProbe);
    CHECK((HandleProbe*)h == pProbe);
    CHECK(h.GetHandleId() == uiId);

    delete pProbe;
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(static_cast<PblHandled*>(pProbe)));
#endif
    CHECK(!(HandleProbe*)h);
    CHECK(h.GetHandleId() == 0);

    HandleProbe* pReuse = new HandleProbe;
    CHECK(pReuse == pProbe);
    CHECK(pReuse->GetHandleId() != uiId);
    CHECK(!(HandleProbe*)hStale);
    CHECK(hStale.GetHandleId() == 0);

    delete pReuse;
    HandleProbe::spPool = 0;
}

static int CountText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    for (size_t uPos = sText.find(pcNeedle); uPos != std::string::npos; uPos = sText.find(pcNeedle, uPos + 1))
    {
        ++iCount;
    }
    return iCount;
}

TEST_CASE("asan fix helpers stay wired into the game (source pin: callers need the game runtime)" * doctest::test_suite("regression"))
{
    std::string sVoice = ReadRepoTextLf("src\\SoundFL\\source\\sndvoicevirtual.cpp");
    CHECK(sVoice.find("void VoiceVirtual::SetVoice(VoiceBase* pVoice)\n"
                      "{\n"
                      "    mFlags |= flags_updateVoice;\n"
                      "    if (mVoice)\n"
                      "    {\n"
                      "        mVoice->SourceTransport::Stop();\n"
                      "        ReleaseHardwareVoice(mVoice);\n"
                      "    }\n") != std::string::npos);

    std::string sSound = ReadRepoTextLf("src\\SoundFL\\source\\sndsound.cpp");
    CHECK(sSound.find("    const unsigned int i = (unsigned int)(pThis - smVoiceVirtuals);\n"
                      "    KeepReleasedVoicePosVel(pVoice, &smSource3DPositions[i], &smSource3DVelocities[i]);\n"
                      "}\n") != std::string::npos);

    std::string sHeader = ReadRepoTextLf("src\\Battlefront2\\Source\\FLEffect.h");
    CHECK(sHeader.find("            ActivateThread(0, PER_FRAME);\n"
                       "        }\n"
                       "    }\n"
                       "    void DetachFromTarget();\n"
                       "    template <class EFFECT>\n"
                       "    friend void DetachEffectFromTarget(EFFECT& rEffect);\n") != std::string::npos);

    std::string sEffect = ReadRepoTextLf("src\\Battlefront2\\Source\\FLEffect.cpp");
    CHECK(sEffect.find("void FLEffectObject::DetachFromTarget()\n"
                       "{\n"
                       "    DetachEffectFromTarget(*this);\n"
                       "}\n") != std::string::npos);

    std::string sOrdnance = ReadRepoTextLf("src\\Battlefront2\\Source\\Ordnance.cpp");
    CHECK(sOrdnance.find("    DeactivateThread();\n"
                         "    if (mTrailEmitterObj)\n"
                         "    {\n"
                         "        mTrailEmitterObj->DetachFromTarget();\n"
                         "        mTrailEmitterObj->DeactivateEffect();\n") != std::string::npos);

    std::string sItem = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityItem.cpp");
    CHECK(sItem.find("    delete m_pLight;\n"
                     "    if (mTrailEmitterObj)\n"
                     "    {\n"
                     "        mTrailEmitterObj->DetachFromTarget();\n"
                     "        mTrailEmitterObj->DeactivateEffect();\n") != std::string::npos);

    std::string sHover = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityHover.cpp");
    CHECK(sHover.find("    VehicleManager::Remove(this);\n"
                      "    if (mWaterEmitter)\n"
                      "    {\n"
                      "        mWaterEmitter->DetachFromTarget();\n"
                      "        mWaterEmitter->DeactivateEffect();\n") != std::string::npos);

    std::string sWater = ReadRepoTextLf("src\\Battlefront2\\Source\\WaterEffect.cpp");
    CHECK(sWater.find("            m_waterEffects[i]->effect->DetachFromTarget();\n"
                      "            m_waterEffects[i]->effect->DeactivateEffect();\n") !=
          std::string::npos);

    std::string sSky = ReadRepoTextLf("src\\Battlefront2\\Source\\SkyDome.cpp");
    size_t uiDtor = sSky.find("SkyDome::~SkyDome()\n");
    REQUIRE(uiDtor != std::string::npos);
    size_t uiDetach = sSky.find("rDome.mEffectData[i].effect->DetachFromTarget();\n", uiDtor);
    size_t uiCleanup = sSky.find("    Cleanup();\n", uiDtor);
    CHECK(uiDetach != std::string::npos);
    CHECK(uiDetach < uiCleanup);

    std::string sFlare = ReadRepoTextLf("src\\Battlefront2\\Platform\\Pc\\pcSunFlare.cpp");
    CHECK(sFlare.find("void SunFlareEffect::SunFlareList::Cleanup()\n"
                      "{\n"
                      "    PblListDeleteSelfRemoving(*this);\n"
                      "    Clear();\n"
                      "}\n") != std::string::npos);
    CHECK(sFlare.find("SunFlare::~SunFlare()\n"
                      "{\n"
                      "    delete m_sunTex;\n"
                      "    DeactivateEffect();\n"
                      "}\n") != std::string::npos);
    CHECK(sFlare.find("        SunFlareEffect::s_pInstance->_List.Remove(this);\n"
                      "        FLEffectObject::DeactivateEffect();\n") != std::string::npos);

    std::string sWeather = ReadRepoTextLf("src\\Battlefront2\\Source\\WeatherEffect.cpp");
    CHECK(sWeather.find("PrecipitationEffect::~PrecipitationEffect()\n"
                        "{\n"
                        "    PblListRemoveAndDelete(m_RainShadows);\n"
                        "    Cleanup();\n"
                        "}\n") != std::string::npos);

    std::string sSquadron = ReadRepoTextLf("src\\Battlefront2\\Source\\EntitySquadron.cpp");
    CHECK(sSquadron.find("    case 0xe5108b93: /* \"SquadronFormation\" */\n"
                         "        AppendFormation(mFormations, mNumFormations, pszValue);\n"
                         "        return true;\n") != std::string::npos);
    CHECK(sSquadron.find("    const EntityFormation** ppFormations = "
                         "AllocFormationList<const EntityFormation>(stack, uNumFormations);\n") != std::string::npos);
    CHECK(sSquadron.find("mFormations[mNumFormations]") == std::string::npos);
    CHECK(sSquadron.find("stack.Alloc(uNumFormations)") == std::string::npos);

    std::string sWalker = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityWalker.cpp");
    CHECK(sWalker.find("        int iCount = 0;\n"
                       "        sscanf(pszValue, \"%d\", &iCount);\n"
                       "        iCount = ClampLegPairCount(iCount);\n"
                       "        if (mLegPairInfo == sOwnLegPairInfo)\n"
                       "        {\n"
                       "            delete[] mLegPairInfo;\n"
                       "        }\n"
                       "        mLegPairCount = iCount;\n"
                       "        mLegPairInfo = new WalkerLegPairInfo[mLegPairCount];\n"
                       "        sOwnLegPairInfo = mLegPairInfo;\n")
          != std::string::npos);
    CHECK(sWalker.find("static WalkerLegPairInfo* CurLegPair(WalkerLegPairInfo* pInfo, int iCount)\n"
                       "{\n"
                       "    return LegPairAt(pInfo, sCurPair, iCount);\n"
                       "}\n")
          != std::string::npos);
    CHECK(sWalker.find("mLegPairInfo[sCurPair]") == std::string::npos);
    CHECK(CountText(sWalker, "CurLegPair(mLegPairInfo, mLegPairCount);") == 8);
    CHECK(CountText(sWalker, "    sCurPair = -1;\n    sOwnLegPairInfo = 0;\n") == 2);
    CHECK(sWalker.find("    for (int i = 0; i < 3; ++i)\n"
                       "    {\n"
                       "        mLegPairs[i] = 0;\n"
                       "    }\n")
          != std::string::npos);
    CHECK(sWalker.find("mLegPairCount > 0 ? mLegPairs[0]->GetStateLimit() : 0.0f, bWrapped);\n")
          != std::string::npos);
    CHECK(sWalker.find("        if (mLegPairCount > 0)\n"
                       "        {\n"
                       "            mLegPairs[0]->SetFrameState(fLegState);\n"
                       "        }\n")
          != std::string::npos);
    CHECK(sWalker.find("        if (mLegPairCount > 0)\n"
                       "        {\n"
                       "            mLegPairs[0]->ResetTurnState();\n"
                       "        }\n")
          != std::string::npos);
    CHECK(sWalker.find("            float fFrontTurn = LegPairTurnState(mLegPairs, mLegPairCount, 0);\n"
                       "            float fBackTurn = LegPairTurnState(mLegPairs, mLegPairCount, 1);\n")
          != std::string::npos);
    CHECK(CountText(sWalker, "mLegPairs[0]") == 5);
    CHECK(CountText(sWalker, "mLegPairs[1]") == 5);

    std::string sCharacter = ReadRepoTextLf("src\\Battlefront2\\Source\\Character.cpp");
    CHECK(sCharacter.find("    else if (!mUnit)\n"
                          "    {\n"
                          "        if (HeroSlotIs(netHeroLastPlayerID, mTeamNumber, mPlayerId) && mPlayerId != -1)\n")
          != std::string::npos);
    CHECK(sCharacter.find("        else if (NetGame::IsEnabled() && mTeamPtr->GetClassPtr(mClassIndex)->mHeroFlag &&\n"
                          "                 !HeroSlotIs(netHeroPlayerID, mTeamNumber, mPlayerId))\n")
          != std::string::npos);
    CHECK(sCharacter.find("            if (HeroSlotIs(netHeroLastPlayerID, mTeamNumber, mPlayerId))\n"
                          "            {\n"
                          "                NetGame::ClearLastHero(mTeamNumber);\n")
          != std::string::npos);
    CHECK(sCharacter.find("            int iHeroId = HeroSlotOr(netHeroAIID, mTeamNumber, -1);\n") != std::string::npos);
    CHECK(sCharacter.find("    if (netHeroesEnabled && HeroSlotSet(netHeroUnlocked, iTeam) &&\n") != std::string::npos);
    CHECK(sCharacter.find("            if (rOwner.mCharacter && rOwner.mCharacter != this)\n"
                          "            {\n"
                          "                SetHeroSlot(netHeroKillerID, rOwner.mCharacter->mTeamNumber, "
                          "rOwner.mCharacter->mPlayerId);\n"
                          "            }\n")
          != std::string::npos);
    CHECK(sCharacter.find("netHeroLastPlayerID[mTeamNumber]") == std::string::npos);
    CHECK(sCharacter.find("netHeroKillerID[rOwner") == std::string::npos);
    CHECK(sCharacter.find("netHeroAIID[mTeamNumber]") == std::string::npos);
    CHECK(sCharacter.find("netHeroPlayerID[mTeamNumber]") == std::string::npos);

    std::string sSpawn = ReadRepoTextLf("src\\Battlefront2\\Platform\\Pc\\pcSpawnDisplay.cpp");
    CHECK(sSpawn.find("                return HeroSlotSet(netHeroUnlocked, iTeam) &&\n"
                      "                       netHeroPlayerID[iTeam] > -1 && netHeroPlayerID[iTeam] == iLocal;\n")
          != std::string::npos);

    std::string sOptions = ReadRepoTextLf("src\\Battlefront2\\Source\\LuaCallbacks_Options.cpp");
    CHECK(sOptions.find("    if (HasRulesSlot(gNumBots, eRules))\n"
                        "    {\n"
                        "        netNumBots = gNumBots[eRules];\n")
          != std::string::npos);
    CHECK(CountText(sOptions, "[eRules]") == 8);
}

struct SndProbeVoice
{
    SndProbeVoice() : mPos3D(0), mVel3D(0), mVoice(0), mLocked(true) {}

    void Lock(bool bLock) { mLocked = bLock; }
    void SetPosition(PblVector3* pPos) { mPos3D = pPos; }
    void SetVelocity(PblVector3* pVel) { mVel3D = pVel; }
    void SetPosVel3D(PblVector3* pPos, PblVector3* pVel)
    {
        SetPosition(pPos);
        SetVelocity(pVel);
    }

    PblVector3* mPos3D;
    PblVector3* mVel3D;
    SndProbeVoice* mVoice;
    bool mLocked;
};

struct SndProbeOwner
{
    PblVector3 mPos;
    PblVector3 mVel;
};

static SndProbeOwner* MakeSndProbeOwner(unsigned char* pBlock)
{
    SndProbeOwner* pOwner = new (pBlock) SndProbeOwner;
    pOwner->mPos = PblVector3(1.0f, 2.0f, 3.0f);
    pOwner->mVel = PblVector3(4.0f, 5.0f, 6.0f);
    return pOwner;
}

static void FreeSndProbeOwner(unsigned char* pBlock)
{
    memset(pBlock, 0xCD, sizeof(SndProbeOwner));
}

static bool SameVector(const PblVector3& v, float fX, float fY, float fZ)
{
    return v.x == fX && v.y == fY && v.z == fZ;
}

TEST_CASE("sound listener keeps its own position after the owner is released" * doctest::test_suite("regression"))
{
    alignas(SndProbeOwner) unsigned char acOwner[sizeof(SndProbeOwner)];
    SndProbeOwner* pOwner = MakeSndProbeOwner(acOwner);

    SndProbeVoice hardware;
    hardware.SetPosVel3D(&pOwner->mPos, &pOwner->mVel);

    Snd::ReleaseHardwareVoice(&hardware);
    FreeSndProbeOwner(acOwner);

    CHECK(!hardware.mLocked);
    CHECK(hardware.mPos3D == &Snd::Globals::smNullVector);
    CHECK(hardware.mVel3D == &Snd::Globals::smNullVector);
    CHECK(SameVector(*hardware.mPos3D, 0.0f, 0.0f, 0.0f));
    CHECK(SameVector(*hardware.mVel3D, 0.0f, 0.0f, 0.0f));
}

TEST_CASE("released 3d voice reports its stored velocity" * doctest::test_suite("regression"))
{
    alignas(SndProbeOwner) unsigned char acOwner[sizeof(SndProbeOwner)];
    SndProbeOwner* pOwner = MakeSndProbeOwner(acOwner);

    SndProbeVoice hardware;
    SndProbeVoice voice;
    voice.mVoice = &hardware;
    voice.SetPosVel3D(&pOwner->mPos, &pOwner->mVel);
    hardware.SetPosVel3D(&pOwner->mPos, &pOwner->mVel);

    PblVector3 vStoredPos(0.0f, 0.0f, 0.0f);
    PblVector3 vStoredVel(0.0f, 0.0f, 0.0f);
    Snd::KeepReleasedVoicePosVel(&voice, &vStoredPos, &vStoredVel);
    FreeSndProbeOwner(acOwner);

    CHECK(voice.mPos3D == &vStoredPos);
    CHECK(voice.mVel3D == &vStoredVel);
    CHECK(hardware.mPos3D == &vStoredPos);
    CHECK(hardware.mVel3D == &vStoredVel);
    CHECK(SameVector(*voice.mPos3D, 1.0f, 2.0f, 3.0f));
    CHECK(SameVector(*voice.mVel3D, 4.0f, 5.0f, 6.0f));
    CHECK(SameVector(*hardware.mVel3D, 4.0f, 5.0f, 6.0f));

    alignas(SndProbeOwner) unsigned char acLone[sizeof(SndProbeOwner)];
    SndProbeOwner* pLone = MakeSndProbeOwner(acLone);
    SndProbeVoice lone;
    lone.SetPosVel3D(&pLone->mPos, &pLone->mVel);
    PblVector3 vLonePos(0.0f, 0.0f, 0.0f);
    PblVector3 vLoneVel(0.0f, 0.0f, 0.0f);
    Snd::KeepReleasedVoicePosVel(&lone, &vLonePos, &vLoneVel);
    FreeSndProbeOwner(acLone);

    CHECK(lone.mVel3D == &vLoneVel);
    CHECK(SameVector(*lone.mVel3D, 4.0f, 5.0f, 6.0f));
}

template <class PROBE>
struct ListProbeSlots
{
    static const int kSlots = 4;

    static void* Take()
    {
        REQUIRE(siUsed < kSlots);
        return &saSlots[siUsed++];
    }

    static void Reset()
    {
        siUsed = 0;
        siDeleted = 0;
        siDeletedWhileLinked = 0;
        memset(saSlots, 0, sizeof(saSlots));
    }

    static void Poison(void* pBlock)
    {
        static_cast<PROBE*>(pBlock)->mNode._pNext = &spList->_head;
        ++siDeleted;
    }

    struct alignas(PROBE) Slot
    {
        unsigned char acBytes[sizeof(PROBE)];
    };

    static Slot saSlots[kSlots];
    static int siUsed;
    static int siDeleted;
    static int siDeletedWhileLinked;
    static PblList<PROBE>* spList;
};

template <class PROBE>
typename ListProbeSlots<PROBE>::Slot ListProbeSlots<PROBE>::saSlots[ListProbeSlots<PROBE>::kSlots];
template <class PROBE>
int ListProbeSlots<PROBE>::siUsed = 0;
template <class PROBE>
int ListProbeSlots<PROBE>::siDeleted = 0;
template <class PROBE>
int ListProbeSlots<PROBE>::siDeletedWhileLinked = 0;
template <class PROBE>
PblList<PROBE>* ListProbeSlots<PROBE>::spList = 0;

struct SelfRemovingProbe
{
    typedef ListProbeSlots<SelfRemovingProbe> Slots;

    ~SelfRemovingProbe()
    {
        if (mNode.IsInList())
        {
            ++Slots::siDeletedWhileLinked;
            Slots::spList->Remove(&mNode);
        }
    }

    static void* operator new(size_t) { return Slots::Take(); }
    static void operator delete(void* pBlock) { Slots::Poison(pBlock); }

    int miValue;
    PblList<SelfRemovingProbe>::Node mNode;
};

struct ListedProbe
{
    typedef ListProbeSlots<ListedProbe> Slots;

    ~ListedProbe()
    {
        if (mNode.IsInList())
        {
            ++Slots::siDeletedWhileLinked;
        }
    }

    static void* operator new(size_t) { return Slots::Take(); }
    static void operator delete(void* pBlock) { Slots::Poison(pBlock); }

    int miValue;
    PblList<ListedProbe>::Node mNode;
};

TEST_CASE("pbl list delete of self removing objects never steps through a freed node" * doctest::test_suite("regression"))
{
    typedef ListProbeSlots<SelfRemovingProbe> Slots;
    Slots::Reset();
    PblList<SelfRemovingProbe> list;
    Slots::spList = &list;
    for (int i = 0; i < 3; ++i)
    {
        SelfRemovingProbe* pProbe = new SelfRemovingProbe;
        pProbe->miValue = i;
        list.Add(&pProbe->mNode, pProbe);
    }

    PblListDeleteSelfRemoving(list);

    CHECK(Slots::siDeleted == 3);
    CHECK(Slots::siDeletedWhileLinked == 3);
    CHECK(list.GetCount() == 0);
    CHECK(list.GetFirst() == &list._head);
    CHECK(list._head._pPrev == &list._head);
    Slots::spList = 0;
}

TEST_CASE("pbl list delete unlinks each object before it is freed" * doctest::test_suite("regression"))
{
    typedef ListProbeSlots<ListedProbe> Slots;
    Slots::Reset();
    PblList<ListedProbe> list;
    Slots::spList = &list;
    for (int i = 0; i < 3; ++i)
    {
        ListedProbe* pProbe = new ListedProbe;
        pProbe->miValue = i;
        list.Add(&pProbe->mNode, pProbe);
    }

    PblListRemoveAndDelete(list);

    CHECK(Slots::siDeleted == 3);
    CHECK(Slots::siDeletedWhileLinked == 0);
    CHECK(list.GetCount() == 0);
    CHECK(list.GetFirst() == &list._head);
    CHECK(list._head._pPrev == &list._head);
    Slots::spList = 0;
}

class PblMatrix;

struct EffectProbeHandle
{
    const void* mObject;
};

struct EffectProbe
{
    enum
    {
        ATTACHED_DYNAMIC = 1,
        ATTACHED_TO_GAMEOBJECT = 2,
        ATTACHED_TO_VECTOR = 4,
    };

    EffectProbe() : m_uiAttachFlags(0), m_pmAttachedToMatrix(0), mvWorld(0.0f, 0.0f, 0.0f)
    {
        m_hobjAttachedToObject.mObject = 0;
    }

    void AttachToVector(PblVector3* pVector, bool bDynamic)
    {
        m_hobjAttachedToObject.mObject = 0;
        m_pvAttachedToVector = pVector;
        m_uiAttachFlags = ATTACHED_TO_VECTOR | (bDynamic ? ATTACHED_DYNAMIC : 0);
        _UpdateWorldMatrix();
    }

    void _UpdateWorldMatrix()
    {
        if ((m_uiAttachFlags & ATTACHED_TO_VECTOR) && m_pvAttachedToVector)
        {
            mvWorld = *m_pvAttachedToVector;
        }
    }

    void Update()
    {
        if (m_uiAttachFlags & ATTACHED_DYNAMIC)
        {
            _UpdateWorldMatrix();
        }
    }

    EffectProbeHandle m_hobjAttachedToObject;
    unsigned int m_uiAttachFlags;
    union
    {
        PblMatrix* m_pmAttachedToMatrix;
        PblVector3* m_pvAttachedToVector;
    };
    PblVector3 mvWorld;
};

TEST_CASE("effect keeps its last position after its attach vector is released" * doctest::test_suite("regression"))
{
    alignas(SndProbeOwner) unsigned char acOwner[sizeof(SndProbeOwner)];
    SndProbeOwner* pOwner = MakeSndProbeOwner(acOwner);

    EffectProbe effect;
    effect.AttachToVector(&pOwner->mPos, true);
    pOwner->mPos = PblVector3(7.0f, 8.0f, 9.0f);

    DetachEffectFromTarget(effect);
    FreeSndProbeOwner(acOwner);
    effect.Update();

    CHECK(SameVector(effect.mvWorld, 7.0f, 8.0f, 9.0f));
    CHECK((effect.m_uiAttachFlags & (EffectProbe::ATTACHED_DYNAMIC | EffectProbe::ATTACHED_TO_VECTOR)) == 0);
    CHECK(!effect.m_pvAttachedToVector);

    PblVector3 vFixed(1.0f, 1.0f, 1.0f);
    EffectProbe still;
    still.AttachToVector(&vFixed, false);
    DetachEffectFromTarget(still);
    CHECK(still.m_uiAttachFlags == EffectProbe::ATTACHED_TO_VECTOR);
    CHECK(still.m_pvAttachedToVector == &vFixed);

    int iObject = 0;
    EffectProbe onObject;
    onObject.AttachToVector(&vFixed, true);
    onObject.m_hobjAttachedToObject.mObject = &iObject;
    DetachEffectFromTarget(onObject);
    CHECK(onObject.m_uiAttachFlags == (EffectProbe::ATTACHED_TO_VECTOR | EffectProbe::ATTACHED_DYNAMIC));
    CHECK(onObject.m_pvAttachedToVector == &vFixed);
}

TEST_CASE("mod data fixed array appends stop at the array size" * doctest::test_suite("regression"))
{
    std::string sDroideka = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityDroideka.cpp");
    CHECK(sDroideka.find("if (iMask >= 0 && mNumFeet < 3)\n"
                         "                {\n"
                         "                    mFootPrimitives[mNumFeet++] = iMask;\n")
          != std::string::npos);

    std::string sHover = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityHover.cpp");
    CHECK(sHover.find("if (iMaskId >= 0 && mNumWakeEmitters < 6)\n"
                      "        {\n"
                      "            mWakeEmitterPrimitives[mNumWakeEmitters++] = iMaskId;\n")
          != std::string::npos);

    std::string sWalker = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityWalker.cpp");
    CHECK(sWalker.find("if (rPair.mFootCollisionBody[0] >= 0 && mNumFeet < 6)\n") != std::string::npos);
    CHECK(sWalker.find("if (rPair.mFootCollisionBody[1] >= 0 && mNumFeet < 6)\n") != std::string::npos);
    CHECK(sWalker.find("    while (mAnimObj[mNumAnimObjs])\n"
                       "    {\n"
                       "        if (++mNumAnimObjs >= 3)\n"
                       "        {\n"
                       "            break;\n"
                       "        }\n"
                       "        char szName[128];\n"
                       "        _snprintf(szName, 127, \"%s%d\", pszName, mNumAnimObjs);\n")
          != std::string::npos);

    std::string sProp = ReadRepoTextLf("src\\Battlefront2\\Source\\PropLayer.cpp");
    CHECK(sProp.find("            if (mNumModels < 32)\n"
                     "            {\n"
                     "                if (pModel->mMaxDist > mMaxDistance)\n"
                     "                {\n"
                     "                    mMaxDistance = pModel->mMaxDist;\n"
                     "                }\n"
                     "                mModels[mNumModels++] = pModel;\n"
                     "            }\n"
                     "            else\n"
                     "            {\n"
                     "                delete pModel;\n"
                     "            }\n")
          != std::string::npos);

    std::string sSky = ReadRepoTextLf("src\\Battlefront2\\Source\\SkyObject.cpp");
    CHECK(sSky.find("    if (mNumObjectClasses < 10)\n"
                    "    {\n"
                    "        mObjectClasses[mNumObjectClasses++] = new SkyObjectClass(config);\n"
                    "    }\n")
          != std::string::npos);
}

TEST_CASE("squadron formation list does not run into the next stack block" * doctest::test_suite("regression"))
{
    RedStack stack;
    static const int siFormation = 7;
    const int** ppFormations = AllocFormationList<const int>(stack, 10);
    unsigned char* pucNext = (unsigned char*)stack.Alloc(16);
    memset(pucNext, 0xAB, 16);
    for (int i = 0; i < 10; ++i)
    {
        ppFormations[i] = &siFormation;
    }
    for (int i = 0; i < 16; ++i)
    {
        INFO(i);
        CHECK(pucNext[i] == 0xAB);
    }
    for (int i = 0; i < 10; ++i)
    {
        CHECK(ppFormations[i] == &siFormation);
    }
}

struct FormationProbe
{
    unsigned int auFormations[10];
    unsigned int uNumFormations;
    unsigned int uAfter;
};

TEST_CASE("squadron formation odf list stops at its array size" * doctest::test_suite("regression"))
{
    static const char* const apcNames[11] = { "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10" };
    FormationProbe probe;
    memset(&probe, 0, sizeof(probe));
    probe.uAfter = 0x5EA15EA1u;
    for (int i = 0; i < 11; ++i)
    {
        AppendFormation(probe.auFormations, probe.uNumFormations, apcNames[i]);
    }
    CHECK(probe.uNumFormations == 10);
    CHECK(probe.uAfter == 0x5EA15EA1u);
    for (int i = 0; i < 10; ++i)
    {
        INFO(i);
        CHECK(probe.auFormations[i] == (unsigned int)PblHash(apcNames[i]));
    }
}

TEST_CASE("walker leg pair count from mod data stays inside the leg pair array" * doctest::test_suite("regression"))
{
    CHECK(ClampLegPairCount(INT_MIN) == 0);
    CHECK(ClampLegPairCount(-1) == 0);
    CHECK(ClampLegPairCount(0) == 0);
    CHECK(ClampLegPairCount(2) == 2);
    CHECK(ClampLegPairCount(3) == 3);
    CHECK(ClampLegPairCount(4) == 3);
    CHECK(ClampLegPairCount(INT_MAX) == 3);
}

struct LegPairInfoProbe
{
    int miJoint;
};

TEST_CASE("walker leg pair properties past the leg pair count are ignored" * doctest::test_suite("regression"))
{
    LegPairInfoProbe akInfo[2] = { { 1 }, { 2 } };
    CHECK(LegPairAt(akInfo, 0, 2) == &akInfo[0]);
    CHECK(LegPairAt(akInfo, 1, 2) == &akInfo[1]);
    CHECK_FALSE(LegPairAt(akInfo, 2, 2));
    CHECK_FALSE(LegPairAt(akInfo, -1, 2));
    CHECK_FALSE(LegPairAt(akInfo, 0, 0));
    CHECK_FALSE(LegPairAt((LegPairInfoProbe*)0, 0, 2));
}

struct LegPairProbe
{
    LegPairProbe() : miReads(0) {}

    float GetTurnState()
    {
        ++miReads;
        return 5.0f;
    }

    int miReads;
};

TEST_CASE("walker without leg pairs never reads a leg pair turn state" * doctest::test_suite("regression"))
{
    LegPairProbe front;
    LegPairProbe back;
    LegPairProbe* apPairs[3] = { &front, &back, 0 };

    CHECK(LegPairTurnState(apPairs, 0, 0) == 0.0f);
    CHECK(LegPairTurnState(apPairs, 0, 1) == 0.0f);
    CHECK(LegPairTurnState(apPairs, 1, 1) == 0.0f);
    CHECK(front.miReads == 0);
    CHECK(back.miReads == 0);

    CHECK(LegPairTurnState(apPairs, 2, 0) == 5.0f);
    CHECK(LegPairTurnState(apPairs, 2, 1) == 5.0f);
    CHECK(front.miReads == 1);
    CHECK(back.miReads == 1);
}

static char sacReusedArena[0x1000];

TEST_CASE("heap re-created after destroy is writable" * doctest::test_suite("regression"))
{
    char* pcEnd = sacReusedArena + sizeof(sacReusedArena);
    int iHeap = RedCreateHeap(sacReusedArena, pcEnd, 0x10, "ArenaReuse");
    REQUIRE(iHeap >= 0);
    void* pFirst = RedAllocFromHeap(iHeap, 0x100, 0);
    REQUIRE(pFirst);
    memset(pFirst, 0x5A, 0x100);
    RedDestroyHeap(iHeap);

    iHeap = RedCreateHeap(sacReusedArena, pcEnd, 0x10, "ArenaReuse");
    REQUIRE(iHeap >= 0);
    CHECK(RedGetHeapFree(iHeap) > 0x200);
    unsigned char* pucBlock = (unsigned char*)RedAllocFromHeap(iHeap, 0x200, 0);
    REQUIRE(pucBlock);
    CHECK(pucBlock[0] == 0);
    CHECK(pucBlock[0x1FF] == 0);
    memset(pucBlock, 0xA5, 0x200);
    CHECK(pucBlock[0x1FF] == 0xA5);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(!__asan_address_is_poisoned(pucBlock + 0x1FF));
#endif
    RedFreeToHeap(iHeap, pucBlock, 0);
    RedDestroyHeap(iHeap);
}
