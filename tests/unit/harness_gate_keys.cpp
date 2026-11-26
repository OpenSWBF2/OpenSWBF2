#include <windows.h>

#include <string>
#include <vector>

#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "gate/gate_keys.h"
#include "gate/gate_readlist.h"

using namespace Gate;

namespace
{

void Put32(std::vector<unsigned char>& k, size_t uAt, unsigned int uValue)
{
    k[uAt] = (unsigned char)uValue;
    k[uAt + 1] = (unsigned char)(uValue >> 8);
    k[uAt + 2] = (unsigned char)(uValue >> 16);
    k[uAt + 3] = (unsigned char)(uValue >> 24);
}

void Put16(std::vector<unsigned char>& k, size_t uAt, unsigned int uValue)
{
    k[uAt] = (unsigned char)uValue;
    k[uAt + 1] = (unsigned char)(uValue >> 8);
}

std::vector<unsigned char> MakePe(bool bPlus)
{
    std::vector<unsigned char> k(0x400);
    for (size_t i = 0; i < k.size(); ++i)
    {
        k[i] = (unsigned char)(0x30 + i % 0x40);
    }
    const size_t e = 0x80;
    const size_t uOpt = e + 24;
    const unsigned int uOptSize = bPlus ? 240 : 224;
    k[0] = 'M';
    k[1] = 'Z';
    Put32(k, 0x3C, (unsigned int)e);
    Put32(k, e, 0x4550);
    Put16(k, e + 6, 1);
    Put32(k, e + 8, 0x11223344);
    Put16(k, e + 20, uOptSize);
    Put16(k, uOpt, bPlus ? 0x20B : 0x10B);
    Put32(k, uOpt + 64, 0xAABBCCDD);
    const size_t uCount = uOpt + (bPlus ? 108 : 92);
    const size_t uDir = uOpt + (bPlus ? 112 : 96);
    Put32(k, uCount, 16);
    Put32(k, uDir + 48, 0x1000);
    Put32(k, uDir + 52, 28);
    const size_t uSec = uOpt + uOptSize;
    Put32(k, uSec + 12, 0x1000);
    Put32(k, uSec + 16, 0x200);
    Put32(k, uSec + 20, 0x200);
    Put32(k, 0x204, 0x55667788);
    Put32(k, 0x20C, 2);
    Put32(k, 0x210, 24);
    Put32(k, 0x218, 0x300);
    k[0x300] = 'R';
    k[0x301] = 'S';
    k[0x302] = 'D';
    k[0x303] = 'S';
    return k;
}

bool ZeroedOnly(const std::vector<unsigned char>& kOld, const std::vector<unsigned char>& kNew,
                const std::vector<std::pair<size_t, size_t> >& kRanges)
{
    if (kOld.size() != kNew.size())
    {
        return false;
    }
    for (size_t i = 0; i < kOld.size(); ++i)
    {
        bool bZeroed = false;
        for (size_t r = 0; r < kRanges.size(); ++r)
        {
            if (i >= kRanges[r].first && i < kRanges[r].first + kRanges[r].second)
            {
                bZeroed = true;
            }
        }
        if (bZeroed ? kNew[i] != 0 : kNew[i] != kOld[i])
        {
            return false;
        }
    }
    return true;
}

std::string TempDir(const char* pcName)
{
    wchar_t acTemp[MAX_PATH];
    GetTempPathW(MAX_PATH, acTemp);
    char acTail[96];
    _snprintf(acTail, sizeof(acTail), "openswbf2-%s-%lu-%lu", pcName, GetCurrentProcessId(), GetTickCount());
    acTail[sizeof(acTail) - 1] = 0;
    const std::string sDir = JoinPath(WideToUtf8(acTemp), acTail);
    MakeDirs(sDir);
    return sDir;
}

void RemoveFiles(const std::string& sDir, const std::vector<std::string>& kRel)
{
    for (size_t i = 0; i < kRel.size(); ++i)
    {
        DeleteFileW(Utf8ToWide(JoinPath(sDir, kRel[i])).c_str());
    }
}

std::string Utf16File(const std::string& sText)
{
    const std::wstring sWide = Utf8ToWide(sText);
    std::string sBytes = "\xFF\xFE";
    for (size_t i = 0; i < sWide.size(); ++i)
    {
        sBytes += (char)(sWide[i] & 0xFF);
        sBytes += (char)(sWide[i] >> 8);
    }
    return sBytes;
}

KeyInput PinnedInput()
{
    KeyInput k;
    KeyPairs kExe;
    kExe.push_back(std::make_pair(std::string("bin\\Win32\\Asan\\BattlefrontII.exe"), std::string("ee")));
    KeyPairs kDlls;
    kDlls.push_back(std::make_pair(std::string("b.dll"), std::string("bb")));
    kDlls.push_back(std::make_pair(std::string("a.dll"), std::string("aa")));
    k.kHeader.push_back(std::make_pair(std::string("step"), std::string("corpus-Asan")));
    k.kHeader.push_back(std::make_pair(std::string("platform"), std::string("Win32")));
    k.kHeader.push_back(std::make_pair(std::string("config"), std::string("Asan")));
    k.kHeader.push_back(std::make_pair(std::string("toolset"), std::string("14.44.35207")));
    k.kHeader.push_back(std::make_pair(std::string("cl"), std::string("19.44.35222")));
    k.kHeader.push_back(std::make_pair(std::string("ucrt"), std::string("10.0.26100.9711 (WinBuild.160101.0800)")));
    k.kHeader.push_back(std::make_pair(std::string("steam"), std::string("yes")));
    k.kHeader.push_back(std::make_pair(std::string("overlay"), std::string("off")));
    k.kHeader.push_back(std::make_pair(std::string("gamedata"), std::string("gd01")));
    k.kHeader.push_back(std::make_pair(std::string("exe"), KeyMap(kExe)));
    k.kHeader.push_back(std::make_pair(std::string("dlls"), KeyMap(kDlls)));
    k.kHeader.push_back(std::make_pair(std::string("vendor"), std::string("none")));
    k.kHeader.push_back(std::make_pair(std::string("asanrt"), std::string("rt01")));
    k.kHeader.push_back(std::make_pair(std::string("tlog"), std::string("ok")));
    k.kJudgeKinds.push_back("compare");
    k.kFiles["tests\\unit\\movie_corpus.cpp"] = "c1";
    k.kFiles["cmd:tests\\unit\\movie_corpus.cpp"] = "m1";
    k.kFiles["SRC\\Common\\Shared.h"] = "s1";
    k.kFiles["tests\\README.md"] = "r1";
    k.kFiles["tests\\baseline\\v143\\mem-peaks.txt"] = "p1";
    k.kFiles["tests\\hardening.ps1"] = "h1";
    k.kFiles["gate-step-definition"] = "d1";
    return k;
}

void SetHeader(KeyInput& k, const char* pcName, const std::string& sValue)
{
    for (size_t i = 0; i < k.kHeader.size(); ++i)
    {
        if (k.kHeader[i].first == pcName)
        {
            k.kHeader[i].second = sValue;
        }
    }
}

const char* const kRoot = "C:\\Dev\\OpenSWBF2";

const char* const kReadTlogText = "^C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\MOVIE_CORPUS.CPP\r\n"
                                  "C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\MOVIE_CORPUS.CPP\r\n"
                                  "C:\\DEV\\OPENSWBF2\\SRC\\COMMON\\SHARED.H\r\n"
                                  "C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\RUNNER.H\r\n"
                                  "C:\\OUTSIDE\\SDK.H\r\n"
                                  "^C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\OTHER.CPP\r\n"
                                  "C:\\DEV\\OPENSWBF2\\SRC\\COMMON\\UNRELATED.H\r\n"
                                  "^C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\RUNNER.CPP\r\n"
                                  "C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\REPORT.H\r\n";

const char* const kCommandTlogText =
    "^C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\MOVIE_CORPUS.CPP\r\n"
    "/c /IC:\\DEV\\OPENSWBF2\\TESTS\\..\\SRC /Od /D OPENSWBF2_TEST /Zc:alignedNew-  "
    "C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\MOVIE_CORPUS.CPP\r\n"
    "^C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\OTHER.CPP\r\n"
    "/c /Od C:\\DEV\\OPENSWBF2\\TESTS\\UNIT\\OTHER.CPP\r\n"
    "^C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\RUNNER.CPP\r\n"
    "/c /Od C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\RUNNER.CPP\r\n";

std::map<std::string, std::string> FixtureTree()
{
    std::map<std::string, std::string> k;
    k["src\\movie\\dec.c"] = "dec1";
    k["src\\common\\shared.h"] = "shared1";
    k["src\\common\\unrelated.h"] = "unrelated1";
    k["tests\\unit\\movie_corpus.cpp"] = "corpus1";
    k["tests\\unit\\other.cpp"] = "other1";
    k["tests\\harness\\runner.h"] = "runnerh1";
    k["tests\\harness\\runner.cpp"] = "runnercpp1";
    k["tests\\harness\\report.h"] = "reporth1";
    k["tests\\excluded.tsv"] = "excluded1";
    return k;
}

bool OldTimes(const std::string&, unsigned long long& uTime)
{
    uTime = 100;
    return true;
}

bool SharedNewer(const std::string& sPath, unsigned long long& uTime)
{
    uTime = LowerText(sPath).find("\\shared.h") != std::string::npos ? 300 : 100;
    return true;
}

}

