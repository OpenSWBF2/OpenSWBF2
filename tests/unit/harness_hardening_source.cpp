#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "gate/gate_machine.h"
#include "hardening/hardening_allow.h"
#include "hardening/hardening_regex.h"
#include "hardening/hardening_source.h"

namespace
{

const char* const kFixtureRoot = "C:\\fixture";

std::string Content(const std::vector<std::string>& kLines)
{
    std::string sOut;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        sOut += kLines[i] + "\r\n";
    }
    return sOut;
}

Hardening::TextFile File(const char* pcRel, const std::vector<std::string>& kLines)
{
    Hardening::TextFile kFile;
    kFile.sRel = pcRel;
    kFile.sText = Content(kLines);
    return kFile;
}

std::vector<Hardening::TextFile> Scoped(const std::vector<Hardening::TextFile>& kAll)
{
    std::vector<Hardening::TextFile> kOut;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        bool bIn = false;
        std::string sError;
        REQUIRE_MESSAGE(Hardening::InSourceScope(kFixtureRoot, kAll[i].sRel, bIn, sError), sError);
        if (bIn)
        {
            kOut.push_back(kAll[i]);
        }
    }
    return kOut;
}

std::string Joined(const std::vector<std::string>& kHits)
{
    std::string sOut;
    for (size_t i = 0; i < kHits.size(); ++i)
    {
        sOut += (i ? ", " : "") + kHits[i];
    }
    return sOut;
}

std::string LineHits(const char* pcRel, const std::vector<int>& kLines)
{
    std::vector<std::string> kHits;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        kHits.push_back(std::string(pcRel) + ":" + std::to_string(kLines[i]));
    }
    return Joined(kHits);
}

std::string RegexRow(const char* pcRow, const std::vector<Hardening::TextFile>& kAll,
                     const std::vector<std::string>& kAllow = std::vector<std::string>())
{
    std::vector<std::string> kHits;
    std::string sError;
    const bool bRan = Hardening::ScanRegexRow(pcRow, kFixtureRoot, Scoped(kAll), kAllow, kHits, sError);
    REQUIRE_MESSAGE(bRan, sError);
    return Joined(kHits);
}

std::string Spacer(const std::vector<Hardening::TextFile>& kAll, const std::vector<std::string>& kAllow)
{
    std::vector<std::string> kHits;
    std::string sError;
    const bool bRan = Hardening::ScanCharSpacer(Scoped(kAll), kAllow, kHits, sError);
    REQUIRE_MESSAGE(bRan, sError);
    return Joined(kHits);
}

std::string Offset(const std::vector<Hardening::TextFile>& kAll, const std::vector<std::string>& kAllow)
{
    std::vector<std::string> kHits;
    std::string sError;
    const bool bRan = Hardening::ScanOffsetHack(Scoped(kAll), kAllow, kHits, sError);
    REQUIRE_MESSAGE(bRan, sError);
    return Joined(kHits);
}

Hardening::SourceInput Input(const Hardening::MemoryTree& kTree)
{
    Hardening::SourceInput kIn;
    kIn.sRoot = kFixtureRoot;
    kIn.pTree = &kTree;
    return kIn;
}

void SetAllow(Hardening::SourceInput& kIn, const char* pcSection, const std::vector<std::string>& kEntries)
{
    for (size_t i = 0; i < kIn.kAllow.kSections.size(); ++i)
    {
        if (kIn.kAllow.kSections[i].sName == pcSection)
        {
            kIn.kAllow.kSections[i].kEntries = kEntries;
            return;
        }
    }
    Hardening::AllowSection kSection;
    kSection.sName = pcSection;
    kSection.kEntries = kEntries;
    kIn.kAllow.kSections.push_back(kSection);
}

bool RowPasses(const char* pcRow, const Hardening::SourceInput& kIn, std::string& sDetail)
{
    std::vector<std::string> kHits;
    std::string sError;
    const bool bRan = Hardening::ScanSourceRow(pcRow, kIn, kHits, sError);
    REQUIRE_MESSAGE(bRan, pcRow << ": " << sError);
    sDetail = Joined(kHits);
    return kHits.empty();
}

std::string RowDetail(const char* pcRow, const Hardening::SourceInput& kIn)
{
    std::string sDetail;
    RowPasses(pcRow, kIn, sDetail);
    return sDetail;
}

std::string RepoText(const char* pcRel)
{
    std::string sText;
    REQUIRE_MESSAGE(ReadRepoBytes(pcRel, sText), pcRel);
    return sText;
}

std::string Replaced(const std::string& sText, const std::string& sFrom, const std::string& sTo)
{
    std::string sOut = sText;
    for (size_t uAt = sOut.find(sFrom); uAt != std::string::npos && !sFrom.empty(); uAt = sOut.find(sFrom, uAt + sTo.size()))
    {
        sOut.replace(uAt, sFrom.size(), sTo);
    }
    return sOut;
}

std::string Formatted(const std::string& sTemplate, const std::vector<std::string>& kArgs)
{
    std::string sOut = sTemplate;
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        sOut = Replaced(sOut, "{" + std::to_string(i) + "}", kArgs[i]);
    }
    return sOut;
}

struct PinCase
{
    const char* pcName;
    bool bPass;
    std::string sPins;
    std::string sProject;
    const char* pcDetail;
};

void CheckPinCases(const char* pcRow, const std::string& sPins, const std::string& sProject,
                   const std::vector<PinCase>& kCases)
{
    for (size_t i = 0; i < kCases.size(); ++i)
    {
        const PinCase& k = kCases[i];
        CAPTURE(k.pcName);
        CHECK_FALSE((!k.bPass && k.sPins == sPins && k.sProject == sProject));
        Hardening::MemoryTree kTree;
        kTree.Add("lib\\vendor-pins.props", k.sPins);
        kTree.Add("BattlefrontII.vcxproj", k.sProject);
        std::string sDetail;
        CHECK(RowPasses(pcRow, Input(kTree), sDetail) == k.bPass);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(sDetail.find(k.pcDetail) != std::string::npos, sDetail);
        }
        if (k.bPass)
        {
            CHECK(sDetail == "");
        }
    }
}

}

