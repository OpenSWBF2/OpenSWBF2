#pragma once
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Gate
{

typedef std::vector<std::pair<std::string, std::string> > KeyPairs;
typedef std::map<std::string, std::string> RecordRow;
typedef std::map<std::string, RecordRow> RecordMap;

std::string Sha256Hex(const void* pData, size_t uSize);
std::string TextSha(const std::string& sText);
bool FileSha(const std::string& sPath, std::string& sHash);
bool ReadFileBytes(const std::string& sPath, std::vector<unsigned char>& kBytes);
bool ReadTextFile(const std::string& sPath, std::string& sText);
bool WriteTextFile(const std::string& sPath, const std::string& sText);
bool AppendTextFile(const std::string& sPath, const std::string& sText);
bool FileExists(const std::string& sPath);
bool DirExists(const std::string& sPath);
bool MakeDirs(const std::string& sPath);
bool FileWriteTime(const std::string& sPath, unsigned long long& uTime);
std::string FullPath(const std::string& sPath);
std::string JoinPath(const std::string& sDir, const std::string& sRel);
std::string LowerText(const std::string& sText);
std::string UpperText(const std::string& sText);
std::string TrimText(const std::string& sText);
bool StartsWithNoCase(const std::string& sText, const std::string& sPrefix);
bool EqualNoCase(const std::string& sA, const std::string& sB);
std::vector<std::string> SplitText(const std::string& sText, char cSep);
std::vector<std::string> SplitLines(const std::string& sText);
std::string JoinText(const std::vector<std::string>& kParts, const std::string& sSep);
std::string DecodeText(const std::vector<unsigned char>& kBytes);
std::string WideToUtf8(const std::wstring& sText);
std::wstring Utf8ToWide(const std::string& sText);

bool PeNormalise(std::vector<unsigned char>& kBytes, std::string& sError);
bool PeNormalisedSha(const std::string& sPath, std::string& sHash, std::string& sError);

bool LikeMatch(const std::string& sText, const std::string& sPattern);
bool InputMatches(const std::string& sPath, const std::vector<std::string>& kInputs);

typedef bool (*TreeListFn)(const std::string& sRoot, std::vector<std::string>& kPaths, std::string& sError);
bool TreeHashes(const std::string& sRoot, TreeListFn pList, std::map<std::string, std::string>& kTree,
                std::string& sError);

struct ReadTlog
{
    std::map<std::string, std::set<std::string> > kSets;
    std::set<std::string> kStale;
};

typedef bool (*FileTimeFn)(const std::string& sPath, unsigned long long& uTime);
void ParseReadTlog(const std::string& sText, const std::string& sRoot, ReadTlog& kTlog);
void MarkStaleUnits(ReadTlog& kTlog, const std::string& sRoot, unsigned long long uTlogTime, FileTimeFn pTime);
bool LoadReadTlog(const std::string& sPath, const std::string& sRoot, ReadTlog& kTlog);
std::string CommandRootToken(const std::string& sCommand, const std::string& sRoot);
void ParseCommandTlog(const std::string& sText, const std::string& sRoot, std::map<std::string, std::string>& kCommands);
bool LoadCommandTlog(const std::string& sPath, const std::string& sRoot, std::map<std::string, std::string>& kCommands);

typedef std::string (*FileHashFn)(const std::string& sRel);

struct KeyFilesInput
{
    const std::map<std::string, std::string>* pTree;
    std::vector<std::string> kReads;
    std::vector<std::string> kSources;
    const ReadTlog* pRead;
    const std::map<std::string, std::string>* pCommands;
    std::string sDefinition;
    FileHashFn pHash;
};

void KeyFiles(const KeyFilesInput& kIn, std::map<std::string, std::string>& kFiles, std::string& sTlogState);

std::string KeyScalar(const std::string& sValue);
std::string KeyList(const std::vector<std::string>& kValues);
std::string KeyMap(const KeyPairs& kValues);

extern const char* const kKeyHeaderNames[];
extern const size_t kKeyHeaderCount;

std::set<std::string> JudgeOnlyFiles(const std::vector<std::string>& kKinds);

struct KeyInput
{
    KeyPairs kHeader;
    std::vector<std::string> kJudgeKinds;
    std::map<std::string, std::string> kFiles;
};

struct StepKey
{
    std::string sRunKey;
    std::string sJudgeKey;
    bool bValid;
    std::string sPlatform;
    std::vector<std::string> kHeader;
    std::vector<std::string> kRunFiles;
    std::vector<std::string> kJudgeFiles;
    std::string sRunText;
    std::string sJudgeText;
};

StepKey MakeStepKey(const KeyInput& kIn);
std::vector<std::string> InvalidKeyFields(const StepKey& kKey);

extern const char* const kStepColumns[];
extern const size_t kStepColumnCount;
extern const char* const kStepKeyColumns[];
extern const size_t kStepKeyColumnCount;
extern const char* const kLedgerColumns[];
extern const size_t kLedgerColumnCount;

std::vector<std::string> ColumnList(const char* const* ppcColumns, size_t uCount);
RecordMap ReadKeyedRecords(const std::string& sText, const std::vector<std::string>& kKeyColumns);
std::string MergeKeyedRecords(const std::string& sOldText, const std::vector<std::string>& kColumns,
                              const std::vector<std::string>& kKeyColumns, const std::vector<std::string>& kNewLines,
                              const std::vector<std::string>& kDropKeys);
std::string FormatRecord(const RecordRow& kRow, const std::vector<std::string>& kColumns);
bool RecordUsable(const RecordRow* pRow);
bool RecordMatches(const RecordRow* pRow, const StepKey& kKey);
bool MergeStepRecordsFile(const std::string& sPath, const std::vector<std::string>& kNewLines,
                          const std::vector<std::string>& kDropKeys);
RecordMap ReadStepRecordsFile(const std::string& sPath);
bool AppendLedgerLine(const std::string& sPath, const std::string& sLine);

}
