#pragma once
#include <map>
#include <string>
#include <vector>

#include "tables/tables_readers.h"

namespace Tables
{

struct TableVerdict
{
    bool bOk;
    bool bThrown;
    std::string sLine;
    std::vector<std::string> kProblems;
};

std::string VerdictText(const TableVerdict& kVerdict);

typedef std::map<std::string, std::vector<std::string> > KeyedRows;

bool ReadKeyed(const FileText& kText, const std::string& sName, const std::string& sDir, int iKeyCols, bool bOffThread,
               const std::string& sSkipRx, KeyedRows& kRows, std::string& sWhy);

TableVerdict CompareTable(const std::string& sLabel, const std::string& sName, int iKeyCols, bool bOffThread,
                          const std::string& sSkipRx, const std::string& sBaseDir, const FileText& kBase,
                          const std::string& sTablesDir, const FileText& kNew);
TableVerdict CompareMemPeaksKeys(const std::string& sLabel, const std::string& sName, const std::string& sPair,
                                 const std::string& sBins, const std::string& sBaseX86, const FileText& kPair,
                                 const std::string& sTablesDir, const FileText& kNew, const std::string& sBaseX64,
                                 const FileText& kBins);
TableVerdict CompareHeapCapsX64(const std::string& sLabel, const std::string& sX64Path, const FileText& kX64,
                                const std::string& sX86Path, const FileText& kX86, const std::string& sRunLog,
                                const FileText& kRunLog);
TableVerdict CompareTableBytes(const std::string& sLabel, const std::string& sName, const std::string& sBaseDir,
                               const FileText& kBase, const std::string& sTablesDir, const FileText& kNew);

struct CompareInput
{
    std::string sPlatform;
    std::string sBaseX86;
    std::string sBaseX64;
    std::string sTables;
    std::string sRunLog;
    RepoTextFn fnRead;
};

std::string BaselineDirFor(const std::string& sName, const CompareInput& kIn);
std::vector<TableVerdict> CompareMode(ReaderKind eKind, const CompareInput& kIn);
TableVerdict CompareEntry(const Reader& kReader, const CompareInput& kIn);
TableVerdict CompareHeapCaps(const Reader& kReader, const CompareInput& kIn);
bool ModeFailed(const std::vector<TableVerdict>& kVerdicts);

}