TEST_CASE_NAMED(FHardeningSourceTest1, "Unit::Hardening::allow file", "[EngineFilter]")
{
    Hardening::AllowFile kFile;
    std::string sError;
    REQUIRE(Hardening::ParseAllowText("[SpacerAllow]\r\nb|x\r\na |  y \r\n[OffsetAllow]\r\nc\r\n", kFile, sError));
    REQUIRE(kFile.kSections.size() == 2);
    CHECK(kFile.kSections[0].sName == "SpacerAllow");
    REQUIRE(kFile.kSections[0].kEntries.size() == 2);
    CHECK(kFile.kSections[0].kEntries[0] == "b|x");
    CHECK(kFile.kSections[0].kEntries[1] == "a |  y ");
    CHECK(kFile.kSections[1].sName == "OffsetAllow");
    CHECK(Hardening::FindAllow(kFile, "OffsetAllow")->size() == 1);
    CHECK(Hardening::FindAllow(kFile, "SerialAllow") == nullptr);

    CHECK_FALSE(Hardening::ParseAllowText("[SpacerAllow]\r\n[Nope]\r\nx\r\n", kFile, sError));
    CHECK(sError.find("unknown section [Nope]") != std::string::npos);
    CHECK_FALSE(Hardening::ParseAllowText("[SpacerAllow]\r\na\r\n\r\nb\r\n", kFile, sError));
    CHECK(sError.find("line 3 is empty") != std::string::npos);
    CHECK_FALSE(Hardening::ParseAllowText("a\r\n[SpacerAllow]\r\n", kFile, sError));
    CHECK_FALSE(Hardening::ParseAllowText("[SpacerAllow]\r\n[SpacerAllow]\r\n", kFile, sError));

    REQUIRE_MESSAGE(Hardening::ReadAllowFile(kFile, sError), sError);
    REQUIRE(kFile.kSections.size() == Hardening::kAllowSectionCount);
    for (size_t i = 0; i < Hardening::kAllowSectionCount; ++i)
    {
        CHECK(kFile.kSections[i].sName == Hardening::kAllowSectionNames[i]);
    }
    const char* const apcSites[] = {"SpacerAllow", "OffsetAllow", "SizeLiteralAllow", "SerialAllow",
                                    "AddrOrderAllow", "FastcallAllow", "BinkNameAllow"};
    const size_t auCounts[] = {78, 51, 8, 10, 15, 5, 1};
    size_t uSites = 0;
    for (size_t i = 0; i < sizeof(auCounts) / sizeof(auCounts[0]); ++i)
    {
        CHECK_MESSAGE(Hardening::FindAllow(kFile, apcSites[i])->size() == auCounts[i], apcSites[i]);
        uSites += Hardening::FindAllow(kFile, apcSites[i])->size();
    }
    CHECK(uSites == 168);
}

TEST_CASE_NAMED(FHardeningSourceTest2, "Unit::Hardening::char-spacer", "[EngineFilter]")
{
    const std::vector<Hardening::TextFile> kFiles = {
        File("src\\x.h", {"unsigned char acAllowed[0x10];", "unsigned char acAllowed[0x10];", "char mGap[0x20];",
                          "char mPad[12];", "char acName[64];", "char mRun[16 + 0x4];", "unsigned char m[2][0x10];",
                          "static const unsigned char kTable[0x4] = { 1, 2, 3, 4 };", "mutable char mBuf[0x8];",
                          "alignas(4) unsigned char mAligned[0x8];", "BYTE mByte[0x8];", "uint8_t mU8[0x8];",
                          "signed char mS[0x8];", "unsigned __int8 mI8[0x8];", "char a[4], pad2[0x10];",
                          "char mTail[sizeof(int) - 1];", "// char mComment[0x10];", "char* apNames[0x10];",
                          "struct S { bool b; volatile char mVol[0x2]; };", "char acSized[0x10];"}),
    };
    const std::vector<std::string> kAllow = {"src\\x.h|acAllowed|[0x10]", "src\\x.h|acSized|[16]", "src\\x.h|mMissing|[0x4]"};
    CHECK(Spacer(kFiles, kAllow) == LineHits("src\\x.h", {2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 19, 20}) +
                                        ", unused allow src\\x.h|acSized|[16], unused allow src\\x.h|mMissing|[0x4]");
}

TEST_CASE_NAMED(FHardeningSourceTest3, "Unit::Hardening::offset-hack", "[EngineFilter]")
{
    const std::vector<Hardening::TextFile> kFiles = {
        File("src\\x.cpp",
             {"p = (Foo*)((char*)pObj + 0x10);", "x = *(int*)((char*)this + 0x1c);",
              "q = (const Bar*)((unsigned char*)pObj->mRec - 8);", "a = (const sockaddr*)(pPeer + 1);",
              "r = reinterpret_cast<Baz*>(pRec + 2);", "s = reinterpret_cast<int*>(reinterpret_cast<char*>(pObj) + 4);",
              "Read((char*)mBitmap + 1, n);", "t = (Node*)((char*)pLink - offsetof(Node, mLink));",
              "u = (Node*)((char*)pLink - (int)offsetof(Node, mLink));", "v = (int*)(pMe->mCtrl);",
              "w = (char*)pA - (char*)pB;", "// c = (Foo*)((char*)pObj + 0x10);", "y = (Foo*)((char*)pAllowed + 4);",
              "y = (Foo*)((char*)pAllowed + 4);", "z = (Foo*)( (char*)pSpaced  +  4 );", "k = (float)(fA + fB);",
              "h = (char*)p + iSize;", "n = (IMAGE_NT_HEADERS*)((char*)pBase + pDos->e_lfanew);",
              "m = (IMAGE_NT_HEADERS*)((char*)pBase + 0x3c);",
              "g = ((Damageable*)((char*)pUnit->GetGameObject() + 320))->Vanish();",
              "e = (Node*)(pList->Head(1) + 2);", "c = (char*)pRec->Buf()[0] + 4;", "b = Get(a)->Value() + 2;"}),
    };
    const std::vector<std::string> kAllow = {"src\\x.cpp|(Foo*)((char*)pAllowed + 4)", "src\\x.cpp|(Foo*)( (char*)pSpaced + 4 )",
                                             "src\\x.cpp|(Foo*)((char*)pMissing + 4)", "src\\y.cpp|(Foo*)((char*)pAllowed + 4)"};
    CHECK(Offset(kFiles, kAllow) == LineHits("src\\x.cpp", {1, 2, 3, 4, 5, 6, 7, 14, 19, 20, 21, 22}) +
                                        ", unused allow src\\x.cpp|(Foo*)((char*)pMissing + 4)"
                                        ", unused allow src\\y.cpp|(Foo*)((char*)pAllowed + 4)");
}

