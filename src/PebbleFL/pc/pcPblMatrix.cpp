#include <d3dx9math.h>
#include <string.h>

#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"

// FUNCTION: SWBF2 0x0062c130
PblMatrix::PblMatrix(const PblVector4& x, const PblVector4& y,
                     const PblVector4& z, const PblVector4& t)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = x.w;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = y.w;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = z.w;
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
    m[3][3] = t.w;
}

#pragma warning(disable : 4073)
#pragma init_seg(lib)
static D3DXMATRIXA16 _IdentityMatrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

// SYNTHETIC: SWBF2 0x0040ae50
// `dynamic initializer for 'gMatrixIdentity''

// GLOBAL: SWBF2 0x009caee0
// gMatrixIdentity
PblMatrix gMatrixIdentity(_IdentityMatrix);

// FUNCTION: SWBF2 0x004384f0
PblMatrix::PblMatrix(const PblVector3& x, const PblVector3& y,
                     const PblVector3& z, const PblVector3& t)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = 0.0f;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = 0.0f;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = 0.0f;
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
    m[3][3] = 1.0f;
}


