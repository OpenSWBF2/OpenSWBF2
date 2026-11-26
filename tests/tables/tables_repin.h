#pragma once
#include <functional>
#include <string>
#include <vector>

#include "tables/tables_readers.h"

namespace Tables
{

typedef std::function<bool(const std::string& sPath, std::string& sSha)> ExeShaFn;

struct RepinInput
{
    std::string sRoot;
    std::string sRunDir;
    std::string sTable;
    std::vector<std::string> kKeyArgs;
    std::string sBaselineDir;
    std::string sExpectedFile;
    std::string sExe;
    RepoTextFn fnRead;
    ExeShaFn fnExeSha;
};

struct RepinResult
{
    int iExit;
    std::vector<std::string> kLines;
    std::string sWritePath;
    std::string sWriteText;
};

const std::vector<std::string>& RepinTables();
bool RepinTableFile(const std::string& sTable, std::string& sFile, int& iKeyCols, std::string& sWhy);
std::vector<std::string> RepinKeys(const std::vector<std::string>& kArgs);
std::string RunParam(const std::string& sRunParams, const std::string& sName);
RepinResult Repin(const RepinInput& kIn);

}
