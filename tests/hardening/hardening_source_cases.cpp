#include "harness/tags.h"

#include <string>
#include <vector>

#include "hardening/hardening_source.h"

namespace
{

bool SourceRows(std::vector<std::string>& kParams, std::string& sError)
{
    kParams = Hardening::SourceRowNames();
    if (kParams.empty())
    {
        sError = "no hardening source rows";
        return false;
    }
    return true;
}

}

COMPLEX_TEST_CASE_NAMED(FHardeningSourceTest, "Hardening::Source", "[EngineFilter][HardeningSource][OnRequest]", SourceRows)
{
    const std::string sRow = RunnerParam();
    std::vector<std::string> kHits;
    std::string sError;
    const bool bRan = Hardening::RunSourceRow(sRow, kHits, sError);
    REQUIRE_MESSAGE(bRan, "[FAIL] hardening:" << sRow << " - " << sError);
    for (size_t i = 0; i < kHits.size(); ++i)
    {
        CHECK_MESSAGE(false, "[FAIL] hardening:" << sRow << " - " << kHits[i]);
    }
}
