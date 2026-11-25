#include "harness/excludelist.h"

#include <algorithm>
#include <stdio.h>

static std::vector<std::string> Split(const std::string& sText, char cSep)
{
    std::vector<std::string> kParts;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uAt = sText.find(cSep, uStart);
        kParts.push_back(sText.substr(uStart, uAt == std::string::npos ? std::string::npos : uAt - uStart));
        if (uAt == std::string::npos)
        {
            return kParts;
        }
        uStart = uAt + 1;
    }
}

static std::vector<std::string> SplitList(const std::string& sCell)
{
    std::vector<std::string> kItems;
    if (!sCell.empty())
    {
        kItems = Split(sCell, ',');
    }
    return kItems;
}

static bool Applies(const std::vector<std::string>& kList, const std::string& sValue)
{
    return kList.empty() || std::find(kList.begin(), kList.end(), sValue) != kList.end();
}

bool ParseExcludeList(const std::string& sText, const std::vector<std::string>& kRegistered,
                      const std::string& sPlatform, const std::string& sConfig, std::vector<ExcludeRow>& kRows,
                      std::string& sError)
{
    kRows.clear();
    std::vector<std::string> kLines = Split(sText, '\n');
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (!kLines[i].empty() && kLines[i][kLines[i].size() - 1] == '\r')
        {
            kLines[i].erase(kLines[i].size() - 1);
        }
    }
    if (kLines.empty() || kLines[0] != "Test\tReason\tPlatforms\tConfigs")
    {
        sError = "exclude list header is not Test<TAB>Reason<TAB>Platforms<TAB>Configs";
        return false;
    }
    for (size_t i = 1; i < kLines.size(); ++i)
    {
        if (kLines[i].empty())
        {
            continue;
        }
        const std::vector<std::string> kCells = Split(kLines[i], '\t');
        char acLine[32];
        _snprintf(acLine, sizeof(acLine), "line %u: ", (unsigned int)(i + 1));
        acLine[sizeof(acLine) - 1] = 0;
        if (kCells.size() != 4)
        {
            sError = std::string(acLine) + "a row needs 4 tab-separated cells";
            return false;
        }
        ExcludeRow kRow;
        kRow.sTest = kCells[0];
        kRow.sReason = kCells[1];
        kRow.kPlatforms = SplitList(kCells[2]);
        kRow.kConfigs = SplitList(kCells[3]);
        if (kRow.sReason.empty())
        {
            sError = std::string(acLine) + kRow.sTest + " has no Reason";
            return false;
        }
        if (Applies(kRow.kPlatforms, sPlatform) && Applies(kRow.kConfigs, sConfig) &&
            std::find(kRegistered.begin(), kRegistered.end(), kRow.sTest) == kRegistered.end())
        {
            sError = std::string(acLine) + kRow.sTest + " is not a registered test";
            return false;
        }
        kRows.push_back(kRow);
    }
    return true;
}

const ExcludeRow* FindExclusion(const std::vector<ExcludeRow>& kRows, const std::string& sTest,
                                const std::string& sPlatform, const std::string& sConfig)
{
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].sTest == sTest && Applies(kRows[i].kPlatforms, sPlatform) && Applies(kRows[i].kConfigs, sConfig))
        {
            return &kRows[i];
        }
    }
    return 0;
}
