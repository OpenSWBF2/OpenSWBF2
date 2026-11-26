#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include "x87/x87_disasm.h"

namespace X87
{

struct OffPathDir
{
    const char* pcPath;
    const char* pcName;
    const char* pcReason;
};

extern const OffPathDir kOffPathDirs[];
extern const size_t kOffPathDirCount;
extern const char* const kNoSourceName;
extern const char* const kNoSourceReason;
extern const char* const kSimExceptions[];
extern const size_t kSimExceptionCount;
extern const char* const kExactEvidence;
extern const char* const kSingleEvidence;

std::string OperandKind(const std::string& sOp);
int StIndex(const std::string& sKind);

typedef std::map<long long, char> RetKinds;

struct Measure
{
    std::string sFlags;
    std::string sPattern;
    std::string sRule;
    char cRet;
};

Measure MeasureFunction(const Function& kFn, const RetKinds& kRet, const std::map<std::string, long long>& kByName,
                        const std::set<long long>& kByAddr);
bool SettleReturns(const std::vector<Function>& kFns, const std::map<std::string, long long>& kByName,
                   const std::set<long long>& kByAddr, int iPasses, std::vector<Measure>& kOut, std::string& sError);

struct SourceRules
{
    SourceMap kSources;
    std::set<std::string> kExceptions;
};

bool PrepareSources(const Hardening::BuildFiles& kFiles, const std::string& sRoot, const std::string& sProject,
                    const std::string& sCauses, SourceRules& kOut, std::string& sError);

struct OffPath
{
    bool bOff;
    std::string sVerdict;
    std::string sEvidence;
    bool bNoSource;
};

OffPath GetOffPath(const std::string& sObj, const SourceRules& kRules);
std::set<std::string> SimCalledNoSource(const std::vector<DisasmFunc>& kFuncs, const std::vector<std::string>& kObjs,
                                        const SourceRules& kRules);

}
