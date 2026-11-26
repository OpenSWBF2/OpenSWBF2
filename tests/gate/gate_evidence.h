#pragma once
#include <map>
#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "gate/gate_machine.h"

namespace Gate
{

struct IndexTest
{
    std::string sFullPath;
    std::string sState;
};

struct IndexJson
{
    bool bHasFailed;
    std::string sFailed;
    double fFailed;
    bool bHasTests;
    std::vector<IndexTest> kTests;
};

bool ReadIndexJson(const std::string& sText, IndexJson& kIndex, std::string& sError);

std::string EvidenceHash(const std::string& sDir);
bool IsEvidenceName(const std::string& sHash);
std::string EvidenceIntact(const std::string& sEvidenceRoot, const std::string& sHash);
bool SaveEvidence(const std::string& sEvidenceRoot, const std::string& sWork, unsigned long uCode, bool bRun,
                  const KeyPairs& kRuns, const std::vector<std::string>& kOutputs, std::string& sHash,
                  std::string& sError);

struct JudgeSpec
{
    std::string sKind;
    bool bDirect;
    std::vector<std::string> kMustRun;
};

std::string JudgeRunRecord(const std::string& sDir, const JudgeSpec& kSpec);
std::string JudgeEvidenceFolder(const std::string& sDir, const JudgeSpec& kSpec);
std::string JudgeArchived(const std::string& sEvidenceRoot, const std::string& sHash, const std::string& sCopy,
                          const JudgeSpec& kSpec);

std::vector<std::string> VerdictLines(const std::string& sOutput);
std::string ResultsText(const std::vector<std::string>& kLines, bool bAsan, const std::string& sNote);

struct RunParamsInput
{
    std::vector<std::string> kArgs;
    std::string sPlatform;
    std::string sConfiguration;
    std::string sSmokeConfiguration;
    bool bWerLocalDumps;
    std::string sSteam;
    std::string sTestsExe;
    KeyPairs kExes;
    KeyPairs kDlls;
};

std::vector<std::string> RunParamsLines(const RunParamsInput& kIn);
bool HostFaultLines(const std::string& sStage, HostFaultCountFn pCount, unsigned long long uStart,
                    std::vector<std::string>& kLines);
std::map<std::string, std::string> ReadKeyValueText(const std::string& sText);
std::string HostFaultNote(const std::map<std::string, std::string>& kParams, int iBad);
std::string LinesText(const std::vector<std::string>& kLines);

extern const char* const kAsanCoverage;

}