TEST_CASE_NAMED(FHardeningSourceTest4, "Unit::Hardening::allow-x64-verdict", "[EngineFilter]")
{
    const std::vector<std::string> kBad = Hardening::AllowVerdicts(
        {"src\\x.cpp|acSafe|[0x10]|x64:safe:byte buffer", "src\\x.cpp|acBare|[0x10]", "src\\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits"},
        {"src\\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:", "src\\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: "});
    CHECK(Joined(kBad) == "no verdict src\\x.cpp|acBare|[0x10], no verdict src\\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits, "
                          "no verdict src\\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:, "
                          "no verdict src\\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: ");
    const std::vector<std::string> kGoodSpacer = {"src\\x.cpp|acSafe|[0x10]|x64:safe:byte buffer",
                                                  "src\\x.cpp|acBare|[0x10]|x64:fixed:sized by sizeof",
                                                  "src\\x.cpp|mOr|[0x10|0x1]|x64:safe:bit mask"};
    const std::vector<std::string> kGoodOffset = {"src\\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:offset in bytes"};
    CHECK(Hardening::AllowVerdicts(kGoodSpacer, kGoodOffset).empty());
    const std::vector<Hardening::TextFile> kFiles = {
        File("src\\x.cpp", {"unsigned char acSafe[0x10];", "unsigned char acBare[0x10];", "char mOr[0x10 | 0x1];",
                            "q = (Foo*)((char*)pA + (iA | iB));"}),
    };
    CHECK(Spacer(kFiles, kGoodSpacer) == "");
    CHECK(Offset(kFiles, kGoodOffset) == "");
}

TEST_CASE_NAMED(FHardeningSourceTest5, "Unit::Hardening::handle-raw", "[EngineFilter]")
{
    const std::vector<Hardening::TextFile> kFiles = {
        File("src\\x.cpp",
             {"int a = PBL_HANDLE_ID(p);", "return a ? b - 1 : 0xdddddddd;", "h.mSavedHandleId = 0;",
              "return (float)(pOwner->PblHandled::GetHandleId() & 0xf);", "if ((unsigned int)pOld != 0xDDDDDDDDu)",
              "if (object->GetHandleId() == (unsigned int)debugId)", "if (t->GetTracker() != (Tracker*)0xdddddddd)",
              "if (state->handle.GetHandleId() == netHostTraceHandleId)", "if (ptr->GetHandleId() != id)", "h = p;",
              "ok = d.mGameObject->GetHandleId() == d.mGameObjectId;", "x = GameObjectHandleId(p);",
              "if (id != ptr->GetHandleId())", "if (p->GetHandleId() == uSaved)",
              "if (p->GetHandleId() != h.GetHandleId())", "if (p->GetHandleId() == saved.id_)",
              "if (o->GetGameObject() && o->GetGameObject()->PblHandled::GetHandleId())",
              "p = reinterpret_cast<Tracker*>(0xdddddddd);", "if (static_cast<unsigned int>(v) == 0xDDDDDDDD)",
              "if (v != 0xDDDDDDDDUL)", "if (p->GetHandleId())", "if (p->GetHandleId() == 0)",
              "if (object->GetHandleId() == 110)", "if (!mSkip && !(PblHandled::GetHandleId() & 1) == even)",
              "if (GetIndex(i, 1, s->PblHandled::GetHandleId()) && ok)", "if ((int)v.GetHandleId() % 30 != b)",
              "if (pObject->GetHandleId() == (unsigned int)debugWriteHandleId)",
              "if (!map.FindId(pObject->GetHandleId()))"}),
        File("src\\LuaFL\\l.cpp", {"int a = PBL_HANDLE_ID(p);"}),
        File("src\\PebbleFL\\Common\\PblHandle.h", {"unsigned int mHandleId;"}),
    };
    CHECK(RegexRow("handle-raw", kFiles) ==
          LineHits("src\\x.cpp", {1, 3, 5, 7, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22}));
}

