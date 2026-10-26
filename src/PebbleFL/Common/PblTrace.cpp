#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

char lastTraceMessage[1024];
void (__cdecl* gPblTraceSink)(char const*) = 0;

// FUNCTION: SWBF2 0x0072b3a0
void __cdecl PblTraceP(const char* Format, ...)
{
    char acMessage[0x400];
    va_list ArgList;
    va_start(ArgList, Format);
    _vsnprintf(acMessage, sizeof(acMessage), Format, ArgList);
    va_end(ArgList);
    acMessage[sizeof(acMessage) - 1] = 0;
    lstrcpynA(lastTraceMessage, acMessage, sizeof(lastTraceMessage));

    OutputDebugStringA(acMessage);

    if (gPblTraceSink)
    {
        gPblTraceSink(acMessage);
    }
}
