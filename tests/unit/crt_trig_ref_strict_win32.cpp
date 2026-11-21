#include "unit/crt_trig.h"

#include <math.h>

namespace
{
__declspec(noinline) double PlainSin(double d)
{
    return sin(d);
}

__declspec(noinline) double PlainCos(double d)
{
    return cos(d);
}
}

double CrtTrigPlain(unsigned int uFn, double d)
{
    return uFn == CRT_TRIG_SIN ? PlainSin(d) : PlainCos(d);
}
