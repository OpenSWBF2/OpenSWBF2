#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Gate
{

void ReadListNote(const std::string& sPath);

struct ReadCheckInput
{
    std::string sStep;
    std::string sRoot;
    std::vector<std::string> kReads;
    std::set<std::string> kClosure;
    const std::map<std::string, std::string>* pTree;
    std::vector<std::string> kArgs;
    std::string sReadsText;
};

std::vector<std::string> ReadCheck(const ReadCheckInput& kIn);

}
