#include "PblHermite.h"

// FUNCTION: SWBF2 0x0072a6e0
PblHermiteVector3::PblHermiteVector3(const PblVector3& vP0, const PblVector3& vP1, const PblVector3& vT0,
                                     const PblVector3& vT1, float fScale)
{
    Set(vP0, vP1, vT0, vT1, fScale);
}

// FUNCTION: SWBF2 0x0072a710
void PblHermiteVector3::Set(const PblVector3& vP0, const PblVector3& vP1, const PblVector3& vT0,
                            const PblVector3& vT1, float fScale)
{
    _a = ((vP0 - vP1) * 2.0f + (vT1 + vT0) * fScale) / (fScale * fScale * fScale);
    _b = ((vP1 - vP0) * 3.0f - (vT1 + vT0 * 2.0f) * fScale) / (fScale * fScale);
    _c = vT0;
    _d = vP0;
}
