#include <d3dx9math.h>
#include <math.h>
#include <string.h>

#include "RedDirectionalLight.h"
#include "RedEngineFL/Application/pc/pcRedDebug.h"
#include "RedEngineFL/Util/RedRegion.h"

#include "RedEngineFL/Application/pc/pcRenderHelpers.h"


static const float EPSILON = 0.0001f;

// FUNCTION: SWBF2 0x006d8c00
RedDirectionalLight::RedDirectionalLight(const PblVector3& vDirection,
                                         const RedColorValue& color)
    : RedLight(TYPE_DIRECTIONAL),
      m_uiRegionName(0), m_boundingRegion(0), m_mProjection(gMatrixIdentity),
      m_bProjectionDirty(true), m_fProjTexOffsetU(1.0f), m_fProjTexOffsetV(1.0f),
      m_fProjTexTileU(1.0f), m_fProjTexTileV(1.0f)
{
    memset(&m_d3dLight, 0, sizeof(m_d3dLight));
    m_d3dLight.Type = D3DLIGHT_DIRECTIONAL;

    SetColor(color);
    SetDirection(vDirection);
    _RecalculateProjectionMatrix();
}

// SYNTHETIC: SWBF2 0x004083a0
// `dynamic initializer for 'RedDirectionalLight::BLACK''
// SYNTHETIC: SWBF2 0x00769990
// `dynamic atexit destructor for 'RedDirectionalLight::BLACK''

// GLOBAL: SWBF2 0x00998300
// RedDirectionalLight::BLACK
RedDirectionalLight RedDirectionalLight::BLACK(PblVector3(0.0f, 0.0f, 0.0f), RedColorValue::BLACK);

// FUNCTION: SWBF2 0x006d8cd0
void RedDirectionalLight::DebugDraw() const
{
    if (!m_boundingRegion)
    {
        return;
    }

    RedColor color;
    color.r = (unsigned char)(int)(m_color.r * 255.0f);
    color.g = (unsigned char)(int)(m_color.g * 255.0f);
    color.b = (unsigned char)(int)(m_color.b * 255.0f);
    color.a = (unsigned char)(int)(m_color.a * 255.0f);

    const PblMatrix& m = m_boundingRegion->mMatrix;
    const PblVector4& size = m_boundingRegion->mSize;

    switch (m_boundingRegion->mType)
    {
    case PblRegion::type_sphere:
        RedDebug::DrawSphere(PblVector3(m.m[3][0], m.m[3][1], m.m[3][2]), size.w, color);
        break;

    case PblRegion::type_box:
    case PblRegion::type_box_aligned:
    {
        PblBox box;
        box._vMin = PblVector3(-size.x, -size.y, -size.z);
        box._vMax = PblVector3(size.x, size.y, size.z);
        RedDebug::DrawOBB(box, m, color);
        break;
    }

    case PblRegion::type_cylinder:
    case PblRegion::type_cylinder_xaligned:
    case PblRegion::type_cylinder_yaligned:
    case PblRegion::type_cylinder_zaligned:
    {
        PblMatrix mBase = m;
        mBase.m[3][0] = m.m[3][0] - m.m[1][0] * size.y;
        mBase.m[3][1] = m.m[3][1] - m.m[1][1] * size.y;
        mBase.m[3][2] = m.m[3][2] - m.m[1][2] * size.y;
        mBase.m[3][3] = 1.0f;
        RedDebug::DrawCone(mBase, size.y * 2.0f, size.x, size.x, color);
        break;
    }

    default:
        break;
    }
}

// FUNCTION: SWBF2 0x006d8f80
bool RedDirectionalLight::IsVisible(const RedCamera* pCamera) const
{
    if (m_boundingRegion)
    {
        return m_boundingRegion->IsVisible(pCamera, 0.0f);
    }
    return true;
}

// FUNCTION: SWBF2 0x006d8fb0
void RedDirectionalLight::SetColor(const RedColorValue& color)
{
    m_color = color;
    m_d3dLight.Diffuse = m_d3dLight.Specular = color;
}

// FUNCTION: SWBF2 0x006d8fe0
void RedDirectionalLight::SetDirection(const PblVector3& vDirection)
{
    float fLengthSq = SquaredLength(vDirection);

    if (EPSILON >= fabsf(fLengthSq - 1.0f))
    {
        m_vDirection = vDirection;
    }
    else
    {
        m_vDirection = Normalize(vDirection);
    }

    m_d3dLight.Direction = *(const D3DVECTOR*)&m_vDirection;
    m_bProjectionDirty = true;
}

// FUNCTION: SWBF2 0x006d9090
void RedDirectionalLight::_RecalculateProjectionMatrix() const
{
    PblVector3 vU = (gVectorUnitX - m_vDirection * m_vDirection.x) / m_fProjTexTileU;
    PblVector3 vV = (gVectorUnitZ - m_vDirection * m_vDirection.z) / m_fProjTexTileV;

    m_mProjection.SetXAxis(vU.x, vV.x, 0.0f, 0.0f);
    m_mProjection.SetYAxis(vU.y, vV.y, 0.0f, 0.0f);
    m_mProjection.SetZAxis(vU.z, vV.z, 1.0f, 0.0f);
    m_mProjection.SetTranslation(m_fProjTexOffsetU / m_fProjTexTileU,
                                 m_fProjTexOffsetV / m_fProjTexTileV, 0.0f, 1.0f);

    m_bProjectionDirty = false;
}