TEST_CASE_NAMED(FHardeningSourceTest6, "Unit::Hardening::x64-size-literal", "[EngineFilter]")
{
    const std::vector<std::string> kLines = {
        "MemoryPool Foo::sMemoryPool(\"Foo\", 0x30);",
        "MemoryPool Bar::sMemoryPool(\"Bar\", sizeof(Bar));",
        "MemoryPool Baz::sMemoryPool(\"Baz\", 0);",
        "FreeList Foo::smFreeList(0xcc, 1, true, 4);",
        "FreeList Bar::smFreeList(sizeof(Bar), 1, true, 4);",
        "gPool.Setup(uCount, 0xFD0, 0);",
        "Agent::sMemoryPool.Setup(600, 0x358, Agent::sMemoryPool.GetGrow());",
        "gPool.Setup(Count(), sizeof(Foo), 0);",
        "pPool->Create(2, 16, 0);",
        "mLock.Create(1, 1);",
        "Foo::SetupMemoryPool(16, 0x40, Foo::sMemoryPool.GetGrow());",
        "mValue.Setup(1.0f, 2.0f, 3.0f);",
        "// gPool.Setup(uCount, 0xFD0, 0);",
        "MemoryPool kAllowed(\"allowed\", 16);",
        "Weapon::UpdateMemoryPoolSize(0x150);",
        "Weapon::UpdateMemoryPoolSize(sizeof(Weapon));",
        "p = RedAllocFromHeap(h, n * 4, 0);",
        "p = RedAllocFromHeap(h, n * sizeof(T*), 0);",
        "p = (T**)RedAllocFromHeap(",
        "    h, (unsigned int)n + 4, 0);",
        "p = RedAllocFromHeap(h, n * 4 * sizeof(T), 0);",
        "pPool->Create(c, uBin * 4, 0);",
        "p = RedAllocFromHeap(h, n * 40, 0);",
        "pLua->Create(c, uLua * 4, 0);",
        "p = RedAllocFromHeap(h, 4 * n, 0);",
        "p = RedAllocFromHeap(h, 40 * n, 0);",
        "const unsigned int uiBytes = n * 4;",
        "q = RedAllocFromHeap(h, uiBytes, 0);",
        "const unsigned int uiLead = 4 * n;",
        "Foo::SetupMemoryPool(16, uiLead, Foo::sMemoryPool.GetGrow());",
        "const unsigned int uiOk = n * sizeof(T*);",
        "r = RedAllocFromHeap(h, uiOk, 0);",
        "bool bSame = uiOk == n * 4;",
        "r = RedAllocFromHeap(h, bSame, 0);"};
    Hardening::MemoryTree kTree;
    kTree.Add("src\\a\\x.cpp", Content(kLines));
    kTree.Add("src\\LuaFL\\l.cpp", Content({"MemoryPool Vendor::sMemoryPool(\"Vendor\", 8);"}));
    Hardening::SourceInput kIn = Input(kTree);
    SetAllow(kIn, "SizeLiteralAllow",
             {"src\\a\\x.cpp|MemoryPool kAllowed(\"allowed\", 16)|x64:safe:raw blocks",
              "src\\a\\x.cpp|->Create(c, uLua * 4, 0)|x64:safe:4-byte bins",
              "src\\a\\x.cpp|MemoryPool kMissing(\"missing\", 16)|x64:safe:raw blocks", "src\\a\\x.cpp|.Create(9, 9, 9)"});
    CHECK(RowDetail("x64-size-literal", kIn) ==
          "allow without an x64:safe reason src\\a\\x.cpp|.Create(9, 9, 9), " +
              LineHits("src\\a\\x.cpp", {1, 4, 6, 7, 9, 22, 11, 15, 17, 19, 25, 30, 28}) +
              ", unused allow src\\a\\x.cpp|MemoryPool kMissing(\"missing\", 16)");
    Hardening::MemoryTree kClean;
    kClean.Add("src\\a\\x.cpp",
               Content({"MemoryPool Bar::sMemoryPool(\"Bar\", sizeof(Bar));", "gPool.Setup(uCount, sizeof(Foo), 0);"}));
    kClean.Add("src\\LuaFL\\l.cpp", Content({"MemoryPool Vendor::sMemoryPool(\"Vendor\", 8);"}));
    Hardening::SourceInput kCleanIn = Input(kClean);
    SetAllow(kCleanIn, "SizeLiteralAllow", {});
    std::string sDetail;
    CHECK(RowPasses("x64-size-literal", kCleanIn, sDetail));
    CHECK(sDetail == "");
}

TEST_CASE_NAMED(FHardeningSourceTest7, "Unit::Hardening::serial-value", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\x.cpp", Content({"static int sThreadActivationKey = 0;", "int a = sThreadActivationKey;",
                                     "unsigned b = pObj->mCollisionSerial;", "x = sCollisionSerialNext + 1;",
                                     "k = NextThreadActivationKey();", "int mCollisionSerialTies = 0;",
                                     "q = pA->mCollisionSerial == pB->mCollisionSerial;"}));
    kTree.Add("src\\LuaFL\\l.cpp", Content({"h = sCollisionSerialNext;"}));
    Hardening::SourceInput kIn = Input(kTree);
    SetAllow(kIn, "SerialAllow", {"src\\x.cpp|static int sThreadActivationKey = 0;", "src\\x.cpp|int stale = sThreadActivationKey;"});
    CHECK(RowDetail("serial-value", kIn) == "src\\LuaFL\\l.cpp:1, " + LineHits("src\\x.cpp", {2, 3, 4, 7}) +
                                                ", unused allow src\\x.cpp|int stale = sThreadActivationKey;");
}

TEST_CASE_NAMED(FHardeningSourceTest8, "Unit::Hardening::addr-order", "[EngineFilter]")
{
    const std::vector<std::string> kLines = {"int k = (int)this;",
                                             "if ((unsigned int)pA > 1) {}",
                                             "if (1 < (unsigned int)pB) {}",
                                             "if (reinterpret_cast<uintptr_t>(pA) > 1) {}",
                                             "if (1 > reinterpret_cast<int>(pA)) {}",
                                             "bool b = PblPtrAfter(x, y);",
                                             "if (pOther > pObject) {}",
                                             "int c = iCount > 3;",
                                             "pGroup->mThreads.Add(&mNode, this, (int)pOwner);",
                                             "if (ppA > ppB) {}",
                                             "bool e = (const char*)pA > (const char*)pB;",
                                             "bool d = std::less<const void*>()(pA, pB);",
                                             "bool f = obj > other;",
                                             "mList.Add(&mNode, this, (int)iKey);",
                                             "n = (char*)pA - (char*)pB;"};
    Hardening::MemoryTree kTree;
    kTree.Add("src\\Battlefront2\\Source\\a.cpp", Content(kLines));
    kTree.Add("src\\RedEngineFL\\a.cpp", Content(kLines));
    Hardening::SourceInput kIn = Input(kTree);
    SetAllow(kIn, "AddrOrderAllow", {});
    CHECK(RowDetail("addr-order", kIn) == LineHits("src\\Battlefront2\\Source\\a.cpp", {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12}));
    SetAllow(kIn, "AddrOrderAllow", {"src\\Battlefront2\\Source\\a.cpp|if (pOther > pObject) {}"});
    const std::string sAllowed = RowDetail("addr-order", kIn);
    CHECK(sAllowed.find(":7,") == std::string::npos);
    const bool bEndsAtSeven = sAllowed.size() >= 2 && sAllowed.compare(sAllowed.size() - 2, 2, ":7") == 0;
    CHECK_FALSE(bEndsAtSeven);
    SetAllow(kIn, "AddrOrderAllow",
             {"src\\Battlefront2\\Source\\a.cpp|if (pOther > pObject) {}", "src\\Battlefront2\\Source\\a.cpp|int nope;"});
    CHECK(RowDetail("addr-order", kIn).find("unused allow src\\Battlefront2\\Source\\a.cpp|int nope;") != std::string::npos);
}

