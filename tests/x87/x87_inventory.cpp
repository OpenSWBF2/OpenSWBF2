#include "x87/x87_inventory.h"

#include <algorithm>
#include <climits>

#include "gate/gate_keys.h"
#include "hardening/hardening_regex.h"

namespace X87
{

const char* const kHeader = "obj\tfunction\tflags\tpattern\tverdict\tevidence";

namespace
{

std::string RowKey(const Row& r)
{
    return r.sObj + "\t" + r.sFunction;
}

const Hardening::RegexPattern& RuleVerdictPattern()
{
    static const Hardening::RegexPattern k(
        [] {
            std::string s = "^(offpath:(";
            for (size_t d = 0; d < kOffPathDirCount; ++d)
            {
                s += std::string(kOffPathDirs[d].pcName) + "|";
            }
            s += std::string(kNoSourceName) + ")$|match:exact$|match:single-rounding$)";
            return s;
        }(),
        false);
    return k;
}

std::string TabToSpace(const std::string& s)
{
    std::string o = s;
    std::replace(o.begin(), o.end(), '\t', ' ');
    return o;
}

}

bool BuildInventory(const SourceRules& kRules, const MapObjects& kMap, std::vector<DisasmFunc>& kFuncs,
                    const std::set<long long>& kBounds, const DecodeFn& fnDecode, std::vector<Row>& kRows,
                    std::string& sError)
{
    kRows.clear();
    if (!AddLostFunctions(kFuncs, kMap, kBounds, fnDecode, sError))
    {
        return false;
    }
    std::vector<std::string> kObjs;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        std::string sObj;
        if (!FunctionObj(kMap, kFuncs[i].iStart, kFuncs[i].sName, sObj, sError))
        {
            return false;
        }
        kObjs.push_back(sObj);
    }
    const std::set<std::string> kSimCalled = SimCalledNoSource(kFuncs, kObjs, kRules);
    std::vector<Function> kFns;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        const DisasmFunc& f = kFuncs[i];
        if (!f.bHasF || f.iStart < 0)
        {
            continue;
        }
        long long iEnd = LLONG_MAX;
        for (size_t j = i + 1; j < kFuncs.size(); ++j)
        {
            if (kFuncs[j].iStart >= 0)
            {
                iEnd = kFuncs[j].iStart;
                break;
            }
        }
        Function c;
        if (ConvertFunction(f, iEnd, c))
        {
            c.sObj = kObjs[i];
            kFns.push_back(c);
        }
    }
    std::set<std::string> kOnce;
    std::set<std::string> kTwice;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        if (kFuncs[i].iStart >= 0 && !kOnce.insert(kFuncs[i].sName).second)
        {
            kTwice.insert(kFuncs[i].sName);
        }
    }
    std::map<std::string, long long> kByName;
    std::set<long long> kByAddr;
    for (size_t i = 0; i < kFns.size(); ++i)
    {
        if (!kTwice.count(kFns[i].sName))
        {
            kByName[kFns[i].sName] = kFns[i].iStart;
        }
        kByAddr.insert(kFns[i].iStart);
    }
    std::vector<Measure> kResults;
    if (!SettleReturns(kFns, kByName, kByAddr, 50, kResults, sError))
    {
        return false;
    }
    std::set<std::string> kKeys;
    for (size_t i = 0; i < kFns.size(); ++i)
    {
        const Function& c = kFns[i];
        if (!kKeys.insert(c.sObj + "\t" + c.sName).second)
        {
            sError = "x87-inventory: two functions named " + c.sName + " in " + c.sObj;
            return false;
        }
        const Measure& r = kResults[i];
        Row kRow;
        kRow.sObj = c.sObj;
        kRow.sFunction = c.sName;
        kRow.sFlags = r.sFlags;
        kRow.sPattern = r.sPattern;
        if (r.sRule == "match:exact")
        {
            kRow.sVerdict = r.sRule;
            kRow.sEvidence = kExactEvidence;
        }
        else if (r.sRule == "match:single-rounding")
        {
            kRow.sVerdict = r.sRule;
            kRow.sEvidence = kSingleEvidence;
        }
        else
        {
            const OffPath o = GetOffPath(c.sObj, kRules);
            if (o.bOff && !(o.bNoSource && kSimCalled.count(c.sName)))
            {
                kRow.sVerdict = o.sVerdict;
                kRow.sEvidence = o.sEvidence;
            }
        }
        kRows.push_back(kRow);
    }
    return true;
}

bool ReadInventoryText(bool bFound, const std::string& sText, const std::string& sPath, std::vector<Row>& kRows,
                       std::string& sError)
{
    kRows.clear();
    if (!bFound)
    {
        sError = "x87-inventory: " + sPath + " not found";
        return false;
    }
    const std::vector<std::string> kLines =
        TextLines(Gate::DecodeText(std::vector<unsigned char>(sText.begin(), sText.end())));
    if (kLines.empty() || kLines[0] != kHeader)
    {
        sError = "x87-inventory: " + sPath + " has no header row";
        return false;
    }
    for (size_t i = 1; i < kLines.size(); ++i)
    {
        const std::vector<std::string> c = Gate::SplitText(kLines[i], '\t');
        if (c.size() != 6)
        {
            sError = "x87-inventory: " + sPath + " line " + std::to_string(i + 1) + " has " + std::to_string(c.size()) +
                     " columns";
            return false;
        }
        Row r;
        r.sObj = c[0];
        r.sFunction = c[1];
        r.sFlags = c[2];
        r.sPattern = c[3];
        r.sVerdict = c[4];
        r.sEvidence = c[5];
        kRows.push_back(r);
    }
    return true;
}

