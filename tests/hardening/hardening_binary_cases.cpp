#include "harness/tags.h"

#include <string>
#include <vector>

#include "hardening/hardening_binary.h"

namespace
{

bool BinaryRows(std::vector<std::string>& kParams, std::string& sError)
{
    kParams = Hardening::BinaryRowNames(RunnerPlatform());
    if (kParams.empty())
    {
        sError = "no hardening binary rows";
        return false;
    }
    return true;
}

bool FFmpegRows(std::vector<std::string>& kParams, std::string& sError)
{
    kParams = Hardening::FFmpegRowNames();
    if (kParams.empty())
    {
        sError = "no hardening FFmpeg rows";
        return false;
    }
    return true;
}

void Judge(const std::string& sRow, bool bRan, const Hardening::BinaryRow& kRow, const std::string& sError)
{
    REQUIRE_MESSAGE(bRan, sError);
    if (kRow.eStatus == Hardening::kRowSkip)
    {
        SkipCase(kRow.sDetail.c_str());
    }
    CHECK_MESSAGE(kRow.eStatus == Hardening::kRowPass, "[FAIL] hardening:" << sRow << " - " << kRow.sDetail);
}

}

COMPLEX_TEST_CASE_NAMED(FHardeningBinaryTest, "Hardening::Binary", "[EngineFilter][HardeningBinary][OnRequest]", BinaryRows)
{
    const std::string sRow = RunnerParam();
    Hardening::BinaryRow kRow;
    kRow.eStatus = Hardening::kRowFail;
    std::string sError;
    const bool bRan = Hardening::RunBinaryRow(sRow, RunnerPlatform(), RunnerConfig(), kRow, sError);
    Judge(sRow, bRan, kRow, sError);
}

COMPLEX_TEST_CASE_NAMED(FHardeningFFmpegTest, "Hardening::FFmpeg", "[EngineFilter][HardeningBinary][OnRequest]", FFmpegRows)
{
    const std::string sRow = RunnerParam();
    Hardening::BinaryRow kRow;
    kRow.eStatus = Hardening::kRowFail;
    std::string sError;
    const bool bRan = Hardening::RunFFmpegRow(sRow, RunnerPlatform(), RunnerConfig(), kRow, sError);
    Judge(sRow, bRan, kRow, sError);
}