TEST_CASE_NAMED(FHardeningSourceTest9, "Unit::Hardening::crt-heap-ban", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\mimalloc\\src\\x.c", Content({"void* p = malloc(1);"}));
    kTree.Add("src\\PebbleFL\\Common\\PblCrtHeap.cpp", Content({"void* p = malloc(1);", "free(p);"}));
    kTree.Add("src\\FFmpegFL\\libavutil\\mem.c", Content({"void* p = _aligned_malloc(1, 16);"}));
    Hardening::MemoryTree kBad = kTree;
    kBad.Add("src\\Foo\\a.cpp", Content({"void* p = malloc(1);", "void* q = calloc (1, 2);", "p = realloc(p, 4);", "free(p);",
                                         "char* s = _strdup(\"x\");", "size_t z = _msize(q);", "pBackend->free(p);",
                                         "mFree(p);", "RedFreeToHeap(0, p, 0);", "PblCrtFree(p);", "int free_count = 0;",
                                         "std::free(p);"}));
    kBad.Add("src\\RedEngineFL\\Memory\\m.cpp", Content({"void* p = _aligned_malloc(1, 16);"}));
    CHECK(RowDetail("crt-heap-ban", Input(kBad)) == LineHits("src\\Foo\\a.cpp", {1, 2, 3, 4, 5, 6}) +
                                                        ", src\\RedEngineFL\\Memory\\m.cpp:1");
    std::string sDetail;
    CHECK(RowPasses("crt-heap-ban", Input(kTree), sDetail));
}

TEST_CASE_NAMED(FHardeningSourceTest10, "Unit::Hardening::mi-api-ban", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\mimalloc\\src\\x.c", Content({"void* p = mi_malloc(1);"}));
    kTree.Add("src\\RedEngineFL\\Memory\\RedMemoryMi.cpp", Content({"void* p = mi_malloc(1);"}));
    Hardening::MemoryTree kBad = kTree;
    kBad.Add("src\\Foo\\a.cpp", Content({"void* p = mi_malloc(1);", "mi_heap_t* h = mi_heap_new();",
                                         "void* q = mi_heap_malloc (h, 1);", "mi_free(q);", "int v = MI_VERSION;",
                                         "int mi_count = 0;", "mi_option_set(mi_option_verbose, 1);"}));
    CHECK(RowDetail("mi-api-ban", Input(kBad)) == LineHits("src\\Foo\\a.cpp", {1, 2, 3, 4, 7}));
    std::string sDetail;
    CHECK(RowPasses("mi-api-ban", Input(kTree), sDetail));
}

TEST_CASE_NAMED(FHardeningSourceTest11, "Unit::Hardening::new-size-t", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\Foo\\a.cpp", Content({"void* Foo::operator new(size_t uSize);"}));
    kTree.Add("tests\\third_party\\t.h", Content({"void* operator " "new(unsigned int n);"}));
    Hardening::MemoryTree kBad = kTree;
    kBad.Add("tests\\stubs\\s.cpp", Content({"int x;", "void* AIPath::operator " "new(unsigned int uSize) { return 0; }"}));
    CHECK(RowDetail("new-size-t", Input(kBad)) == "tests\\stubs\\s.cpp:2");
    Hardening::MemoryTree kGood = kTree;
    kGood.Add("tests\\stubs\\s.cpp", Content({"int x;", "void* AIPath::operator new(size_t uSize) { return 0; }"}));
    std::string sDetail;
    CHECK(RowPasses("new-size-t", Input(kGood), sDetail));
}

TEST_CASE_NAMED(FHardeningSourceTest12, "Unit::Hardening::steamid-abi", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\Steam\\steamworks.h", Content({"class CSteamID;", "class ISteamUser", "{", "    virtual CSteamID GetSteamID() = 0;",
                                                   "    virtual void EndAuthSession(CSteamID) = 0;", "};"}));
    Hardening::MemoryTree kBad = kTree;
    kBad.Add("src\\Foo\\a.cpp", Content({"void A(ISteamUser* pUser) { pUser->GetSteamID(); }",
                                         "void B(ISteamUser& k) { k.EndAuthSession(x); }", "void C(CSteamID kId);",
                                         "class CSteamID;", "void D(const CSteamID* pId, CSteamID& kId);",
                                         "void E() { pUser->BLoggedOn(); }",
                                         "int F() { return ISteamUser::AdvertiseGame (a, b, c); }"}));
    const std::string sNote = " (settle the x64 CSteamID hidden-return and by-value convention first)";
    CHECK(RowDetail("steamid-abi", Input(kBad)) == "src\\Foo\\a.cpp:1" + sNote + ", src\\Foo\\a.cpp:2" + sNote +
                                                       ", src\\Foo\\a.cpp:3" + sNote + ", src\\Foo\\a.cpp:7" + sNote);
    std::string sDetail;
    CHECK(RowPasses("steamid-abi", Input(kTree), sDetail));
}

