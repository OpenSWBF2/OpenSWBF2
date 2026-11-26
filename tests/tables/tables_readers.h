#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Tables
{

enum ReaderKind
{
    kReadTables,
    kReadMemPeaksX64,
    kReadCommit,
    kReadLuaBins,
    kReadLuaHist,
    kReadOffThread,
    kReadCollect,
    kReadMap,
    kReadDebugDigests,
    kReadX87Inventory,
    kReadUnit,
    kReadVerify
};

struct Reader
{
    const char* pcFile;
    const char* pcFolder;
    ReaderKind eKind;
    const char* pcLabel;
    int iKeyCols;
    const char* pcSkipRx;
    bool bOffThread;
    const char* pcPair;
    const char* pcBins;
    const char* pcCpp;
    const char* pcUnit;
    const char* pcVerify;
};

struct FileText
{
    bool bFound;
    std::string sText;
};

FileText Found(const std::string& sText);
FileText Absent();

const std::vector<Reader>& BaselineReaders();
std::string ReaderFolder(const Reader& kReader);
const char* KindName(ReaderKind eKind);
std::vector<Reader> SelectReaders(ReaderKind eKind, const std::string& sPlatform);
bool KeyColumnsFor(const std::string& sFile, int& iKeyCols);

std::vector<std::string> ReadAllLines(const std::string& sText);
std::vector<std::string> SplitTabs(const std::string& sLine);
std::string JoinText(const std::vector<std::string>& kItems, const std::string& sSep);
std::string TrimEndSpace(const std::string& sText);
std::string LowerAscii(const std::string& sText);
bool EqualNoCase(const std::string& sA, const std::string& sB);
size_t FindNoCase(const std::string& sText, const std::string& sNeedle);
bool StartsWith(const std::string& sText, const std::string& sPrefix);
bool AllDigits(const std::string& sText);
std::string JoinPath(const std::string& sDir, const std::string& sName);
int CultureCompare(const std::string& sA, const std::string& sB, bool bIgnoreCase);
void CultureSort(std::vector<std::string>& kItems, bool bIgnoreCase);

typedef std::function<FileText(const std::string& sRel)> RepoTextFn;

std::vector<std::string> BaselineReadersCheck(const std::vector<Reader>& kReaders,
                                              const std::map<std::string, std::vector<std::string> >& kFolderFiles,
                                              const RepoTextFn& fnRepoText);
std::vector<std::string> AllCheckpointsCheck(const RepoTextFn& fnRepoText);

}
