#pragma once

namespace pcProgress
{
    void Open();
    void Turn();
    void Step();
    bool StallRequested();
    void FormatName(char* pcOut, unsigned int uiSize, unsigned long ulPid);
}