TEST_CASE_NAMED(FHardeningSourceTest13, "Unit::Hardening::unicows-staged", "[EngineFilter]")
{
    const std::string sShipped = RepoText("BattlefrontII.vcxproj");
    const std::string sProj = "<Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\"><Target Name=\"StageGameData\">"
                              "<ItemGroup><GameDataFile Include=\"$(SWBF2_GAMEDATA)\\steam_api.dll\" />{0}</ItemGroup>"
                              "<Copy SourceFiles=\"{1}\" DestinationFolder=\"$(OutDir)\" /><Delete Files=\"$(OutDir)unicows.dll\" />"
                              "</Target></Project>";
    const std::string sCopyGood = "@(GameDataFile)";
    const std::string sSrcGood = Content({"int a;"});
    const std::string sSrcBad = Content({"LoadLibraryA(\"unicows.dll\");"});
    struct UnicowsCase
    {
        const char* pcName;
        bool bPass;
        std::string sProject;
        std::vector<std::string> kWorkers;
        bool bDefaultWorkers;
        std::string sSrc;
        const char* pcDetail;
    };
    const std::vector<UnicowsCase> kCases = {
        {"the shipped project with a clean worker list and src", true, sShipped, {}, true, sSrcGood, 0},
        {"a project with only the Delete of the stale copy", true, Formatted(sProj, {"", sCopyGood}),
         {"BattlefrontII.exe", "steam_appid.txt"}, false, sSrcGood, 0},
        {"a GameDataFile item naming unicows", false,
         Formatted(sProj, {"<GameDataFile Include=\"$(SWBF2_GAMEDATA)\\unicows.dll\" />", sCopyGood}), {}, true, sSrcGood,
         "vcxproj GameDataFile"},
        {"a VendorFile item naming unicows", false,
         Formatted(sProj, {"<VendorFile Include=\"$(SWBF2_VENDOR_X64)\\unicows.dll\" />", sCopyGood}), {}, true, sSrcGood,
         "vcxproj VendorFile"},
        {"a Copy whose SourceFiles names unicows", false, Formatted(sProj, {"", "$(SWBF2_GAMEDATA)\\unicows.dll"}), {}, true,
         sSrcGood, "vcxproj Copy"},
        {"unicows.dll in the worker list", false, sShipped, {"BattlefrontII.exe", "unicows.dll"}, false, sSrcGood, "worker list"},
        {"a src file naming unicows", false, sShipped, {}, true, sSrcBad, "named in "},
    };
    for (size_t i = 0; i < kCases.size(); ++i)
    {
        const UnicowsCase& k = kCases[i];
        CAPTURE(k.pcName);
        Hardening::MemoryTree kTree;
        kTree.Add("BattlefrontII.vcxproj", k.sProject);
        kTree.Add("src\\a.cpp", k.sSrc);
        Hardening::SourceInput kIn = Input(kTree);
        kIn.kWin32Workers = k.bDefaultWorkers ? Gate::SmokeWorkerFiles("Win32") : k.kWorkers;
        kIn.kX64Workers = k.bDefaultWorkers ? Gate::SmokeWorkerFiles("x64") : k.kWorkers;
        std::string sDetail;
        CHECK(RowPasses("unicows-staged", kIn, sDetail) == k.bPass);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(sDetail.find(k.pcDetail) != std::string::npos, sDetail);
        }
        if (k.bPass)
        {
            CHECK(sDetail == "");
        }
    }
}

TEST_CASE_NAMED(FHardeningSourceTest14, "Unit::Hardening::d3dx9-pins", "[EngineFilter]")
{
    const std::string sPins = RepoText("lib\\vendor-pins.props");
    const std::string sProj = RepoText("BattlefrontII.vcxproj");
    std::map<std::string, std::string, Hardening::CultureLess> kPins;
    std::string sError;
    REQUIRE_MESSAGE(Hardening::VendorPins(sPins, kPins, sError), sError);
    const std::string sX64Pin = kPins["D3dxX64Sha256"];
    REQUIRE(sX64Pin.size() > 1);
    const std::string sWin32Verify = "<VerifyFileHash Condition=\"'$(Platform)'=='Win32'\" File=\"$(D3dxDll)\" Algorithm=\"SHA256\" "
                                     "Hash=\"$(D3dxWin32Sha256)\" />";
    const std::string sX64Verify = "<VerifyFileHash Condition=\"'$(Platform)'=='x64'\" File=\"$(D3dxDll)\" Algorithm=\"SHA256\" "
                                   "Hash=\"$(D3dxX64Sha256)\" />";
    const std::string sNupkgVerify = "<VerifyFileHash File=\"$(D3dxNupkg)\" Algorithm=\"SHA256\" Hash=\"$(D3dxNupkgSha256)\" />";
    const std::vector<PinCase> kCases = {
        {"the shipped pins and project", true, sPins, sProj, 0},
        {"a 63-digit DLL pin", false, Replaced(sPins, sX64Pin, sX64Pin.substr(1)), sProj, "D3dxX64Sha256 is not 64 hex digits"},
        {"an upper-case nupkg pin", false, Replaced(sPins, "ead0906ae8a26c18", "EAD0906AE8A26C18"), sProj,
         "D3dxNupkgSha256 is not 64 hex digits"},
        {"a project without the Win32 DLL VerifyFileHash", false, sPins, Replaced(sProj, sWin32Verify, ""),
         "against $(D3dxWin32Sha256)"},
        {"a project without the x64 DLL VerifyFileHash", false, sPins, Replaced(sProj, sX64Verify, ""),
         "against $(D3dxX64Sha256)"},
        {"a project whose cache VerifyFileHash is conditional", false, sPins,
         Replaced(sProj, sNupkgVerify, Replaced(sNupkgVerify, "<VerifyFileHash ", "<VerifyFileHash Condition=\"false\" ")),
         "of $(D3dxNupkg) against"},
        {"a project that does not import the pins", false, sPins,
         Replaced(sProj, "<Import Project=\"lib\\vendor-pins.props\" />", ""), "does not import"},
        {"a project that extracts only a missing cached DLL", false, sPins,
         Replaced(sProj, "<Unzip Condition=\"'$(D3dxDllHash)' != '$(D3dxDllSha256)'\"", "<Unzip Condition=\"!Exists('$(D3dxDll)')\""),
         "not conditioned on the cached DLL hash"},
    };
    CheckPinCases("d3dx9-pins", sPins, sProj, kCases);
}

