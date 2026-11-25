#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/excludelist.h"

namespace
{

const char* const kHeader = "Test\tReason\tPlatforms\tConfigs\n";

std::vector<std::string> Registered()
{
    std::vector<std::string> k;
    k.push_back("Unit::A::one");
    k.push_back("Unit::A::two");
    return k;
}

bool Parse(const std::string& sText, std::vector<ExcludeRow>& kRows, std::string& sError,
           const std::string& sPlatform = "x64", const std::string& sConfig = "Debug")
{
    return ParseExcludeList(sText, Registered(), sPlatform, sConfig, kRows, sError);
}

}

TEST_CASE_NAMED(FHarnessExcludelistTest1, "Unit::HarnessExcludelist::a row applies to its platforms and configurations",
                "[SmokeFilter]")
{
    std::vector<ExcludeRow> kRows;
    std::string sError;
    REQUIRE(Parse(std::string(kHeader) + "Unit::A::one\tx87 only\tx64\t\r\nUnit::A::two\tslow\t\tDebug,Asan\n", kRows,
                  sError));
    CHECK(kRows.size() == 2);
    REQUIRE(FindExclusion(kRows, "Unit::A::one", "x64", "Test"));
    CHECK(FindExclusion(kRows, "Unit::A::one", "x64", "Test")->sReason == "x87 only");
    CHECK(!FindExclusion(kRows, "Unit::A::one", "Win32", "Test"));
    CHECK(FindExclusion(kRows, "Unit::A::two", "Win32", "Asan"));
    CHECK(FindExclusion(kRows, "Unit::A::two", "x64", "Debug"));
    CHECK(!FindExclusion(kRows, "Unit::A::two", "x64", "Test"));
    CHECK(!FindExclusion(kRows, "Unit::A::three", "x64", "Test"));
}

TEST_CASE_NAMED(FHarnessExcludelistTest2, "Unit::HarnessExcludelist::a row without a reason is refused", "[SmokeFilter]")
{
    std::vector<ExcludeRow> kRows;
    std::string sError;
    CHECK(!Parse(std::string(kHeader) + "Unit::A::one\t\tx64\t\n", kRows, sError));
    CHECK(sError.find("has no Reason") != std::string::npos);
}

TEST_CASE_NAMED(FHarnessExcludelistTest3, "Unit::HarnessExcludelist::a row naming no registered test is refused",
                "[SmokeFilter]")
{
    std::vector<ExcludeRow> kRows;
    std::string sError;
    CHECK(!Parse(std::string(kHeader) + "Unit::A::gone\tstale\t\t\n", kRows, sError));
    CHECK(sError.find("not a registered test") != std::string::npos);
}

TEST_CASE_NAMED(FHarnessExcludelistTest4, "Unit::HarnessExcludelist::a bad header or cell count is refused", "[SmokeFilter]")
{
    std::vector<ExcludeRow> kRows;
    std::string sError;
    CHECK(!Parse("Test\tReason\tPlatforms\n", kRows, sError));
    CHECK(!Parse(std::string(kHeader) + "Unit::A::one\treason\tx64\n", kRows, sError));
    CHECK(Parse(kHeader, kRows, sError));
    CHECK(kRows.empty());
}

TEST_CASE_NAMED(FHarnessExcludelistTest5,
                "Unit::HarnessExcludelist::a row is checked for its test only on the platforms and configurations it lists",
                "[SmokeFilter]")
{
    std::vector<ExcludeRow> kRows;
    std::string sError;
    const std::string sText = std::string(kHeader) + "Unit::A::x64only\tbuilt on x64 only\tx64\t\n" +
                              "Unit::A::asanonly\tbuilt under Asan only\t\tAsan\n";
    CHECK(Parse(sText, kRows, sError, "Win32", "Debug"));
    CHECK(kRows.size() == 2);
    CHECK(!FindExclusion(kRows, "Unit::A::x64only", "Win32", "Debug"));
    CHECK(!Parse(sText, kRows, sError, "x64", "Debug"));
    CHECK(sError.find("Unit::A::x64only is not a registered test") != std::string::npos);
    CHECK(!Parse(sText, kRows, sError, "Win32", "Asan"));
    CHECK(sError.find("Unit::A::asanonly is not a registered test") != std::string::npos);
}
