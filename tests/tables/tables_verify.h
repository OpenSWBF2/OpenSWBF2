#pragma once
#include <string>
#include <vector>

#include "tables/tables_readers.h"

namespace Tables
{

struct BaselineFile
{
    std::string sRel;
    std::string sText;
};

struct VerifyResult
{
    bool bOk;
    std::vector<std::string> kLines;
    std::vector<std::string> kProblems;
};

VerifyResult ManifestVerify(const std::string& sBaseline, const FileText& kManifest,
                            const std::vector<BaselineFile>& kFiles, const std::string& sUser,
                            const std::string& sTrackedBinOut);

}
