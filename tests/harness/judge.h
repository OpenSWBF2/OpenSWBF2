#pragma once
#include <string>
#include <vector>

#include "harness/expect.h"
#include "harness/logparse.h"

struct RunFacts
{
    ParsedLog kLog;
    unsigned long uExitCode = 0;
    bool bKilled = false;
    int iDumps = 0;
    bool bAsanReport = false;
};

struct RunExpect
{
    int iExitCode = 0;
    bool bFatal = false;
    bool bEndsEarly = false;
    int iDumps = 0;
};

struct RunVerdict
{
    bool bPass = true;
    std::string sReason;
    std::vector<std::string> kErrors;
};

RunVerdict JudgeRun(const RunFacts& kFacts, const RunExpect& kExpect, ExpectedMessages& kMessages);
