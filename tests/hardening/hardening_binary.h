#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "hardening/hardening_allow.h"
#include "testutil.h"

namespace Hardening
{

enum RowStatus
{
    kRowPass,
    kRowFail,
    kRowSkip
};

struct BinaryRow
{
    std::string sName;
    RowStatus eStatus;
    std::string sDetail;
};

class BuildFiles
{
public:
    virtual ~BuildFiles()
    {
    }
    virtual bool Read(const std::string& sPath, std::string& sBytes) const = 0;
    virtual bool IsFile(const std::string& sPath) const = 0;
    virtual bool IsDir(const std::string& sPath) const = 0;
    virtual bool List(const std::string& sDir, std::vector<RepoEntry>& kOut) const = 0;
};

class DiskFiles : public BuildFiles
{
public:
    bool Read(const std::string& sPath, std::string& sBytes) const override;
    bool IsFile(const std::string& sPath) const override;
    bool IsDir(const std::string& sPath) const override;
    bool List(const std::string& sDir, std::vector<RepoEntry>& kOut) const override;
};

class MemoryFiles : public BuildFiles
{
public:
    void Add(const std::string& sPath, const std::string& sBytes);
    bool Read(const std::string& sPath, std::string& sBytes) const override;
    bool IsFile(const std::string& sPath) const override;
    bool IsDir(const std::string& sPath) const override;
    bool List(const std::string& sDir, std::vector<RepoEntry>& kOut) const override;

private:
    std::map<std::string, std::pair<std::string, std::string> > mkFiles;
};

struct ClCounts
{
    int iTotal = 0;
    int iGsMinus = 0;
    int iSdl = 0;
    int iAlignedNewOff = 0;
    int iInlineOff = 0;
    int iInlineOn = 0;
    int iThrowingNew = 0;
    int iGl = 0;
    int iFpFast = 0;
    int iFpContract = 0;
    std::vector<std::string> kMiLines;
    std::vector<std::string> kMissing;
    int iWe4311 = 0;
    int iWe4312 = 0;
    int iWe4302 = 0;
    int iWe4477 = 0;
    int iWeAll = 0;
    std::vector<std::string> kWeMissing;
};

struct GameProject
{
    std::string sName;
    std::string sVcxproj;
    std::string sIntDir;
    std::string sTlogDir;
};

typedef std::vector<std::pair<std::string, std::string> > FlagMap;

std::string JoinedLines(const std::string& sText);
std::string DelayHalf(const std::string& sImports, std::string& sStatic);
bool WildcardNoCase(const std::string& sText, const std::string& sPattern);

std::vector<BinaryRow> HardeningChecks(const std::string& sHeaders, const std::string& sLoadConfig,
                                       const std::string& sPlatform, int iClTotal, int iClGsMinus, int iClSdl,
                                       std::string& sError);
std::vector<BinaryRow> GuardChecks(const std::string& sHeaders, const std::string& sLoadConfig, bool bWantCfg,
                                   bool bWantCet, std::string& sError);
bool CountClText(const std::string& sText, ClCounts& kCounts, std::string& sError);
bool CountClTlogs(const BuildFiles& kFiles, const std::vector<std::string>& kDirs, ClCounts& kCounts,
                  std::string& sError);
std::vector<BinaryRow> FlagChecks(const ClCounts& kGame, const ClCounts* pTests, const std::string& sGameConfig,
                                  const std::string& sTestsConfig, const ClCounts* pLuac, const std::string& sPlatform,
                                  const AllowFile& kAllow, std::string& sError);
std::vector<BinaryRow> ImportChecks(const std::string& sImports, const std::string& sPlatform,
                                    const std::vector<std::string>& kStaticAllow,
                                    const std::vector<std::string>& kDelayAllow, std::string& sError);
BinaryRow UnicowsBuildCheck(const std::string& sExeDir, bool bStaged, const std::string& sImports,
                            std::string& sError);
BinaryRow BinkwBuildCheck(const std::string& sExeDir, bool bStaged, const std::string& sImports, std::string& sError);
BinaryRow SteamApi64StagedCheck(const std::string& sDll, bool bStaged, const std::string& sSha, const std::string& sPin,
                                const std::string& sExports, const std::string& sImports, const std::string& sDef,
                                std::string& sError);

bool MsBuildCondition(const std::string& sCondition, const std::string& sPlatform, const std::string& sConfig,
                      bool& bOk, std::string& sError);
bool GameProjects(const BuildFiles& kFiles, const std::string& sRoot, const std::string& sPlatform,
                  const std::string& sConfig, std::vector<GameProject>& kOut, std::string& sError);
bool X64ObjectsCheck(const BuildFiles& kFiles, const std::string& sRoot, const std::string& sPlatform,
                     const std::string& sConfig, BinaryRow& kRow, std::string& sError);
BinaryRow CrtTrigRouteCheck(const std::vector<std::string>& kObjects, const std::string& sSymbols,
                            std::string& sError);
bool LinkedObjects(const std::string& sLinkTlog, std::vector<std::string>& kObjects);
BinaryRow Fma3OffCheck(const std::string& sDisasm, std::string& sError);
bool Fma3OffRanges(const std::string& sMap, std::vector<std::pair<unsigned long long, unsigned long long> >& kRanges,
                   std::string& sError);
BinaryRow MiCrtHookCheck(const std::string& sMap, const std::string& sPlatform, std::string& sError);
BinaryRow RiderLinkMapCheck(const std::string& sMap, const std::string& sConfig, std::string& sError);
std::string PeAslr(const std::string& sBytes);

BinaryRow FFmpegLicenceCheck(const std::string& sConfig, const std::string& sComponents, const std::string& sNotice,
                             const std::string& sChanges, std::string& sError);
BinaryRow FFmpegFlagCheck(const FlagMap& kFlags, const std::string& sConfig, const std::string& sPlatform,
                          std::string& sError);
bool ReadClFlags(const std::string& sText, FlagMap& kFlags);
BinaryRow FFmpegDllCheck(const std::string& sHeaders, const std::string& sLoadConfig, const std::string& sPlatform,
                         bool bWantCfg, bool bWantCet, std::string& sError);
std::vector<BinaryRow> FFmpegImportChecks(const std::string& sImports, const std::string& sPlatform,
                                          const std::string& sConfig, const std::vector<std::string>& kAsanHeap,
                                          std::string& sError);
bool AsanHeapNames(const std::string& sSymbols, const std::string& sPlatform, std::vector<std::string>& kOut,
                   std::string& sError);

std::vector<std::string> BinaryRowNames(const std::string& sPlatform);
std::vector<std::string> FFmpegRowNames();
bool RunBinaryRow(const std::string& sRow, const std::string& sPlatform, const std::string& sConfig, BinaryRow& kRow,
                  std::string& sError);
bool RunFFmpegRow(const std::string& sRow, const std::string& sPlatform, const std::string& sConfig, BinaryRow& kRow,
                  std::string& sError);

}
