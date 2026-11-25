#pragma once
#include <string>
#include <vector>

struct ExcludeRow
{
    std::string sTest;
    std::string sReason;
    std::vector<std::string> kPlatforms;
    std::vector<std::string> kConfigs;
};

bool ParseExcludeList(const std::string& sText, const std::vector<std::string>& kRegistered,
                      const std::string& sPlatform, const std::string& sConfig, std::vector<ExcludeRow>& kRows,
                      std::string& sError);
const ExcludeRow* FindExclusion(const std::vector<ExcludeRow>& kRows, const std::string& sTest,
                                const std::string& sPlatform, const std::string& sConfig);
