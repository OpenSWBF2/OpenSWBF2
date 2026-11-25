#pragma once

#include <d3d9.h>

namespace GammaPass
{

void Route(const D3DGAMMARAMP& ramp, bool bNoRender, bool bTestKind, bool bProg2);
void Draw();
void ReleaseDefault();
void Shutdown();
bool IsEnsuring();
#ifdef OPENSWBF2_TEST
const char* TestPathName();
const char* TestReason();
#endif

}
