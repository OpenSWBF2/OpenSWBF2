#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include "x87/x87_disasm.h"
#include "x87/x87_sim.h"

namespace X87
{

struct Row
{
    std::string sObj;
    std::string sFunction;
    std::string sFlags;
    std::string sPattern;
    std::string sVerdict;
    std::string sEvidence;
};

extern const char* const kHeader;

bool BuildInventory(const SourceRules& kRules, const MapObjects& kMap, std::vector<DisasmFunc>& kFuncs,
                    const std::set<long long>& kBounds, const DecodeFn& fnDecode, std::vector<Row>& kRows,
                    std::string& sError);
bool ReadInventoryText(bool bFound, const std::string& sText, const std::string& sPath, std::vector<Row>& kRows,
                       std::string& sError);
bool RowIndex(const std::vector<Row>& kRows, std::map<std::string, const Row*>& kOut, std::string& sError);
std::string InventoryText(const std::vector<Row>& kRows);
bool IsRuleVerdict(const std::string& sVerdict);
bool CompareInventory(const std::vector<Row>& kPinned, const std::vector<Row>& kFresh, std::vector<std::string>& kBad,
                      std::string& sError);
bool KeepHandVerdicts(std::vector<Row>& kRows, const std::vector<Row>& kOld, std::string& sError);
std::string CountsLine(const std::vector<Row>& kRows);

}
