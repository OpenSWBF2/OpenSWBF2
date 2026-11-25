#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/runner.h"

namespace
{

const char* const kOnePassed = "[doctest] test cases:      1 |      1 passed |      0 failed |      0 skipped";
const char* const kPoisonReport =
    "=================================================================\n"
    "==4242==ERROR: AddressSanitizer: use-after-poison on address 0x12345678 at pc 0x00401000 bp 0x0019f000 sp 0x0019eff0\n"
    "WRITE of size 1 at 0x12345678 thread T0\n"
    "    #0 0x00401000 in DOCTEST_ANON_FUNC_6 C:\\Dev\\OpenSWBF2\\tests\\unit\\asan_canary.cpp:55\n"
    "    #1 0x00402000 in doctest::Context::run C:\\Dev\\OpenSWBF2\\tests\\third_party\\doctest\\doctest.h:5300\n";
const char* const kOverflowReport =
    "==4242==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x12345678 at pc 0x00401000 bp 0x0019f000 sp 0x0019eff0\n"
    "WRITE of size 1 at 0x12345678 thread T0\n"
    "    #0 0x00401000 in DOCTEST_ANON_FUNC_2 C:\\Dev\\OpenSWBF2\\tests\\unit\\asan_canary.cpp:40\n";
const char* const kOtherFrameReport =
    "==4242==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x12345678 at pc 0x00401000 bp 0x0019f000 sp 0x0019eff0\n"
    "READ of size 4 at 0x12345678 thread T0\n"
    "    #0 0x00405000 in RedFreeToHeap C:\\Dev\\OpenSWBF2\\src\\RedEngineFL\\Memory\\RedMemory.cpp:900\n"
    "    #1 0x00401000 in DOCTEST_ANON_FUNC_2 C:\\Dev\\OpenSWBF2\\tests\\unit\\asan_canary.cpp:40\n";
const char* const kCanaryFile = "C:\\Dev\\OpenSWBF2\\tests\\unit\\asan_canary.cpp";

ChildResult Child(unsigned long uCode, const char* pcSummary = kOnePassed)
{
    ChildResult k;
    k.uCode = uCode;
    k.uPid = 0;
    k.bStalled = false;
    k.bKilled = false;
    k.sSummary = pcSummary;
    k.bAsanReport = false;
    k.bHasProgress = false;
    k.uTurns = 0;
    k.uSteps = 0;
    return k;
}

RunnerCase Case(const char* pcName, const char* pcTags)
{
    RunnerCase k;
    k.sName = pcName;
    k.sTags = pcTags;
    return k;
}

bool Refused(const RunnerCase& kBad)
{
    std::vector<RunnerCase> kCases;
    kCases.push_back(Case("Unit::A::one", "[SmokeFilter]"));
    kCases.push_back(kBad);
    std::string sError;
    const bool bOk = CheckCases(kCases, sError);
    return !bOk && !sError.empty();
}

}

TEST_CASE_NAMED(FHarnessRunnerTest1, "Unit::HarnessRunner::the verdict follows the child exit code", "[SmokeFilter]")
{
    CHECK(JudgeCase(Child(0), false, 0).eVerdict == kVerdictPass);
    CHECK(JudgeCase(Child(1), false, 0).eVerdict == kVerdictFail);
    CHECK(JudgeCase(Child(77), false, 0).eVerdict == kVerdictSkip);
    CHECK(JudgeCase(Child(124), false, 0).eVerdict == kVerdictStall);
    CHECK(JudgeCase(Child(3), false, 0).eVerdict == kVerdictHarness);
    CHECK(JudgeCase(Child(2), false, 0).eVerdict == kVerdictFail);
    CHECK(JudgeCase(Child(66), false, 0).eVerdict == kVerdictFail);
    const Judgement kCrash = JudgeCase(Child(0xC0000005ul), false, 0);
    CHECK(kCrash.eVerdict == kVerdictCrash);
    CHECK(kCrash.sDetail == "0xC0000005");
    CHECK(JudgeCase(Child(0xE0000001ul), false, 0).eVerdict == kVerdictCrash);
}

TEST_CASE_NAMED(FHarnessRunnerTest2, "Unit::HarnessRunner::exit 0 passes only when the case ran exactly once", "[SmokeFilter]")
{
    CHECK(JudgeCase(Child(0, "[doctest] test cases:      0 |      0 passed |      0 failed |      1 skipped"), false, 0)
              .eVerdict == kVerdictFail);
    CHECK(JudgeCase(Child(0, "[doctest] test cases:      2 |      2 passed |      0 failed |      0 skipped"), false, 0)
              .eVerdict == kVerdictFail);
    CHECK(JudgeCase(Child(0, ""), false, 0).eVerdict == kVerdictFail);
}

TEST_CASE_NAMED(FHarnessRunnerTest3, "Unit::HarnessRunner::a stall killed by the runner is a stall whatever the exit code",
                "[SmokeFilter]")
{
    ChildResult k = Child(1);
    k.bStalled = true;
    k.bKilled = true;
    CHECK(JudgeCase(k, false, 0).eVerdict == kVerdictStall);
}

