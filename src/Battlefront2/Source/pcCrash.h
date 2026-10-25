#pragma once

struct _EXCEPTION_POINTERS;

namespace pcCrash
{
    void Install();
    void Reinstall();
    long Filter(struct _EXCEPTION_POINTERS* pInfo);
    void __cdecl Exit(int iCode);
    void SetLogPath(char const* pcPath);
    void SetCrashTest(char const* pcKind);
    void RunCrashTest();
}