TEST_CASE_NAMED(FHardeningSourceTest15, "Unit::Hardening::steam-api64-pins", "[EngineFilter]")
{
    const std::string sPins = RepoText("lib\\vendor-pins.props");
    const std::string sProj = RepoText("BattlefrontII.vcxproj");
    std::map<std::string, std::string, Hardening::CultureLess> kPins;
    std::string sError;
    REQUIRE_MESSAGE(Hardening::VendorPins(sPins, kPins, sError), sError);
    const std::string sPin = kPins["SteamApi64Sha256"];
    REQUIRE(sPin.size() > 1);
    const Hardening::RegexPattern kVerifyRx(R"rx(<VerifyFileHash [^>]*File="\$\(SWBF2_VENDOR_X64\)\\steam_api64\.dll"[^>]*/>)rx", false);
    const Hardening::RegexPattern kCopyRx(R"rx(<Copy [^>]*SourceFiles="\$\(SWBF2_VENDOR_X64\)\\steam_api64\.dll"[^>]*/>)rx", false);
    Hardening::RegexMatcher kVerifyMatch(kVerifyRx, sProj);
    Hardening::RegexMatcher kCopyMatch(kCopyRx, sProj);
    REQUIRE_MESSAGE(kVerifyMatch.FindNext(), "StageVendor steam_api64 steps not found in BattlefrontII.vcxproj");
    REQUIRE_MESSAGE(kCopyMatch.FindNext(), "StageVendor steam_api64 steps not found in BattlefrontII.vcxproj");
    const std::string sVerify = sProj.substr(kVerifyMatch.GetMatchBeginning(), kVerifyMatch.GetMatchLength());
    const std::string sCopy = sProj.substr(kCopyMatch.GetMatchBeginning(), kCopyMatch.GetMatchLength());
    const std::vector<PinCase> kCases = {
        {"the shipped pin and project", true, sPins, sProj, 0},
        {"a 63-digit steam_api64 pin", false, Replaced(sPins, sPin, sPin.substr(1)), sProj, "SteamApi64Sha256 is not 64 hex digits"},
        {"a Copy without a VerifyFileHash", false, sPins, Replaced(sProj, sVerify, ""), "without a VerifyFileHash"},
        {"a VerifyFileHash after the Copy", false, sPins,
         Replaced(sProj, sVerify + "\r\n    " + sCopy, sCopy + "\r\n    " + sVerify), "without a VerifyFileHash"},
        {"a VerifyFileHash with another condition", false, sPins,
         Replaced(sProj, sVerify, Replaced(sVerify, "<VerifyFileHash Condition=\"", "<VerifyFileHash Condition=\"false and ")),
         "without a VerifyFileHash"},
        {"a project that never copies steam_api64.dll", false, sPins, Replaced(sProj, sCopy, ""), "has no Copy"},
    };
    CheckPinCases("steam-api64-pins", sPins, sProj, kCases);
}