TEST_CASE_NAMED(FGateKeysTest1, "Unit::Gate::PE bytes are normalised", "[EngineFilter]")
{
    for (int iPlus = 0; iPlus < 2; ++iPlus)
    {
        const std::vector<unsigned char> kOld = MakePe(iPlus != 0);
        std::vector<unsigned char> kNew = kOld;
        std::string sError;
        REQUIRE_MESSAGE(PeNormalise(kNew, sError), sError);
        const size_t uOpt = 0x80 + 24;
        std::vector<std::pair<size_t, size_t> > kRanges;
        kRanges.push_back(std::make_pair((size_t)0x88, (size_t)4));
        kRanges.push_back(std::make_pair(uOpt + 64, (size_t)4));
        kRanges.push_back(std::make_pair((size_t)0x204, (size_t)4));
        kRanges.push_back(std::make_pair((size_t)0x304, (size_t)20));
        CHECK(ZeroedOnly(kOld, kNew, kRanges));
    }
    std::vector<unsigned char> kShort = MakePe(false);
    kShort.resize(0x100);
    std::string sError;
    CHECK_FALSE(PeNormalise(kShort, sError));
    CHECK(sError == "not a PE file: optional header truncated");
    std::vector<unsigned char> kCut = MakePe(true);
    kCut.resize(0x210);
    CHECK_FALSE(PeNormalise(kCut, sError));
    CHECK(sError == "not a PE file: debug directory outside the file");
    std::vector<unsigned char> kNoMz(0x40);
    CHECK_FALSE(PeNormalise(kNoMz, sError));
    CHECK(sError == "not a PE file: no MZ header");
}

