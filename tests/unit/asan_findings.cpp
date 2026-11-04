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

TEST_CASE("hero and game rules reads stay bounded" * doctest::test_suite("regression"))
{
    struct GuardSite
    {
        const char* pcPath;
        const char* pcGuard;
    };
    const GuardSite akSites[8] = {
        { "src\\Battlefront2\\Source\\Character.cpp", "\n        if (PblInBounds(netHeroLastPlayerID, mTeamNumber) &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "            if (PblInBounds(netHeroLastPlayerID, mTeamNumber) &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroUnlocked, iTeam) && netHeroUnlocked[iTeam] &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroKillerID, rOwner.mCharacter->mTeamNumber)" },
        { "src\\Battlefront2\\Source\\Character.cpp", "!PblInBounds(netHeroPlayerID, mTeamNumber) ||" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroAIID, mTeamNumber) ? netHeroAIID[mTeamNumber] : -1" },
        { "src\\Battlefront2\\Platform\\Pc\\pcSpawnDisplay.cpp", "PblInBounds(netHeroUnlocked, iTeam) && netHeroUnlocked[iTeam] &&" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Options.cpp", "if (PblInBounds(gNumBots, eRules))" },
    };
    for (int i = 0; i < 8; ++i)
    {
        std::string sText;
        REQUIRE(ReadRepoText(akSites[i].pcPath, sText));
        INFO(akSites[i].pcPath);
        INFO(akSites[i].pcGuard);
        CHECK(sText.find(akSites[i].pcGuard) != std::string::npos);
    }
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

TEST_CASE("effect stops reading a released attach vector" * doctest::test_suite("regression"))
{
    std::string sHeader = ReadRepoTextLf("src\\Battlefront2\\Source\\FLEffect.h");
    CHECK(sHeader.find("    virtual void StopAndFinish();\n    void DetachFromTarget();\n") !=
          std::string::npos);

    std::string sEffect = ReadRepoTextLf("src\\Battlefront2\\Source\\FLEffect.cpp");
    CHECK(sEffect.find("void FLEffectObject::DetachFromTarget()\n"
                       "{\n"
                       "    if (m_hobjAttachedToObject.mObject || !(m_uiAttachFlags & ATTACHED_DYNAMIC))\n"
                       "    {\n"
                       "        return;\n"
                       "    }\n"
                       "    _UpdateWorldMatrix();\n"
                       "    m_uiAttachFlags &= ~(ATTACHED_DYNAMIC | ATTACHED_TO_VECTOR);\n"
                       "    m_pmAttachedToMatrix = 0;\n"
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
    CHECK(__asan_address_is_poisoned(&pProbe->mHandleId));
#endif
    CHECK(pProbe->PblHandled::GetHandleId() == 0);
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

TEST_CASE("sound listener stops reading a released position" * doctest::test_suite("regression"))
{
    std::string sVoice = ReadRepoTextLf("src\\SoundFL\\source\\sndvoicevirtual.cpp");
    CHECK(sVoice.find("void VoiceVirtual::SetVoice(VoiceBase* pVoice)\n"
                      "{\n"
                      "    mFlags |= flags_updateVoice;\n"
                      "    if (mVoice)\n"
                      "    {\n"
                      "        mVoice->SourceTransport::Stop();\n"
                      "        mVoice->Lock(false);\n"
                      "        mVoice->SetPosVel3D(&Globals::smNullVector, &Globals::smNullVector);\n"
                      "    }\n") != std::string::npos);
}

TEST_CASE("released 3d voice keeps its own velocity" * doctest::test_suite("regression"))
{
    std::string sSound = ReadRepoTextLf("src\\SoundFL\\source\\sndsound.cpp");
    CHECK(sSound.find("void __cdecl Sound::VoiceVirtualRelease(VoiceVirtual* pVoice)\n"
                      "{\n"
                      "    FireForget* pThis = static_cast<FireForget*>(pVoice);\n"
                      "    pThis->SetStolenCallback(0, 0);\n"
                      "    pVoice->SetInactiveCallback(&FireForget::InactiveCallback, pVoice);\n"
                      "    const unsigned int i = (unsigned int)(pThis - smVoiceVirtuals);\n"
                      "    smSource3DPositions[i] = *pVoice->mPos3D;\n"
                      "    smSource3DVelocities[i] = *pVoice->mVel3D;\n"
                      "    pVoice->SetPosition(&smSource3DPositions[i]);\n"
                      "    pVoice->SetVelocity(&smSource3DVelocities[i]);\n"
                      "    if (pVoice->mVoice)\n"
                      "    {\n"
                      "        pVoice->mVoice->SetPosVel3D(&smSource3DPositions[i], &smSource3DVelocities[i]);\n"
                      "    }\n"
                      "}\n") != std::string::npos);
}

TEST_CASE("sun flare cleanup does not step through a deleted node" * doctest::test_suite("regression"))
{
    std::string sFlare = ReadRepoTextLf("src\\Battlefront2\\Platform\\Pc\\pcSunFlare.cpp");
    CHECK(sFlare.find("void SunFlareEffect::SunFlareList::Cleanup()\n"
                      "{\n"
                      "    while (GetCount() > 0)\n"
                      "    {\n"
                      "        delete GetFirst()->_pObject;\n"
                      "    }\n"
                      "    RemoveAll();\n"
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
                        "    m_RainShadows.Clear();\n"
                        "    Cleanup();\n"
                        "}\n") != std::string::npos);
}

TEST_CASE("squadron formation list fits its stack block" * doctest::test_suite("regression"))
{
    std::string sSquadron = ReadRepoTextLf("src\\Battlefront2\\Source\\EntitySquadron.cpp");
    CHECK(sSquadron.find("    const EntityFormation** ppFormations = "
                         "(const EntityFormation**)stack.Alloc((int)(uNumFormations * sizeof(*ppFormations)));\n")
          != std::string::npos);
    CHECK(sSquadron.find("stack.Alloc(uNumFormations)") == std::string::npos);
}

TEST_CASE("squadron formation list stops at ten" * doctest::test_suite("regression"))
{
    std::string sSquadron = ReadRepoTextLf("src\\Battlefront2\\Source\\EntitySquadron.cpp");
    CHECK(sSquadron.find("    case 0xe5108b93:\n"
                         "        if (mNumFormations < 10)\n"
                         "        {\n"
                         "            mFormations[mNumFormations] = PblHash(pszValue);\n"
                         "            mNumFormations++;\n"
                         "        }\n"
                         "        return true;\n")
          != std::string::npos);
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

static int CountText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    for (size_t uPos = sText.find(pcNeedle); uPos != std::string::npos; uPos = sText.find(pcNeedle, uPos + 1))
    {
        ++iCount;
    }
    return iCount;
}

TEST_CASE("walker leg pair properties stay inside the leg pair array" * doctest::test_suite("regression"))
{
    std::string sWalker = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityWalker.cpp");
    CHECK(sWalker.find("        int iCount = 0;\n"
                       "        sscanf(pszValue, \"%d\", &iCount);\n"
                       "        if (iCount < 0)\n"
                       "        {\n"
                       "            iCount = 0;\n"
                       "        }\n"
                       "        else if (iCount > 3)\n"
                       "        {\n"
                       "            iCount = 3;\n"
                       "        }\n"
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
                       "    if (pInfo && sCurPair >= 0 && sCurPair < iCount)\n"
                       "    {\n"
                       "        return &pInfo[sCurPair];\n"
                       "    }\n"
                       "    return 0;\n"
                       "}\n")
          != std::string::npos);
    CHECK(sWalker.find("mLegPairInfo[sCurPair]") == std::string::npos);
    CHECK(CountText(sWalker, "CurLegPair(mLegPairInfo, mLegPairCount);") == 8);
    CHECK(CountText(sWalker, "    sCurPair = -1;\n    sOwnLegPairInfo = 0;\n") == 2);
}

TEST_CASE("walker without leg pairs never reads a leg pair" * doctest::test_suite("regression"))
{
    std::string sWalker = ReadRepoTextLf("src\\Battlefront2\\Source\\EntityWalker.cpp");
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
    CHECK(sWalker.find("float fFrontTurn = mLegPairCount > 0 ? mLegPairs[0]->GetTurnState() : 0.0f;\n") != std::string::npos);
    CHECK(CountText(sWalker, "mLegPairs[0]") == 6);
    CHECK(CountText(sWalker, "mLegPairs[1]") == 6);
}

static char sacReusedArena[0x1000];

TEST_CASE("heap re-created over a reused arena is writable" * doctest::test_suite("regression"))
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
