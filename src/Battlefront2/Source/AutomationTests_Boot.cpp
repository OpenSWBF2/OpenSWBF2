#ifdef OPENSWBF2_TEST

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "AutomationTest.h"
#include "pcCrash.h"
#include "RedEngineFL/Memory/RedMemory.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBootAddrSpaceTest, "Boot.AddrSpace", EAutomationTestFlags::ProductFilter)

bool FBootAddrSpaceTest::RunTest(char const* pcParameters)
{
    char const* pcFault = pcCrash::HogLayoutFault();
    if (pcFault)
    {
        AddError("%s", pcFault);
    }
    return TestEqual("parameters", pcParameters, "");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBootHeapCapsTest, "Boot.HeapCaps", EAutomationTestFlags::ProductFilter)

bool FBootHeapCapsTest::RunTest(char const* pcParameters)
{
    char acNames[16][32];
    unsigned long aulCaps[16];
    bool abSeen[16];
    int iCount = 0;
    char const* pc = pcParameters;
    while (*pc)
    {
        char const* pcColon = strchr(pc, ':');
        char const* pcEnd = strchr(pc, ',');
        if (!pcEnd)
        {
            pcEnd = pc + strlen(pc);
        }
        if (!pcColon || pcColon >= pcEnd || pcColon == pc || pcColon - pc >= (int)sizeof(acNames[0]) || iCount == 16)
        {
            AddError("Boot.HeapCaps needs parameters Name:cap,Name:cap, got \"%s\"", pcParameters);
            return false;
        }
        memcpy(acNames[iCount], pc, pcColon - pc);
        acNames[iCount][pcColon - pc] = 0;
        char* pcNumberEnd = 0;
        aulCaps[iCount] = strtoul(pcColon + 1, &pcNumberEnd, 10);
        if (pcNumberEnd != pcEnd || pcColon + 1 == pcEnd)
        {
            AddError("Boot.HeapCaps cap of %s is not a number in \"%s\"", acNames[iCount], pcParameters);
            return false;
        }
        abSeen[iCount] = false;
        ++iCount;
        pc = *pcEnd ? pcEnd + 1 : pcEnd;
    }
    TestTrue("Boot.HeapCaps has expected caps", iCount > 0);
    for (int iHeap = 0; iHeap < maxheaps; ++iHeap)
    {
        RedHeapStats kStats;
        if (!RedGetHeapStats(iHeap, &kStats))
        {
            continue;
        }
        char const* pcName = RedGetHeapName(iHeap);
        int iFound = -1;
        for (int i = 0; i < iCount && iFound < 0; ++i)
        {
            if (pcName && strcmp(pcName, acNames[i]) == 0)
            {
                iFound = i;
            }
        }
        if (iFound < 0)
        {
            AddError("heap %s cap %u has no expected cap", pcName ? pcName : "?", kStats.uCap);
            continue;
        }
        abSeen[iFound] = true;
        if (kStats.uCap != aulCaps[iFound])
        {
            AddError("heap %s cap %u, expected %lu", acNames[iFound], kStats.uCap, aulCaps[iFound]);
        }
    }
    for (int i = 0; i < iCount; ++i)
    {
        if (!abSeen[i])
        {
            AddError("heap %s expected with cap %lu is not live", acNames[i], aulCaps[i]);
        }
    }
    return true;
}

#endif
