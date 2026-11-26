#pragma once
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "hardening/hardening_binary.h"

namespace X87
{

typedef std::function<void(const std::string&)> LineFn;

class LineSplitter
{
public:
    LineSplitter();
    void Feed(const char* pcData, size_t uSize, const LineFn& fnLine);
    void End(const LineFn& fnLine);

private:
    std::string msPending;
    bool mbCr;
};

std::vector<std::string> TextLines(const std::string& sText);
std::string TrimSpace(const std::string& sText);
bool IsHex8Upper(const std::string& sText);
std::string Hex8(long long iValue);

struct MapEntry
{
    std::string sName;
    std::string sObj;
    bool bFunc;
};

typedef std::map<unsigned int, std::vector<MapEntry> > MapObjects;

bool ReadMapObjects(const std::string& sText, MapObjects& kOut, std::string& sError);
bool FunctionObj(const MapObjects& kMap, long long iStart, const std::string& sName, std::string& sObj,
                 std::string& sError);

struct DisasmFunc
{
    std::string sName;
    long long iStart;
    bool bHasF;
    std::vector<std::string> kLines;
    std::vector<std::string> kCalls;
};

class DisasmReader
{
public:
    explicit DisasmReader(std::set<long long>* pBounds);
    bool Line(const std::string& sLine, std::string& sError);
    void Finish(std::vector<DisasmFunc>& kOut);

private:
    void Close();

    std::set<long long>* mpBounds;
    std::vector<DisasmFunc> mkFuncs;
    bool mbOpen;
    DisasmFunc mkCur;
};

bool ReadDisasmFunctions(const std::string& sText, std::set<long long>* pBounds, std::vector<DisasmFunc>& kOut,
                         std::string& sError);

typedef std::map<std::string, std::string> SourceMap;

bool ItemCondition(const std::string& sCondition, bool& bIn, std::string& sError);
bool ProjectSources(const Hardening::BuildFiles& kFiles, const std::string& sRoot, const std::string& sProject,
                    SourceMap& kOut, std::string& sError);
bool CauseFiles(const std::string& sText, const SourceMap& kSources, std::set<std::string>& kOut, std::string& sError);

struct Ins
{
    long long iA;
    std::string sM;
    std::string sO;
};

struct Function
{
    std::string sName;
    long long iStart;
    std::vector<Ins> kIns;
    std::set<long long> kLabels;
    std::string sObj;
};

bool ConvertFunction(const DisasmFunc& kF, long long iEnd, Function& kOut);

typedef std::function<bool(long long iFrom, long long iTo, std::vector<DisasmFunc>& kOut, std::string& sError)>
    DecodeFn;

bool AddLostFunctions(std::vector<DisasmFunc>& kFuncs, const MapObjects& kMap, const std::set<long long>& kBounds,
                      const DecodeFn& fnDecode, std::string& sError);

bool RunDisasm(const std::string& sPlatform, const std::vector<std::string>& kArgs, std::set<long long>* pBounds,
               std::vector<DisasmFunc>& kOut, std::string& sError);

}