bool RowIndex(const std::vector<Row>& kRows, std::map<std::string, const Row*>& kOut, std::string& sError)
{
    kOut.clear();
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (!kOut.insert(std::make_pair(RowKey(kRows[i]), &kRows[i])).second)
        {
            sError = "x87-inventory: row " + kRows[i].sObj + " " + kRows[i].sFunction + " appears twice";
            return false;
        }
    }
    return true;
}

std::string InventoryText(const std::vector<Row>& kRows)
{
    std::vector<const Row*> kSorted;
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        kSorted.push_back(&kRows[i]);
    }
    std::stable_sort(kSorted.begin(), kSorted.end(),
                     [](const Row* a, const Row* b) { return RowKey(*a) < RowKey(*b); });
    std::string sOut = std::string(kHeader) + "\r\n";
    for (size_t i = 0; i < kSorted.size(); ++i)
    {
        const Row& r = *kSorted[i];
        sOut += r.sObj + "\t" + r.sFunction + "\t" + r.sFlags + "\t" + r.sPattern + "\t" + r.sVerdict + "\t" +
                r.sEvidence + "\r\n";
    }
    return sOut;
}

bool IsRuleVerdict(const std::string& sVerdict)
{
    std::string sError;
    return Hardening::RegexIsMatch(RuleVerdictPattern(), sVerdict, sError);
}

bool CompareInventory(const std::vector<Row>& kPinned, const std::vector<Row>& kFresh, std::vector<std::string>& kBad,
                      std::string& sError)
{
    kBad.clear();
    if (!RuleVerdictPattern().Valid())
    {
        sError = RuleVerdictPattern().Error();
        return false;
    }
    std::map<std::string, const Row*> p;
    std::map<std::string, const Row*> f;
    if (!RowIndex(kPinned, p, sError) || !RowIndex(kFresh, f, sError))
    {
        return false;
    }
    std::set<std::string> kKeys;
    for (std::map<std::string, const Row*>::const_iterator it = p.begin(); it != p.end(); ++it)
    {
        kKeys.insert(it->first);
    }
    for (std::map<std::string, const Row*>::const_iterator it = f.begin(); it != f.end(); ++it)
    {
        kKeys.insert(it->first);
    }
    for (std::set<std::string>::const_iterator k = kKeys.begin(); k != kKeys.end(); ++k)
    {
        const std::string sName = TabToSpace(*k);
        std::map<std::string, const Row*>::const_iterator fi = f.find(*k);
        if (fi == f.end())
        {
            kBad.push_back("removed " + sName);
            continue;
        }
        const Row& n = *fi->second;
        std::map<std::string, const Row*>::const_iterator pi = p.find(*k);
        if (pi == p.end())
        {
            kBad.push_back("new " + sName + " flags=" + n.sFlags + " pattern=" + n.sPattern);
            continue;
        }
        const Row& o = *pi->second;
        if (o.sFlags != n.sFlags || o.sPattern != n.sPattern)
        {
            kBad.push_back("changed " + sName + " flags " + o.sFlags + " -> " + n.sFlags + " pattern " + o.sPattern +
                           " -> " + n.sPattern);
            continue;
        }
        if (o.sVerdict.empty())
        {
            kBad.push_back("no verdict " + sName);
            continue;
        }
        const bool bRuleRow = IsRuleVerdict(o.sVerdict) || IsRuleVerdict(n.sVerdict);
        if (bRuleRow && (o.sVerdict != n.sVerdict || o.sEvidence != n.sEvidence))
        {
            kBad.push_back("rule verdict " + sName + " pinned '" + o.sVerdict + "' generated '" + n.sVerdict + "'");
        }
    }
    return true;
}

bool KeepHandVerdicts(std::vector<Row>& kRows, const std::vector<Row>& kOld, std::string& sError)
{
    std::map<std::string, const Row*> kIndex;
    if (!RowIndex(kOld, kIndex, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        Row& r = kRows[i];
        if (!r.sVerdict.empty())
        {
            continue;
        }
        std::map<std::string, const Row*>::const_iterator it = kIndex.find(RowKey(r));
        if (it == kIndex.end())
        {
            continue;
        }
        const Row& o = *it->second;
        if (o.sFlags == r.sFlags && o.sPattern == r.sPattern && !o.sVerdict.empty() && !IsRuleVerdict(o.sVerdict))
        {
            r.sVerdict = o.sVerdict;
            r.sEvidence = o.sEvidence;
        }
    }
    return true;
}

std::string CountsLine(const std::vector<Row>& kRows)
{
    size_t uExact = 0;
    size_t uSingle = 0;
    size_t uOff = 0;
    size_t uKept = 0;
    size_t uOpen = 0;
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        const std::string& v = kRows[i].sVerdict;
        if (v == "match:exact")
        {
            ++uExact;
        }
        else if (v == "match:single-rounding")
        {
            ++uSingle;
        }
        else if (v.compare(0, 8, "offpath:") == 0)
        {
            ++uOff;
        }
        else if (!v.empty())
        {
            ++uKept;
        }
        else
        {
            ++uOpen;
        }
    }
    return "x87-inventory: rows=" + std::to_string(kRows.size()) + " exact=" + std::to_string(uExact) +
           " single-rounding=" + std::to_string(uSingle) + " offpath=" + std::to_string(uOff) +
           " kept=" + std::to_string(uKept) + " open=" + std::to_string(uOpen);
}

}