TEST_CASE_NAMED(FGateKeysTest2, "Unit::Gate::key text is pinned", "[EngineFilter]")
{
    CHECK(TextSha("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(TextSha("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    const StepKey kKey = MakeStepKey(PinnedInput());
    const std::string sRun = "step\tcorpus-Asan\nplatform\tWin32\nconfig\tAsan\ntoolset\t14.44.35207\n"
                             "cl\t19.44.35222\nucrt\t10.0.26100.9711 (WinBuild.160101.0800)\nsteam\tyes\noverlay\toff\n"
                             "gamedata\tgd01\nexe\tbin\\Win32\\Asan\\BattlefrontII.exe=ee\ndlls\ta.dll=aa;b.dll=bb\n"
                             "vendor\tnone\nasanrt\trt01\ntlog\tok\n"
                             "cmd:tests\\unit\\movie_corpus.cpp\tm1\ngate-step-definition\td1\n"
                             "src\\common\\shared.h\ts1\ntests\\hardening.ps1\th1\ntests\\unit\\movie_corpus.cpp\tc1";
    CHECK(kKey.bValid);
    CHECK(kKey.sPlatform == "Win32");
    CHECK(kKey.sRunText == sRun);
    CHECK(kKey.sJudgeText == "tests\\baseline\\v143\\mem-peaks.txt\tp1");
    CHECK(kKey.sRunKey == "1633eb1c99e472619ef924d21fb87cbc8d60f1534ad4cae24167449af5019bc9");
    CHECK(kKey.sJudgeKey == "fffd2df2e09fb375e88ad816e93321a40ecc9ea13efa373609bd52b25f7d1364");
    REQUIRE(kKeyHeaderCount == 14);
    for (size_t i = 0; i < kKeyHeaderCount; ++i)
    {
        CHECK(kKey.kHeader[i].substr(0, kKey.kHeader[i].find('\t')) == kKeyHeaderNames[i]);
    }

    KeyInput kRelease = PinnedInput();
    SetHeader(kRelease, "config", "Release");
    SetHeader(kRelease, "asanrt", "none");
    const StepKey kNone = MakeStepKey(kRelease);
    CHECK(kNone.bValid);
    CHECK(kNone.kHeader[12] == "asanrt\tnone");
    CHECK(kNone.sRunKey != kKey.sRunKey);

    KeyInput kDash = PinnedInput();
    KeyPairs kDlls;
    kDlls.push_back(std::make_pair(std::string("a.dll"), std::string("")));
    SetHeader(kDash, "dlls", KeyMap(kDlls));
    const StepKey kBadDll = MakeStepKey(kDash);
    CHECK_FALSE(kBadDll.bValid);
    CHECK(kBadDll.kHeader[10] == "dlls\ta.dll=-");
    CHECK(InvalidKeyFields(kBadDll) == std::vector<std::string>(1, "dlls"));

    KeyInput kOverlay = PinnedInput();
    SetHeader(kOverlay, "overlay", "maybe");
    const StepKey kBadOverlay = MakeStepKey(kOverlay);
    CHECK_FALSE(kBadOverlay.bValid);
    CHECK(kBadOverlay.kHeader[7] == "overlay\t-");

    KeyInput kTlog = PinnedInput();
    SetHeader(kTlog, "tlog", "-");
    CHECK_FALSE(MakeStepKey(kTlog).bValid);
    CHECK(InvalidKeyFields(MakeStepKey(kTlog)) == std::vector<std::string>(1, "tlog"));

    KeyInput kEmpty = PinnedInput();
    SetHeader(kEmpty, "gamedata", "   ");
    CHECK_FALSE(MakeStepKey(kEmpty).bValid);

    KeyInput kNoFiles = PinnedInput();
    kNoFiles.kFiles.clear();
    const StepKey kBare = MakeStepKey(kNoFiles);
    CHECK_FALSE(kBare.bValid);
    CHECK(kBare.kHeader.back() == "files\t-");

    CHECK(KeyList(std::vector<std::string>()) == "-");
    std::vector<std::string> kList;
    kList.push_back("b");
    kList.push_back("");
    kList.push_back("a");
    CHECK(KeyList(kList) == "a,b");
    CHECK(KeyMap(KeyPairs()) == "-");
    CHECK(KeyScalar("  x  ") == "x");
}

TEST_CASE_NAMED(FGateKeysTest3, "Unit::Gate::tlog closure is read from UTF-16 text", "[EngineFilter]")
{
    const std::string sDir = TempDir("gatetlog");
    std::vector<std::string> kRel;
    kRel.push_back("a.cpp");
    kRel.push_back("b.h");
    kRel.push_back("CL.read.1.tlog");
    REQUIRE(WriteTextFile(JoinPath(sDir, "a.cpp"), "a"));
    REQUIRE(WriteTextFile(JoinPath(sDir, "b.h"), "b"));
    const std::string sUpper = UpperText(sDir);
    REQUIRE(WriteTextFile(JoinPath(sDir, "CL.read.1.tlog"),
                          Utf16File("^" + sUpper + "\\A.CPP\r\n" + sUpper + "\\A.CPP\r\n" + sUpper + "\\B.H\r\n" +
                                    "C:\\ELSEWHERE\\C.H\r\n")));
    ReadTlog kTlog;
    REQUIRE(LoadReadTlog(JoinPath(sDir, "CL.read.1.tlog"), sDir, kTlog));
    REQUIRE(kTlog.kSets.size() == 1);
    const std::set<std::string>& kSet = kTlog.kSets["a.cpp"];
    CHECK(kSet.size() == 2);
    CHECK(kSet.count("a.cpp") == 1);
    CHECK(kSet.count("b.h") == 1);
    CHECK(kTlog.kStale.empty());
    ReadTlog kMissing;
    CHECK_FALSE(LoadReadTlog(JoinPath(sDir, "none.tlog"), sDir, kMissing));
    RemoveFiles(sDir, kRel);
    RemoveDirectoryW(Utf8ToWide(sDir).c_str());
}

TEST_CASE_NAMED(FGateKeysTest4, "Unit::Gate::a changed compile command line changes the key", "[EngineFilter]")
{
    ReadTlog kRead;
    ParseReadTlog(kReadTlogText, kRoot, kRead);
    std::map<std::string, std::string> kCommands;
    ParseCommandTlog(kCommandTlogText, kRoot, kCommands);
    REQUIRE(kCommands.size() == 3);
    const std::map<std::string, std::string> kTree = FixtureTree();
    KeyFilesInput kIn;
    kIn.pTree = &kTree;
    kIn.kSources.push_back("tests\\unit\\movie_corpus.cpp");
    kIn.pRead = &kRead;
    kIn.pCommands = &kCommands;
    kIn.sDefinition = "d1";
    kIn.pHash = 0;
    std::map<std::string, std::string> kFiles;
    std::string sState;
    KeyFiles(kIn, kFiles, sState);
    CHECK(sState == "ok");
    KeyInput kKeyIn = PinnedInput();
    kKeyIn.kFiles = kFiles;
    const std::string sBase = MakeStepKey(kKeyIn).sRunKey;

    std::string sFlag = kCommandTlogText;
    sFlag.replace(sFlag.find("/D OPENSWBF2_TEST"), 17, "/D OPENSWBF2_TESX");
    std::map<std::string, std::string> kFlagCommands;
    ParseCommandTlog(sFlag, kRoot, kFlagCommands);
    kIn.pCommands = &kFlagCommands;
    KeyFiles(kIn, kFiles, sState);
    kKeyIn.kFiles = kFiles;
    CHECK(sState == "ok");
    CHECK(MakeStepKey(kKeyIn).sRunKey != sBase);

    std::string sOtherRoot = kCommandTlogText;
    for (size_t uAt = sOtherRoot.find("C:\\DEV\\OPENSWBF2"); uAt != std::string::npos;
         uAt = sOtherRoot.find("C:\\DEV\\OPENSWBF2", uAt + 1))
    {
        sOtherRoot.replace(uAt, 16, "D:\\WORK\\CHECKOUT2");
    }
    std::map<std::string, std::string> kOtherCommands;
    ParseCommandTlog(sOtherRoot, "D:\\Work\\Checkout2", kOtherCommands);
    CHECK(kOtherCommands == kCommands);

    std::string sMissing = kCommandTlogText;
    sMissing.erase(sMissing.find("^C:\\DEV\\OPENSWBF2\\TESTS\\HARNESS\\RUNNER.CPP"));
    std::map<std::string, std::string> kMissingCommands;
    ParseCommandTlog(sMissing, kRoot, kMissingCommands);
    kIn.pCommands = &kMissingCommands;
    KeyFiles(kIn, kFiles, sState);
    CHECK(sState == "-");
    kIn.pCommands = 0;
    KeyFiles(kIn, kFiles, sState);
    CHECK(sState == "-");
    SetHeader(kKeyIn, "tlog", sState);
    CHECK_FALSE(MakeStepKey(kKeyIn).bValid);
}

TEST_CASE_NAMED(FGateKeysTest5, "Unit::Gate::source key files", "[EngineFilter]")
{
    ReadTlog kRead;
    ParseReadTlog(kReadTlogText, kRoot, kRead);
    MarkStaleUnits(kRead, kRoot, 200, OldTimes);
    std::map<std::string, std::string> kCommands;
    ParseCommandTlog(kCommandTlogText, kRoot, kCommands);
    const std::map<std::string, std::string> kTree = FixtureTree();
    KeyFilesInput kIn;
    kIn.pTree = &kTree;
    kIn.kReads.push_back("tests\\excluded.tsv");
    kIn.kSources.push_back("src\\movie\\");
    kIn.kSources.push_back("tests\\unit\\movie_corpus.cpp");
    kIn.pRead = &kRead;
    kIn.pCommands = &kCommands;
    kIn.sDefinition = "d1";
    kIn.pHash = 0;
    std::map<std::string, std::string> kFiles;
    std::string sState;
    KeyFiles(kIn, kFiles, sState);
    CHECK(sState == "ok");

    SUBCASE("a TU outside the sources is not a key file")
    {
        CHECK(kFiles.count("tests\\unit\\other.cpp") == 0);
        CHECK(kFiles.count("src\\common\\unrelated.h") == 0);
        CHECK(kFiles.count("cmd:tests\\unit\\other.cpp") == 0);
    }
    SUBCASE("a listed source is")
    {
        CHECK(kFiles["src\\movie\\dec.c"] == "dec1");
        CHECK(kFiles["tests\\unit\\movie_corpus.cpp"] == "corpus1");
        CHECK(kFiles["cmd:tests\\unit\\movie_corpus.cpp"] == kCommands["tests\\unit\\movie_corpus.cpp"]);
        CHECK(kFiles["tests\\excluded.tsv"] == "excluded1");
        CHECK(kFiles["gate-step-definition"] == "d1");
    }
    SUBCASE("a header in the closure is")
    {
        CHECK(kFiles["src\\common\\shared.h"] == "shared1");
        CHECK(kFiles["tests\\harness\\runner.h"] == "runnerh1");
    }
    SUBCASE("a closure header's same-named cpp under tests is")
    {
        CHECK(kFiles["tests\\harness\\runner.cpp"] == "runnercpp1");
        CHECK(kFiles["tests\\harness\\report.h"] == "reporth1");
        CHECK(kFiles["cmd:tests\\harness\\runner.cpp"] == kCommands["tests\\harness\\runner.cpp"]);
        CHECK(kFiles.size() == 10);
    }
    SUBCASE("a stale tlog makes the key invalid")
    {
        ReadTlog kStale;
        ParseReadTlog(kReadTlogText, kRoot, kStale);
        MarkStaleUnits(kStale, kRoot, 200, SharedNewer);
        CHECK(kStale.kStale.count("tests\\unit\\movie_corpus.cpp") == 1);
        CHECK(kStale.kStale.count("tests\\unit\\other.cpp") == 0);
        kIn.pRead = &kStale;
        KeyFiles(kIn, kFiles, sState);
        CHECK(sState == "-");
    }
    SUBCASE("a missing tlog makes the key invalid")
    {
        kIn.pRead = 0;
        KeyFiles(kIn, kFiles, sState);
        CHECK(sState == "-");
        CHECK(kFiles["src\\movie\\dec.c"] == "dec1");
    }
    SUBCASE("a step with no sources keys its reads only")
    {
        kIn.kSources.clear();
        KeyFiles(kIn, kFiles, sState);
        CHECK(sState == "none");
        CHECK(kFiles.size() == 2);
    }
}

TEST_CASE_NAMED(FGateKeysTest6, "Unit::Gate::input matching", "[EngineFilter]")
{
    std::vector<std::string> kIn;
    kIn.push_back("tests\\harness\\");
    kIn.push_back("*.vcxproj");
    kIn.push_back("tests\\testutil.*");
    kIn.push_back("tests\\excluded.tsv");
    kIn.push_back("lib\\vendor?pins.props");
    CHECK(InputMatches("tests\\harness\\runner.cpp", kIn));
    CHECK(InputMatches("TESTS\\HARNESS\\RUNNER.CPP", kIn));
    CHECK_FALSE(InputMatches("tests\\harnessx\\runner.cpp", kIn));
    CHECK(InputMatches("BattlefrontII.vcxproj", kIn));
    CHECK(InputMatches("tests\\OpenSWBF2Tests.VCXPROJ", kIn));
    CHECK_FALSE(InputMatches("BattlefrontII.vcxproj.filters", kIn));
    CHECK(InputMatches("tests\\testutil.h", kIn));
    CHECK(InputMatches("Tests\\Excluded.TSV", kIn));
    CHECK_FALSE(InputMatches("tests\\excluded.tsv.bak", kIn));
    CHECK_FALSE(InputMatches("lib\\vendor-pins.props", kIn));
    CHECK(InputMatches("lib\\vendor?pins.props", kIn));
    CHECK(LikeMatch("a7.txt", "a[0-9].*"));
    CHECK_FALSE(LikeMatch("ab.txt", "a[0-9].*"));
}

TEST_CASE_NAMED(FGateKeysTest7, "Unit::Gate::records the script wrote are read and kept", "[EngineFilter]")
{
    const std::string sHeader = "Step\tRunKey\tJudgeKey\tPlatform\tConfig\tEvidence\tResult\tWall_s\tTree\tCommit\tDate";
    const std::string sRow1 = "all-Release-short\t8e3ef6b703402285ec788b81f7570556bba7fb326a9d784608e9493148f85aaf\t"
                              "7035c77d381adb6652e94ab36526faa0403d105407fda2f820939063f0e21c4b\tWin32\tRelease\t"
                              "720b35c4b15f5b97d54d7c9c22af340cdff504c15be38f5deea43eaca14a7dcb\tPASS\t159\t"
                              "951391b140f90e833144ec351121ada5ff2a13bc\taf34d34def5e2680d7df0f7625fba41e24bb32ad\t"
                              "2026-11-25T12:46:27";
    const std::string sRow2 = "all-Release-short\t14dd144c2034248a2cbace5dd729437f850540f86dbf6ca2ccbf51aa2b1e4c2a\t"
                              "7035c77d381adb6652e94ab36526faa0403d105407fda2f820939063f0e21c4b\tx64\tRelease\t"
                              "b5b912b32c59fe22862fb70fe29a4166b6c11e8e4ebadb10c7d8030bf0bf7ff0\tPASS\t155\t"
                              "951391b140f90e833144ec351121ada5ff2a13bc\taf34d34def5e2680d7df0f7625fba41e24bb32ad\t"
                              "2026-11-25T12:47:45";
    const std::string sOld = sHeader + "\r\n" + sRow1 + "\r\n" + sRow2 + "\r\n";
    const std::vector<std::string> kColumns = ColumnList(kStepColumns, kStepColumnCount);
    const std::vector<std::string> kKeys = ColumnList(kStepKeyColumns, kStepKeyColumnCount);
    CHECK(JoinText(kColumns, "\t") == sHeader);

    const RecordMap kRecords = ReadKeyedRecords(sOld, kKeys);
    REQUIRE(kRecords.size() == 2);
    const RecordRow& kRow = kRecords.find("all-Release-short\tWin32\tRelease")->second;
    CHECK(kRow.find("Wall_s")->second == "159");
    CHECK(RecordUsable(&kRow));
    CHECK(FormatRecord(kRow, kColumns) == sRow1);

    const std::string sNew = "build-Debug\tr\tj\tWin32\tDebug\te\tPASS\t1\tt\tc\t2026-11-25T13:00:00";
    const std::string sMerged =
        MergeKeyedRecords(sOld, kColumns, kKeys, std::vector<std::string>(1, sNew), std::vector<std::string>());
    CHECK(sMerged == sHeader + "\r\n" + sRow1 + "\r\n" + sRow2 + "\r\n" + sNew + "\r\n");
    const std::string sDropped = MergeKeyedRecords(sOld, kColumns, kKeys, std::vector<std::string>(),
                                                   std::vector<std::string>(1, "all-Release-short\tx64\tRelease"));
    CHECK(sDropped == sHeader + "\r\n" + sRow1 + "\r\n");

    StepKey kKey;
    kKey.bValid = true;
    kKey.sPlatform = "Win32";
    kKey.sRunKey = "8e3ef6b703402285ec788b81f7570556bba7fb326a9d784608e9493148f85aaf";
    CHECK(RecordMatches(&kRow, kKey));
    kKey.sPlatform = "win32";
    CHECK_FALSE(RecordMatches(&kRow, kKey));
    CHECK_FALSE(RecordMatches(0, kKey));

    const std::string sDir = TempDir("gaterecords");
    const std::string sLedger = JoinPath(sDir, "ledger.tsv");
    const std::string sLedgerOld = "Step\tPlatform\tConfig\tTier\tRunKey\tJudgeKey\tDecision\tResult\tEvidence\tWall_s\t"
                                   "Start\tEnd\r\n"
                                   "build-Debug\tWin32\tDebug\tplan\t"
                                   "4817d339ad8fca449382b552c902bc002963323dbc33460d7c1276f52950dc57\t"
                                   "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\tRUN\tPASS\t-\t73\t"
                                   "2026-11-24T23:06:54\t2026-11-24T23:08:07\r\n";
    REQUIRE(WriteTextFile(sLedger, sLedgerOld));
    const std::string sLine = "t1-Debug\tWin32\tDebug\tplan\tr\tj\tCARRY\tPASS\te\t0\ts\tf";
    REQUIRE(AppendLedgerLine(sLedger, sLine));
    std::string sLedgerNew;
    REQUIRE(ReadTextFile(sLedger, sLedgerNew));
    CHECK(sLedgerNew == sLedgerOld + sLine + "\r\n");
    std::vector<std::string> kLedgerKeys;
    kLedgerKeys.push_back("Step");
    kLedgerKeys.push_back("Platform");
    CHECK(ReadKeyedRecords(sLedgerNew, kLedgerKeys).size() == 2);

    const std::string sSteps = JoinPath(sDir, "steps.tsv");
    REQUIRE(WriteTextFile(sSteps, sOld));
    REQUIRE(MergeStepRecordsFile(sSteps, std::vector<std::string>(1, sNew), std::vector<std::string>()));
    std::string sStepsNew;
    REQUIRE(ReadTextFile(sSteps, sStepsNew));
    CHECK(sStepsNew == sMerged);
    CHECK(ReadStepRecordsFile(sSteps).size() == 3);
    std::vector<std::string> kRel;
    kRel.push_back("ledger.tsv");
    kRel.push_back("steps.tsv");
    RemoveFiles(sDir, kRel);
    RemoveDirectoryW(Utf8ToWide(sDir).c_str());
}

TEST_CASE_NAMED(FGateKeysTest8, "Unit::Gate::read list check", "[EngineFilter]")
{
    const std::map<std::string, std::string> kTree = FixtureTree();
    ReadCheckInput kIn;
    kIn.sStep = "standard-Release";
    kIn.sRoot = kRoot;
    kIn.kReads.push_back("tests\\smoke\\");
    kIn.kClosure.insert("tests\\harness\\runner.h");
    kIn.pTree = &kTree;
    kIn.kArgs.push_back("--runner");

    SUBCASE("an unlisted tracked path fails")
    {
        kIn.sReadsText = "C:\\Dev\\OpenSWBF2\\src\\common\\shared.h\r\nC:\\Dev\\OpenSWBF2\\src\\common\\shared.h\r\n";
        const std::vector<std::string> kFails = ReadCheck(kIn);
        REQUIRE(kFails.size() == 1);
        CHECK(kFails[0] == "harness: standard-Release read src\\common\\shared.h, which is not in its Reads");
    }
    SUBCASE("a listed path passes")
    {
        kIn.kReads.push_back("src\\common\\");
        kIn.sReadsText = "c:/dev/openswbf2/SRC/Common/Shared.h\r\n";
        CHECK(ReadCheck(kIn).empty());
    }
    SUBCASE("a path in the Sources closure passes")
    {
        kIn.sReadsText = "C:\\Dev\\OpenSWBF2\\tests\\harness\\runner.h\r\n";
        CHECK(ReadCheck(kIn).empty());
    }
    SUBCASE("a path that names no tracked file is ignored")
    {
        kIn.sReadsText = "C:\\Dev\\OpenSWBF2\\bin\\Win32\\Release\\BattlefrontII.exe\r\nD:\\elsewhere\\x.txt\r\n";
        CHECK(ReadCheck(kIn).empty());
    }
    SUBCASE("an exclude argument's file counts as read")
    {
        kIn.kArgs.push_back("--exclude=C:\\Dev\\OpenSWBF2\\tests\\excluded.tsv");
        const std::vector<std::string> kFails = ReadCheck(kIn);
        REQUIRE(kFails.size() == 1);
        CHECK(kFails[0] == "harness: standard-Release read tests\\excluded.tsv, which is not in its Reads");
        kIn.kReads.push_back("tests\\excluded.tsv");
        CHECK(ReadCheck(kIn).empty());
    }
}

TEST_CASE_NAMED(FGateKeysTest9, "Unit::Gate::a read is noted only when the variable is set", "[EngineFilter]")
{
    wchar_t acOld[MAX_PATH * 4];
    const DWORD uOld = GetEnvironmentVariableW(L"OPENSWBF2_READS", acOld, MAX_PATH * 4);
    const std::string sDir = TempDir("gatereads");
    const std::string sList = JoinPath(sDir, "reads.txt");
    SetEnvironmentVariableW(L"OPENSWBF2_READS", NULL);
    ReadListNote("C:\\Dev\\OpenSWBF2\\tests\\excluded.tsv");
    CHECK_FALSE(FileExists(sList));
    SetEnvironmentVariableW(L"OPENSWBF2_READS", Utf8ToWide(sList).c_str());
    ReadListNote("C:\\Dev\\OpenSWBF2\\tests\\excluded.tsv");
    ReadListNote("C:\\Dev\\OpenSWBF2\\tests\\smoke\\a.txt");
    std::string sText;
    CHECK(ReadTextFile(sList, sText));
    CHECK(sText == "C:\\Dev\\OpenSWBF2\\tests\\excluded.tsv\r\nC:\\Dev\\OpenSWBF2\\tests\\smoke\\a.txt\r\n");
    SetEnvironmentVariableW(L"OPENSWBF2_READS", uOld > 0 && uOld < MAX_PATH * 4 ? acOld : NULL);
    DeleteFileW(Utf8ToWide(sList).c_str());
    RemoveDirectoryW(Utf8ToWide(sDir).c_str());
}