TEST_CASE_NAMED(FHarnessRunnerTest4, "Unit::HarnessRunner::an asan report fails a passing case", "[SmokeFilter]")
{
    ChildResult k = Child(0);
    k.bAsanReport = true;
    const Judgement kJudge = JudgeCase(k, true, 0);
    CHECK(kJudge.eVerdict == kVerdictFail);
    CHECK(kJudge.sDetail == "asan");
    CHECK(JudgeCase(Child(0), true, 0).eVerdict == kVerdictPass);
}

TEST_CASE_NAMED(FHarnessRunnerTest5, "Unit::HarnessRunner::asan exit 66 without a report is a start-up failure", "[SmokeFilter]")
{
    const Judgement kJudge = JudgeCase(Child(66, ""), true, 0);
    CHECK(kJudge.eVerdict == kVerdictFail);
    CHECK(kJudge.sDetail.find("Asan runtime failed before main") != std::string::npos);
}

TEST_CASE_NAMED(FHarnessRunnerTest6, "Unit::HarnessRunner::a canary passes only on an asan run with its report", "[SmokeFilter]")
{
    CanaryExpect kCanary;
    kCanary.sKind = "use-after-poison";
    kCanary.sFile = kCanaryFile;
    ChildResult kReported = Child(66, "");
    kReported.bAsanReport = true;
    kReported.sAsanReport = kPoisonReport;
    CHECK(JudgeCase(kReported, true, &kCanary).eVerdict == kVerdictPass);
    CHECK(JudgeCase(Child(0), true, &kCanary).eVerdict == kVerdictFail);
    CHECK(JudgeCase(Child(1), true, &kCanary).eVerdict == kVerdictFail);
    CHECK(JudgeCase(kReported, false, &kCanary).eVerdict == kVerdictFail);
}

TEST_CASE_NAMED(FHarnessRunnerTest9,
                "Unit::HarnessRunner::a canary fails on an asan report of another kind or frame", "[SmokeFilter]")
{
    CanaryExpect kCanary;
    kCanary.sKind = "heap-buffer-overflow";
    kCanary.sFile = kCanaryFile;
    ChildResult kChild = Child(66, "");
    kChild.bAsanReport = true;
    kChild.sAsanReport = kOverflowReport;
    CHECK(JudgeCase(kChild, true, &kCanary).eVerdict == kVerdictPass);
    kChild.sAsanReport = kPoisonReport;
    const Judgement kWrongKind = JudgeCase(kChild, true, &kCanary);
    CHECK(kWrongKind.eVerdict == kVerdictFail);
    CHECK(kWrongKind.sDetail.find("use-after-poison") != std::string::npos);
    kChild.sAsanReport = kOtherFrameReport;
    const Judgement kWrongFrame = JudgeCase(kChild, true, &kCanary);
    CHECK(kWrongFrame.eVerdict == kVerdictFail);
    CHECK(kWrongFrame.sDetail.find("frame #0") != std::string::npos);
    kChild.sAsanReport = kOverflowReport;
    kCanary.sKind = "";
    CHECK(JudgeCase(kChild, true, &kCanary).eVerdict == kVerdictFail);
}

TEST_CASE_NAMED(FHarnessRunnerTest7, "Unit::HarnessRunner::the case list refuses bad tags and names", "[SmokeFilter]")
{
    std::vector<RunnerCase> kGood;
    kGood.push_back(Case("Unit::A::one", "[SmokeFilter]"));
    kGood.push_back(Case("Unit::A::two", "[StressFilter][OnRequest]"));
    kGood.push_back(Case("Unit::A::three", "[NegativeFilter][AsanCanary]"));
    std::string sError;
    CHECK(CheckCases(kGood, sError));
    CHECK(Refused(Case("Unit::A::two filters", "[SmokeFilter][EngineFilter]")));
    CHECK(Refused(Case("Unit::A::no filter", "[OnRequest]")));
    CHECK(Refused(Case("Unit::A::no tags", "")));
    CHECK(Refused(Case("Unit::A::bad tags", "SmokeFilter")));
    CHECK(Refused(Case("Unit::A::open tag", "[SmokeFilter")));
    CHECK(Refused(Case("Unit::A::ONE", "[SmokeFilter]")));
    CHECK(Refused(Case("Unit::A::one, two", "[SmokeFilter]")));
    CHECK(Refused(Case("Unit::A::star*", "[SmokeFilter]")));
    CHECK(Refused(Case("Unit::A::what?", "[SmokeFilter]")));
}

