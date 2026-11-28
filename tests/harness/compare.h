#pragma once
#include <string>
#include <vector>

std::vector<std::string> MarkerLines(const std::string& sLog, const char* pcMarker);
std::string LastMarkerLine(const std::string& sLog, const char* pcMarker);
bool FindTableRow(const std::string& sTable, const char* pcSection, const std::string& sName, std::string& sRow);
bool CompareDigestRow(const std::string& sLog, const std::string& sRow, std::string& sWhy);
bool HasDigestTrace(const std::string& sLog, unsigned int uTurn);
bool CompareSameRun(const std::string& sLog, const std::string& sRefLog, std::string& sWhy);
bool CompareEqualDigests(const std::string& sLog, int iCount, std::string& sWhy);
bool CompareTurn1500Runtime(const std::string& sLog, const std::string& sRefLog, std::string& sWhy);
bool CompareCommitFlat(const std::string& sLog, long long llBound, int iCycles, int iSettled, std::string& sWhy);
bool CompareCommitRows(const std::string& sLog, const std::string& sTable, bool bSteam, bool bHog, int iSettled,
                       std::string& sWhy);
bool CompareBytes(const std::string& sA, const std::string& sB, std::string& sWhy);
bool CompareSmokeAllRow(const std::string& sLog, const std::string& sFinalScore, const std::string& sRow,
                        std::string& sWhy);
bool CompareOffThreadRow(const std::string& sLog, const std::string& sRow, std::string& sWhy);
bool IniHasLine(const std::string& sIni, const char* pcLine);
bool IniHasKey(const std::string& sIni, const char* pcKey);
bool HasArgWord(const std::string& sArgs, const std::string& sArg);
std::string DigestRowName(const std::string& sCase);
bool CommitHogMode(const std::string& sPlatform, const std::string& sArgs, bool& bHog, std::string& sWhy);

struct MissionLine
{
    std::string sName;
    std::string sClass;
};

bool ParseMissions(const std::string& sText, std::vector<MissionLine>& kMissions, std::string& sError);
bool CheckMissionClass(const std::string& sClass, unsigned int uTurns, unsigned int uSoakTurns, unsigned int uCap,
                       std::string& sWhy);
bool LastTurnsCovered(const std::string& sLog, unsigned int uTurns, std::string& sWhy);
std::string FinalScoreKey(const std::string& sFinalScore);
bool CompareFinalScores(const std::string& sA, const std::string& sB, std::string& sWhy);
bool CompareLastDigests(const std::string& sLogA, const std::string& sLogB, std::string& sWhy);
int TraceSplitTurn(const std::string& sLogA, const std::string& sLogB);
bool TraceMatchesDigest(const std::string& sLog, std::string& sWhy);
bool CheckPerturbBase(const std::string& sBase, std::string& sWhy);
bool ComparePerturbRun(const std::string& sBase, const std::string& sRun, bool bRepeat, std::string& sWhy);
std::vector<std::string> GrownPools(const std::string& sTable, const std::string& sMission);
bool FramesCount(const std::string& sLog, long long& llFrames);
