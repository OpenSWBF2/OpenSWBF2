#ifdef OPENSWBF2_TEST

#include "AutomationTest.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "CommandLine.h"
#include "RedEngineFL/Util/RedLog.h"

FAutomationTestBase* FAutomationTestBase::sFirst;

FAutomationTestBase::FAutomationTestBase(char const* pcName, int iFlags, char const* pcFile, int iLine)
    : mpNext(sFirst), miErrors(0), mpcName(pcName), miFlags(iFlags), mpcFile(pcFile), miLine(iLine)
{
    sFirst = this;
}

void FAutomationTestBase::AddError(char const* pcFormat, ...)
{
    char acText[512];
    va_list args;
    va_start(args, pcFormat);
    int iLen = _vsnprintf(acText, sizeof(acText) - 1, pcFormat, args);
    va_end(args);
    if (iLen < 0)
    {
        iLen = (int)sizeof(acText) - 1;
    }
    acText[iLen] = 0;
    ++miErrors;
    LOG(LogCore, Error, "%s", acText);
}

bool FAutomationTestBase::TestTrue(char const* pcWhat, bool bValue)
{
    if (!bValue)
    {
        AddError("Expected '%s' to be true.", pcWhat);
    }
    return bValue;
}

bool FAutomationTestBase::TestFalse(char const* pcWhat, bool bValue)
{
    if (bValue)
    {
        AddError("Expected '%s' to be false.", pcWhat);
    }
    return !bValue;
}

bool FAutomationTestBase::TestEqual(char const* pcWhat, int iActual, int iExpected)
{
    if (iActual != iExpected)
    {
        AddError("Expected '%s' to be %d, but it was %d.", pcWhat, iExpected, iActual);
        return false;
    }
    return true;
}

bool FAutomationTestBase::TestEqual(char const* pcWhat, char const* pcActual, char const* pcExpected)
{
    pcActual = pcActual ? pcActual : "";
    pcExpected = pcExpected ? pcExpected : "";
    if (strcmp(pcActual, pcExpected) != 0)
    {
        AddError("Expected '%s' to be \"%s\", but it was \"%s\".", pcWhat, pcExpected, pcActual);
        return false;
    }
    return true;
}

namespace AutomationTest
{

static bool WildMatch(char const* pcPattern, char const* pcPatternEnd, char const* pcName)
{
    if (pcPattern == pcPatternEnd)
    {
        return *pcName == 0;
    }
    if (*pcPattern == '*')
    {
        for (;; ++pcName)
        {
            if (WildMatch(pcPattern + 1, pcPatternEnd, pcName))
            {
                return true;
            }
            if (*pcName == 0)
            {
                return false;
            }
        }
    }
    return *pcName != 0 && tolower((unsigned char)*pcPattern) == tolower((unsigned char)*pcName) &&
           WildMatch(pcPattern + 1, pcPatternEnd, pcName + 1);
}

bool Select(char const* pcFilter, char const* pcName, char* pcParameters, int iParametersSize)
{
    pcParameters[0] = 0;
    char const* pc = pcFilter;
    while (*pc)
    {
        char const* pcEnd = strchr(pc, '+');
        if (!pcEnd)
        {
            pcEnd = pc + strlen(pc);
        }
        char const* pcEquals = (char const*)memchr(pc, '=', pcEnd - pc);
        char const* pcPatternEnd = pcEquals ? pcEquals : pcEnd;
        if (pcPatternEnd > pc && WildMatch(pc, pcPatternEnd, pcName))
        {
            if (pcEquals)
            {
                const int iLen = (int)(pcEnd - pcEquals - 1);
                if (iLen >= iParametersSize)
                {
                    return false;
                }
                memcpy(pcParameters, pcEquals + 1, iLen);
                pcParameters[iLen] = 0;
            }
            return true;
        }
        pc = *pcEnd ? pcEnd + 1 : pcEnd;
    }
    return false;
}

int RunRequested(int iExitCode)
{
    if (!gAutoTestRequested)
    {
        return iExitCode;
    }

    int iRan = 0;
    int iFailed = 0;
    char const* pcLast = 0;
    for (;;)
    {
        FAutomationTestBase* pTest = 0;
        for (FAutomationTestBase* p = FAutomationTestBase::sFirst; p; p = p->mpNext)
        {
            if ((!pcLast || _stricmp(p->GetTestFullName(), pcLast) > 0) &&
                (!pTest || _stricmp(p->GetTestFullName(), pTest->GetTestFullName()) < 0))
            {
                pTest = p;
            }
        }
        if (!pTest)
        {
            break;
        }
        pcLast = pTest->GetTestFullName();

        char acParameters[512];
        if (!Select(gAutoTestFilter, pcLast, acParameters, (int)sizeof(acParameters)))
        {
            continue;
        }

        char const* pcLeaf = strrchr(pcLast, '.');
        pcLeaf = pcLeaf ? pcLeaf + 1 : pcLast;
        ++iRan;
        LOG(LogCore, Inform, "Test Started. Name={%s} Path={%s}", pcLeaf, pcLast);
        pTest->miErrors = 0;
        const bool bPassed = pTest->RunTest(acParameters) && !pTest->HasAnyErrors();
        LOG(LogCore, Inform, "Test Completed. Result={%s} Name={%s} Path={%s}", bPassed ? "Success" : "Fail", pcLeaf,
            pcLast);
        if (!bPassed)
        {
            ++iFailed;
        }
    }

    if (iRan == 0)
    {
        LOG(LogCore, Error, "No automation tests matched '%s'", gAutoTestFilter);
    }
    return (iRan == 0 || iFailed != 0) ? -1 : iExitCode;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHarnessAutoTestPass, "Harness.AutoTest.Pass", EAutomationTestFlags::SmokeFilter)

bool FHarnessAutoTestPass::RunTest(char const* pcParameters)
{
    return TestEqual("parameters", pcParameters, "");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHarnessAutoTestFail, "Harness.AutoTest.Fail", EAutomationTestFlags::NegativeFilter)

bool FHarnessAutoTestFail::RunTest(char const* pcParameters)
{
    AddError("Harness.AutoTest.Fail fails on purpose, parameters \"%s\"", pcParameters);
    return true;
}

#endif