TEST_CASE_NAMED(FHarnessRunnerTest8, "Unit::HarnessRunner::on request cases run only when the tag filter names them",
                "[SmokeFilter]")
{
    const RunnerCase kStress = Case("Unit::A::soak", "[StressFilter][OnRequest]");
    const RunnerCase kSmoke = Case("Unit::A::quick", "[SmokeFilter]");
    CHECK(!IsSelected(kStress, "", ""));
    CHECK(!IsSelected(kStress, "*[StressFilter]*", ""));
    CHECK(IsSelected(kStress, "*[StressFilter][OnRequest]*", ""));
    CHECK(IsSelected(kSmoke, "", ""));
    CHECK(IsSelected(kSmoke, "*[smokefilter]*", "Unit::A::*"));
    CHECK(!IsSelected(kSmoke, "*[EngineFilter]*", ""));
    CHECK(!IsSelected(kSmoke, "", "Unit::B::*"));
    CHECK(IsSelected(kSmoke, "", "Unit::B::*,Unit::A::quick"));
}

TEST_CASE_NAMED(FHarnessRunnerTest9, "Unit::HarnessRunner::child arguments are quoted for the command line", "[SmokeFilter]")
{
    CHECK(QuoteArg("-tc=a b") == "\"-tc=a b\"");
    CHECK(QuoteArg("a\"b") == "\"a\\\"b\"");
    CHECK(QuoteArg("c:\\x\\") == "\"c:\\x\\\\\"");
}

TEST_CASE_NAMED(FHarnessRunnerTest10, "Unit::HarnessRunner::game run cases run only with a game folder", "[SmokeFilter]")
{
    const RunnerCase kBoot = Case("Game::Boot::boot-shell", "[ProductFilter][Boot]");
    const RunnerCase kCorpus = Case("Unit::A::corpus", "[ProductFilter]");
    CHECK(IsGameRunCase(kBoot));
    CHECK(IsGameRunCase(Case("Game::SaveCross::save-cross-load", "[ProductFilter][SaveCross]")));
    CHECK(!IsGameRunCase(kCorpus));
    CHECK(!IsSelected(kBoot, "", ""));
    CHECK(!IsSelected(kBoot, "*[Boot]*", ""));
    CHECK(IsSelected(kBoot, "*[Boot]*", "", true));
    CHECK(!IsSelected(kBoot, "*[Standard]*", "", true));
    CHECK(IsSelected(kCorpus, "", ""));
    CHECK(!IsSelected(kCorpus, "", "", true));
}

namespace
{

bool TwoMissions(std::vector<std::string>& kParams, std::string& sError)
{
    kParams.push_back("geo1c_con");
    kParams.push_back("nab2g_c");
    return true;
}

bool UnsafeMission(std::vector<std::string>& kParams, std::string& sError)
{
    kParams.push_back("geo1c_con -crashtest nullderef");
    return true;
}

bool NoMission(std::vector<std::string>& kParams, std::string& sError)
{
    return true;
}

bool BrokenFile(std::vector<std::string>& kParams, std::string& sError)
{
    sError = "missions line 2 is blank";
    return false;
}

bool Expanded(ComplexTestsFn pfn, std::vector<RunnerCase>& kOut, std::string& sError)
{
    std::vector<RunnerCase> kDeclared;
    kDeclared.push_back(Case("Game::All", "[ProductFilter][All]"));
    kDeclared.push_back(Case("Unit::A::one", "[SmokeFilter]"));
    std::vector<ComplexTest> kComplex;
    ComplexTest k = {"Game::All", pfn};
    kComplex.push_back(k);
    return ExpandCases(kDeclared, kComplex, kOut, sError);
}

}

TEST_CASE_NAMED(FHarnessRunnerTest11, "Unit::HarnessRunner::a complex test expands into one child per listed name", "[SmokeFilter]")
{
    std::vector<RunnerCase> kOut;
    std::string sError;
    REQUIRE(Expanded(TwoMissions, kOut, sError));
    REQUIRE(kOut.size() == 3);
    CHECK(kOut[0].sName == "Game::All::geo1c_con");
    CHECK(kOut[0].sTestName == "Game::All");
    CHECK(kOut[0].sParam == "geo1c_con");
    CHECK(kOut[0].sTags == "[ProductFilter][All]");
    CHECK(kOut[1].sName == "Game::All::nab2g_c");
    CHECK(kOut[2].sName == "Unit::A::one");
    CHECK(kOut[2].sTestName == "Unit::A::one");
    CHECK(kOut[2].sParam.empty());
    CHECK(CheckCases(kOut, sError));
    CHECK(!Expanded(UnsafeMission, kOut, sError));
    CHECK(sError.find("letters, digits") != std::string::npos);
    CHECK(!Expanded(NoMission, kOut, sError));
    CHECK(!Expanded(BrokenFile, kOut, sError));
    CHECK(sError.find("blank") != std::string::npos);
    std::vector<RunnerCase> kNoCase(1, Case("Unit::A::one", "[SmokeFilter]"));
    std::vector<ComplexTest> kComplex(1);
    kComplex[0].pcName = "Game::All";
    kComplex[0].pfnGetTests = TwoMissions;
    CHECK(!ExpandCases(kNoCase, kComplex, kOut, sError));
}
