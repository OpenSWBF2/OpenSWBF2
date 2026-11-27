#include "tables/tables_verify.h"

#include <regex>

#include "gate/gate_keys.h"

namespace Tables
{

namespace
{

struct Listed
{
    std::string sKey;
    std::string sHash;
};

Listed* FindListed(std::vector<Listed>& k, const std::string& sKey)
{
    for (size_t i = 0; i < k.size(); ++i)
    {
        if (EqualNoCase(k[i].sKey, sKey))
        {
            return &k[i];
        }
    }
    return 0;
}

std::string TrimText(const std::string& s)
{
    size_t uStart = 0;
    while (uStart < s.size() && (s[uStart] == ' ' || (s[uStart] >= '\t' && s[uStart] <= '\r')))
    {
        ++uStart;
    }
    return TrimEndSpace(s.substr(uStart));
}

}

VerifyResult ManifestVerify(const std::string& sBaseline, const FileText& kManifest,
                            const std::vector<BaselineFile>& kFiles, const std::string& sUser,
                            const std::string& sTrackedBinOut)
{
    VerifyResult k;
    k.bOk = false;
    if (!kManifest.bFound)
    {
        k.kLines.push_back("[FAIL] capture:verify - MANIFEST.md not found in the output dir");
        k.kProblems.push_back("MANIFEST.md not found in the output dir");
        return k;
    }
    const std::regex kZipRx("^Archive: `([^`]+)\\\\([^`\\\\]+)` SHA-256 `([0-9A-F]{64})`");
    const std::regex kRowRx("^\\| `([^`]+)` \\| ([0-9A-F]{64}) \\|");
    std::string sSection;
    std::vector<Listed> kListed;
    std::vector<Listed> kMembers;
    bool bZip = false;
    const std::vector<std::string> kText = ReadAllLines(kManifest.sText);
    for (size_t i = 0; i < kText.size(); ++i)
    {
        const std::string& l = kText[i];
        if (StartsWith(l, "## "))
        {
            sSection = TrimText(l.substr(3));
            continue;
        }
        std::smatch m;
        if (std::regex_search(l, m, kZipRx))
        {
            bZip = true;
            continue;
        }
        if (!std::regex_search(l, m, kRowRx))
        {
            continue;
        }
        std::vector<Listed>* pList = 0;
        if (EqualNoCase(sSection, "Archive"))
        {
            pList = &kMembers;
        }
        else if (EqualNoCase(sSection, "Text artifacts"))
        {
            pList = &kListed;
        }
        if (!pList)
        {
            continue;
        }
        Listed* p = FindListed(*pList, m[1].str());
        if (p)
        {
            p->sHash = m[2].str();
        }
        else
        {
            Listed n;
            n.sKey = m[1].str();
            n.sHash = m[2].str();
            pList->push_back(n);
        }
    }
    if (kListed.empty())
    {
        k.kProblems.push_back("no text artifacts listed");
    }
    std::vector<std::string> kListedKeys;
    for (size_t i = 0; i < kListed.size(); ++i)
    {
        kListedKeys.push_back(kListed[i].sKey);
    }
    CultureSort(kListedKeys, true);
    std::vector<std::string> kArtifacts;
    for (size_t i = 0; i < kFiles.size(); ++i)
    {
        if (!EqualNoCase(kFiles[i].sRel, "MANIFEST.md"))
        {
            kArtifacts.push_back(kFiles[i].sRel);
        }
    }
    CultureSort(kArtifacts, true);
    for (size_t i = 0; i < kListedKeys.size(); ++i)
    {
        const std::string& sRel = kListedKeys[i];
        const BaselineFile* pFile = 0;
        for (size_t j = 0; j < kFiles.size(); ++j)
        {
            if (EqualNoCase(kFiles[j].sRel, sRel))
            {
                pFile = &kFiles[j];
                break;
            }
        }
        if (!pFile)
        {
            k.kProblems.push_back("missing " + sRel);
            k.kLines.push_back("[FAIL] verify:" + sRel + " - missing");
            continue;
        }
        if (!EqualNoCase(Gate::TextSha(pFile->sText), FindListed(kListed, sRel)->sHash))
        {
            k.kProblems.push_back("hash mismatch " + sRel);
            k.kLines.push_back("[FAIL] verify:" + sRel + " - hash mismatch");
        }
        else
        {
            k.kLines.push_back("[PASS] verify:" + sRel);
        }
    }
    for (size_t i = 0; i < kArtifacts.size(); ++i)
    {
        if (!FindListed(kListed, kArtifacts[i]))
        {
            k.kProblems.push_back("not in manifest " + kArtifacts[i]);
        }
    }
    if (bZip || !kMembers.empty())
    {
        k.kProblems.push_back("archive section in the " + sBaseline + " manifest");
    }
    if (!sUser.empty())
    {
        for (size_t i = 0; i < kFiles.size(); ++i)
        {
            if (FindNoCase(kFiles[i].sText, sUser) != std::string::npos)
            {
                const size_t uSlash = kFiles[i].sRel.find_last_of("\\/");
                k.kProblems.push_back("user name in " +
                                      (uSlash == std::string::npos ? kFiles[i].sRel : kFiles[i].sRel.substr(uSlash + 1)));
            }
        }
    }
    const std::regex kBinRx("\\.(exe|pdb|map)$", std::regex::icase);
    const std::vector<std::string> kTracked = ReadAllLines(sTrackedBinOut);
    for (size_t i = 0; i < kTracked.size(); ++i)
    {
        if (std::regex_search(kTracked[i], kBinRx))
        {
            k.kProblems.push_back("tracked binary " + kTracked[i]);
        }
    }
    for (size_t i = 0; i < k.kProblems.size(); ++i)
    {
        k.kLines.push_back("[FAIL] capture:verify - " + k.kProblems[i]);
    }
    k.bOk = k.kProblems.empty();
    if (k.bOk)
    {
        k.kLines.push_back("[PASS] capture:verify");
    }
    return k;
}

}
