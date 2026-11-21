#include "unit/d3dx_corpus.h"

#include "PebbleFL/pc/pcD3dxPort.h"

namespace
{
void PortMatrixRotationX(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(PcD3dxMatrixRotationX, puIn, uVariant, puOut);
}

void PortMatrixRotationY(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(PcD3dxMatrixRotationY, puIn, uVariant, puOut);
}

void PortMatrixRotationZ(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(PcD3dxMatrixRotationZ, puIn, uVariant, puOut);
}

void PortMatrixRotationAxis(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotationAxis(PcD3dxMatrixRotationAxis, puIn, uVariant, puOut);
}

const D3dxCorpusFn sX87Fns[] = {
    {"MatrixRotationX", "a", 1, 17, 1, false, true, PortMatrixRotationX, 0},
    {"MatrixRotationY", "a", 1, 17, 1, false, true, PortMatrixRotationY, 0},
    {"MatrixRotationZ", "a", 1, 17, 1, false, true, PortMatrixRotationZ, 0},
    {"MatrixRotationAxis", "xa", 4, 17, 2, false, true, PortMatrixRotationAxis, 0},
};
}

const D3dxCorpusFn* D3dxX87Fns(unsigned int& uCount)
{
    uCount = sizeof(sX87Fns) / sizeof(sX87Fns[0]);
    return sX87Fns;
}