TEST_CASE_NAMED(FHardeningSourceTest16, "Unit::Hardening::bink-gone", "[EngineFilter]")
{
    const std::string sProj = "<Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\"><ItemDefinitionGroup><Link>"
                              "<AdditionalDependencies>kernel32.lib;{0}</AdditionalDependencies></Link></ItemDefinitionGroup>"
                              "<ItemGroup>{1}</ItemGroup><Target Name=\"StageVendor\"><Delete Files=\"$(OutDir)unicows.dll\" />{2}"
                              "</Target></Project>";
    const std::string sDelete = "<Delete Files=\"$(OutDir)binkw32.dll\" />";
    const std::vector<std::string> kWorkers = {"BattlefrontII.exe", "openswbf2-ffmpeg.dll"};
    const std::string sLabelAllow = "tests\\game\\x.cpp|binkw32 ingame digest|a table label";
    struct BinkCase
    {
        const char* pcName;
        bool bPass;
        std::map<std::string, std::string> kFiles;
        std::vector<std::string> kWorkers;
        const char* pcDetail;
        std::vector<std::string> kAllow;
    };
    const std::vector<BinkCase> kCases = {
        {"a clean tree", true, {}, kWorkers, 0, {}},
        {"a FFmpegFL source calling BinkOpen and a game source with PixelBink and IsBinkRevision and bink in lower case",
         true,
         {{"src\\FFmpegFL\\libavcodec\\x.c", "Bink" "Open(p);"},
          {"src\\Game\\a.cpp", "PixelBink(1); IsBinkRevision(c); av_find_input_format(\"bink\");"}},
         kWorkers, 0, {}},
        {"lib\\Win32\\binkw32.lib", false, {{"lib\\Win32\\binkw32.lib", "x"}}, kWorkers, "lib file lib\\Win32\\binkw32.lib", {}},
        {"binkw32.lib in AdditionalDependencies", false,
         {{"BattlefrontII.vcxproj", Formatted(sProj, {"lib\\Win32\\binkw32.lib", "", sDelete})}}, kWorkers,
         "BattlefrontII.vcxproj AdditionalDependencies", {}},
        {"a GameDataFile staging binkw32.dll", false,
         {{"BattlefrontII.vcxproj", Formatted(sProj, {"", "<GameDataFile Include=\"$(SWBF2_GAMEDATA)\\binkw32.dll\" />", sDelete})}},
         kWorkers, "BattlefrontII.vcxproj GameDataFile", {}},
        {"a library project linking binkw32", false, {{"src\\Game\\Game.vcxproj", Formatted(sProj, {"binkw32.lib", "", ""})}},
         kWorkers, "src\\Game\\Game.vcxproj AdditionalDependencies", {}},
        {"a StageVendor without the binkw32.dll Delete", false, {{"BattlefrontII.vcxproj", Formatted(sProj, {"", "", ""})}},
         kWorkers, "StageVendor does not delete a stale binkw32.dll", {}},
        {"binkw32 named in tests\\game\\x.cpp", false, {{"tests\\game\\x.cpp", "const char* p = \"binkw32.dll\";"}}, kWorkers,
         "named in tests\\game\\x.cpp", {}},
        {"binkw32 named without the extension in tests\\harness\\h.cpp", false,
         {{"tests\\harness\\h.cpp", "const char* p = \"binkw32\";"}}, kWorkers, "named in tests\\harness\\h.cpp", {}},
        {"an allowed section label in tests\\game\\x.cpp", true,
         {{"tests\\game\\x.cpp", "Find(sTable, \"binkw32 ingame digest\");"}}, kWorkers, "", {sLabelAllow}},
        {"a section label with no allow entry", false, {{"tests\\game\\x.cpp", "Find(sTable, \"binkw32 ingame digest\");"}},
         kWorkers, "named in tests\\game\\x.cpp", {}},
        {"an allowed section label beside a binkw32.dll name", false,
         {{"tests\\game\\x.cpp", "Find(sTable, \"binkw32 ingame digest\"); Load(\"binkw32.dll\");"}}, kWorkers,
         "named in tests\\game\\x.cpp", {sLabelAllow}},
        {"an allow entry for another file", false, {{"tests\\game\\x.cpp", "Find(sTable, \"binkw32 ingame digest\");"}},
         kWorkers, "named in tests\\game\\x.cpp", {"tests\\game\\y.cpp|binkw32 ingame digest|a table label"}},
        {"an unused allow entry", false, {}, kWorkers, "unused allow tests\\game\\x.cpp|binkw32 ingame digest|a table label",
         {sLabelAllow}},
        {"an allow entry without a reason", false, {{"tests\\game\\x.cpp", "Find(sTable, \"binkw32 ingame digest\");"}},
         kWorkers, "named in tests\\game\\x.cpp", {"tests\\game\\x.cpp|binkw32 ingame digest"}},
        {"binkw32 named in tests\\gate\\gate_machine.cpp", false,
         {{"tests\\gate\\gate_machine.cpp", "const char* const a[] = {\"BINKW32.DLL\"};"}}, kWorkers,
         "named in tests\\gate\\gate_machine.cpp", {}},
        {"binkw32.dll in the worker list", false, {}, {"BattlefrontII.exe", "binkw32.dll"}, "worker list", {}},
        {"a game source calling BinkOpen", false, {{"src\\Game\\a.cpp", "int a;\r\nvoid f() { Bink" "Open(0, 0); }"}},
         kWorkers, "source src\\Game\\a.cpp:2", {}},
        {"a test source using the Bink struct", false, {{"tests\\unit\\b.cpp", "static_assert(sizeof(BI" "NK) == 0xd4, \"\");"}},
         kWorkers, "source tests\\unit\\b.cpp:1", {}},
        {"a game header declaring the Bink IO struct", false, {{"src\\Game\\a.h", "struct BI" "NKIO;"}}, kWorkers,
         "source src\\Game\\a.h:1", {}},
        {"a game header declaring the Bink file struct", false, {{"src\\Game\\a.h", "struct BI" "NKFILE;"}}, kWorkers,
         "source src\\Game\\a.h:1", {}},
        {"a game source loading binkw32.dll at run time", false,
         {{"src\\Game\\a.cpp", "int a;\r\nvoid f() { LoadLibraryA(\"binkw32.dll\"); }"}}, kWorkers, "source src\\Game\\a.cpp:2", {}},
        {"a game source naming BinkW64 without the extension", false, {{"src\\Game\\a.cpp", "const char* p = \"BinkW64\";"}},
         kWorkers, "source src\\Game\\a.cpp:1", {}},
        {"a test source naming the binkw32 golden section", true, {{"tests\\unit\\b.cpp", "const char* s = \"binkw32 frames\";"}},
         kWorkers, "", {}},
    };
    for (size_t i = 0; i < kCases.size(); ++i)
    {
        const BinkCase& k = kCases[i];
        CAPTURE(k.pcName);
        std::map<std::string, std::string> kFiles = {{"BattlefrontII.vcxproj", Formatted(sProj, {"", "", sDelete})},
                                                     {"src\\Game\\a.cpp", "int a;"},
                                                     {"tests\\unit\\b.cpp", "int b;"},
                                                     {"tests\\game\\x.cpp", "int x;"},
                                                     {"tests\\gate\\gate_machine.cpp", "int b;"}};
        for (std::map<std::string, std::string>::const_iterator it = k.kFiles.begin(); it != k.kFiles.end(); ++it)
        {
            kFiles[it->first] = it->second;
        }
        Hardening::MemoryTree kTree;
        for (std::map<std::string, std::string>::const_iterator it = kFiles.begin(); it != kFiles.end(); ++it)
        {
            kTree.Add(it->first, it->second);
        }
        Hardening::SourceInput kIn = Input(kTree);
        kIn.kWin32Workers = k.kWorkers;
        SetAllow(kIn, "BinkNameAllow", k.kAllow);
        std::string sDetail;
        CHECK_MESSAGE(RowPasses("bink-gone", kIn, sDetail) == k.bPass, sDetail);
        if (k.bPass)
        {
            CHECK(sDetail == "");
        }
        else if (k.pcDetail && *k.pcDetail)
        {
            CHECK_MESSAGE(sDetail.compare(0, strlen(k.pcDetail), k.pcDetail) == 0, sDetail);
        }
    }
}

TEST_CASE_NAMED(FHardeningSourceTest18, "Unit::Hardening::redlog-vendor", "[EngineFilter]")
{
    Hardening::MemoryTree kTree;
    kTree.Add("src\\Battlefront2\\Source\\a.cpp", "#include \"RedEngineFL/Util/RedLog.h\"");
    kTree.Add("src\\LuaFL\\src\\l.c", "int a;");
    std::string sDetail;
    CHECK(RowPasses("redlog-vendor", Input(kTree), sDetail));
    kTree.Add("src\\ZlibFL\\z.c", "#include \"REDLOG.H\"");
    kTree.Add("src\\mimalloc\\m.c", "int m;");
    CHECK(RowDetail("redlog-vendor", Input(kTree)) == std::string(kFixtureRoot) + "\\src\\ZlibFL\\z.c");
}
