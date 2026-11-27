#pragma once
#include <map>
#include <string>
#include <vector>

#include "tables/tables_readers.h"

namespace Tables
{

struct Scrub
{
    std::string sRoot;
    std::string sProfile;
    std::string sUser;
};

std::string ScrubText(const std::string& sText, const Scrub& kScrub);
bool BaselineText(const std::vector<std::string>& kLines, const Scrub& kScrub, const std::string& sLeaf,
                  std::string& sText, std::string& sWhy);

struct CaseLog
{
    std::string sName;
    FileText kRunLog;
    FileText kFinalScore;
};

const CaseLog* FindCase(const std::vector<CaseLog>& kCases, const std::string& sName);
std::vector<std::string> LogLines(const FileText& kRunLog, const std::string& sMarker, bool bBeforeSoak);
std::vector<std::string> FactoryLines(const FileText& kRunLog);
std::vector<std::string> BuildIdLines(const FileText& kRunLog);
std::string DigestKey(const FileText& kRunLog);
std::vector<std::string> MissionNames(const std::string& sText);
std::vector<CaseLog> SortedCases(const std::vector<CaseLog>& kCases);

struct BuiltTable
{
    std::string sName;
    std::vector<std::string> kLines;
};

struct BuildResult
{
    bool bOk;
    std::string sWhy;
    std::vector<BuiltTable> kTables;
    std::vector<std::string> kInfo;
    std::vector<std::string> kVerdicts;
};

BuildResult BuildSmokeAllTables(const std::vector<CaseLog>& kCases, const std::string& sMissionsText,
                                const std::string& sCommit);
BuildResult BuildLuaTables(const std::vector<CaseLog>& kCases, const std::string& sMissionsText,
                           const std::string& sCommit);
BuildResult BuildDebugDigests(const std::vector<CaseLog>& kCases);
BuildResult BuildCollect(const std::vector<CaseLog>& kCases, int iRunCode);

struct MapSymbols
{
    std::string sTimestamp;
    unsigned long long ullBase;
    unsigned long long ullTextEnd;
    std::vector<unsigned long long> kAddrs;
    std::vector<std::string> kNames;
};

bool ReadMapSymbols(const std::string& sMap, MapSymbols& kSym, std::string& sWhy);

struct Frame
{
    bool bSym;
    std::string sName;
    unsigned long long ullOffset;
    std::string sFrame;
};

bool FindMapSymbol(const MapSymbols& kSym, const std::string& sFrame, Frame& kFrame, std::string& sWhy);

struct OffRow
{
    bool bText;
    std::string sText;
    std::string sPrefix;
    std::vector<Frame> kFrames;
};

bool OffThreadRows(const std::vector<CaseLog>& kCases, const std::string& sSmokeDir, const MapSymbols& kSym,
                   std::vector<OffRow>& kRows, std::string& sWhy);
std::vector<OffRow> OffThreadHeader(const std::string& sCommit);
std::vector<std::string> OffThreadNames(const std::vector<OffRow>& kRows);
std::vector<std::string> FormatOffThreadRows(const std::vector<OffRow>& kRows,
                                             const std::map<std::string, std::string>& kDem);

struct MapRow
{
    int iSec;
    unsigned long long ullOffset;
    std::string sSymbol;
    std::string sObj;
    size_t uLine;
};

struct MapTables
{
    std::vector<MapRow> kInRange;
    std::vector<MapRow> kDiscarded;
    std::vector<MapRow> kDi;
    unsigned long long ullXcuSize;
};

bool MapInitializers(const std::string& sMap, MapTables& kTables, std::string& sWhy);
std::vector<std::string> MapNames(const MapTables& kTables);
std::vector<std::string> FormatMapRows(const std::vector<MapRow>& kRows, const std::map<std::string, std::string>& kDem);
std::string MapInfoLine(const MapTables& kTables, size_t uDemangled);

std::vector<std::string> UndnameArgLines(const std::vector<std::string>& kNames);
void ParseUndname(const std::string& sOutput, std::map<std::string, std::string>& kDem);

}
