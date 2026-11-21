#include "unit/d3dx_corpus.h"

namespace
{
const char kReason[] = "x87 transcription in ml64, x64 only; the x86 game calls the DLL";

const D3dxCorpusFn sX87Fns[] = {
    {"MatrixRotationX", "a", 1, 17, 1, false, true, 0, kReason},
    {"MatrixRotationY", "a", 1, 17, 1, false, true, 0, kReason},
    {"MatrixRotationZ", "a", 1, 17, 1, false, true, 0, kReason},
    {"MatrixRotationAxis", "xa", 4, 17, 2, false, true, 0, kReason},
};
}

const D3dxCorpusFn* D3dxX87Fns(unsigned int& uCount)
{
    uCount = sizeof(sX87Fns) / sizeof(sX87Fns[0]);
    return sX87Fns;
}
