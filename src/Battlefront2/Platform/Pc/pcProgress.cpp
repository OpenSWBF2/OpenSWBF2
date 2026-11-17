#include <windows.h>
#include <stdio.h>

#include "pcProgress.h"

struct ProgressBlock
{
    volatile LONG lTurns;
    volatile LONG lSteps;
    volatile LONG lStall;
};

static ProgressBlock sFallback;
static ProgressBlock* volatile spBlock;
static HANDLE shMap;

static ProgressBlock* Block()
{
    ProgressBlock* pBlock = spBlock;
    return pBlock != 0 ? pBlock : &sFallback;
}

void pcProgress::FormatName(char* pcOut, unsigned int uiSize, unsigned long ulPid)
{
    if (pcOut == 0 || uiSize == 0)
    {
        return;
    }
    _snprintf(pcOut, uiSize, "Local\\OpenSWBF2Progress.%lu", ulPid);
    pcOut[uiSize - 1] = 0;
}

void pcProgress::Open()
{
    if (spBlock != 0)
    {
        return;
    }
    char acName[64];
    FormatName(acName, sizeof(acName), GetCurrentProcessId());
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 4096, acName);
    if (hMap == 0)
    {
        return;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMap);
        return;
    }
    void* pView = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 4096);
    if (pView == 0)
    {
        CloseHandle(hMap);
        return;
    }
    shMap = hMap;
    spBlock = (ProgressBlock*)pView;
}

void pcProgress::Turn()
{
    InterlockedIncrement(&Block()->lTurns);
}

void pcProgress::Step()
{
    InterlockedIncrement(&Block()->lSteps);
}

bool pcProgress::StallRequested()
{
    return Block()->lStall != 0;
}
