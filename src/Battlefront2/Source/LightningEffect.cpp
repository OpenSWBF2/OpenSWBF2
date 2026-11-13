#include <d3dx9math.h>
#include "LightningEffect.h"
#include "CameraManager.h"
#include "LightningBoltEffect.h"
#include "SkyDome.h"
#include "PebbleFL/Common/PblConfig.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblRandom.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/RedCamera.h"
#include "RedEngineFL/Graphics/RedDirectionalLight.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcTerrainShader.h"
#include "SoundFL/source/sndenginebase.h"
#include "SoundFL/source/sndspace.h"

#undef PlaySound


namespace
{



inline unsigned char ClampColorComponent(float f)
{
    if (f < 0.0f)
    {
        f = 0.0f;
    }
    else if (f > 255.0f)
    {
        f = 255.0f;
    }
    return (unsigned char)(int)f;
}
}

// FUNCTION: SWBF2 0x00572830
bool LightningEffect::IsReflected()
{
    return false;
}

// FUNCTION: SWBF2 0x00572840
void LightningEffect::Render(unsigned int) const
{
}

// GLOBAL: SWBF2 0x01e573e4
LightningEffect* LightningEffect::s_pInstance;

// FUNCTION: SWBF2 0x00572850
void LightningEffect::Create()
{
    if (!s_pInstance)
    {
        s_pInstance = new LightningEffect();
    }
}

// FUNCTION: SWBF2 0x005728c0
void LightningEffect::Destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

// FUNCTION: SWBF2 0x005728e0
LightningEffect::LightningEffect()
    : FLEffect(0xE812F405u /* "Lightning" */, RENDERABLE_POSTWORLD)
{
    Enable(false);
    m_colorLightning = RedColorValue(0.937254965f, 0.972549081f, 1.0f, 1.0f);
    m_fSunlightFadeMax = 0.1f;
    m_fSkyDomeDarken = 0.4f;
    m_fBrightnessMin = 0.9f;
    m_fTimeToFade = 0.2f;
    m_fTimeBetweenFlashesMin = 3.0f;
    m_fTimeBetweenFlashesMax = 8.0f;
    m_fTimeBetweenSubFlashesMin = 0.08f;
    m_fTimeBetweenSubFlashesMax = 0.16f;
    m_iNumSubFlashesMin = 2;
    m_iNumSubFlashesMax = 5;
    m_fHorizonAngleMin = 0.523598790f;
    m_fHorizonAngleMax = 1.04719758f;
    m_fTimer = 0.0f;
    m_fTimeNextFlash = 8.0f;
    m_iNumSubFlashesRemaining = 0;
    m_bInitRestore = false;
    mBoltFrequencyMin = 4.0f;
    mBoltFrequencyMax = 7.0f;
    mBoltTimer = 0.0f;
}

// SYNTHETIC: SWBF2 0x00572a00
// LightningEffect::`scalar deleting destructor'

// FUNCTION: SWBF2 0x00572a30
LightningEffect::~LightningEffect()
{
}

// FUNCTION: SWBF2 0x00572a80
void LightningEffect::ReadConfig(PblConfig& config)
{
    while (!config.IsDone())
    {
        PblConfig::Data data = config.ReadNextData();
        switch (data.GetId())
        {
        case 0xAF8BB8CEu: /* "Enable" */
            Enable(data.GetUint32Arg(0) != 0);
            break;
        case 0x3D7E6258u: /* "Color" */
        {
            RedColor color;
            color.r = ClampColorComponent(data.GetFloatArg(0));
            color.g = ClampColorComponent(data.GetFloatArg(1));
            color.b = ClampColorComponent(data.GetFloatArg(2));
            color.a = 255;
            m_colorLightning = RedColorValue(color);
            break;
        }
        case 0x15F3DEB6u: /* "SunlightFadeFactor" */
            m_fSunlightFadeMax = data.GetFloatArg(0);
            break;
        case 0x034AEBFDu: /* "SkyDomeDarkenFactor" */
            m_fSkyDomeDarken = data.GetFloatArg(0);
            break;
        case 0xDB50529Eu: /* "BrightnessMin" */
            m_fBrightnessMin = data.GetFloatArg(0);
            break;
        case 0xA40F930Au: /* "FadeTime" */
            m_fTimeToFade = data.GetFloatArg(0);
            break;
        case 0x260E9234u: /* "TimeBetweenFlashesMinMax" */
            m_fTimeBetweenFlashesMin = data.GetFloatArg(0);
            m_fTimeBetweenFlashesMax = data.GetFloatArg(1);
            break;
        case 0xD71D3C32u: /* "TimeBetweenSubFlashesMinMax" */
            m_fTimeBetweenSubFlashesMin = data.GetFloatArg(0);
            m_fTimeBetweenSubFlashesMax = data.GetFloatArg(1);
            break;
        case 0x67F9208Du: /* "NumSubFlashesMinMax" */
            m_iNumSubFlashesMin = data.GetUint32Arg(0);
            m_iNumSubFlashesMax = data.GetUint32Arg(1);
            break;
        case 0xFC6B14B1u: /* "HorizonAngleMinMax" */
            m_fHorizonAngleMin = data.GetFloatArg(0) * 0.017453292f;
            m_fHorizonAngleMax = data.GetFloatArg(1) * 0.017453292f;
            break;
        case 0xE46BF7A2u: /* "SoundCrack" */
            m_soundCrack.SetID(data.GetStringArg(0), false, false);
            break;
        case 0x882E50B8u: /* "SoundSubCrack" */
            m_soundSubCrack.SetID(data.GetStringArg(0), false, false);
            break;
        }
    }
    m_fTimer = 0.0f;
    m_fTimeNextFlash = m_fTimeBetweenFlashesMax;
    m_iNumSubFlashesRemaining = 0;
    m_bInitRestore = false;
}

// FUNCTION: SWBF2 0x00572d40
void LightningEffect::PlaySound(GameSound& sound)
{
    bool bOutdoors = false;
    for (int i = 0; i < Snd::EngineBase::kMaxListeners; ++i)
    {
        const Snd::Listener& listener = Snd::EngineBase::smListeners[i];
        bOutdoors |= listener.Enabled() && listener.GetSpace() == &Snd::Space::smDefaultSpace;
    }
    if (bOutdoors)
    {
        sound.Play(0, 0, false, true);
    }
}

// FUNCTION: SWBF2 0x00572de0
void LightningEffect::Update(float fDeltaTime)
{
    if (!m_bInitRestore)
    {
        m_colorSunlight_Restore = RedLight::GetGlobalDirectionalLight(0)->m_color;
        m_colorAmbient_Restore = RedLight::GetGlobalAmbientLight()->m_color;
        RedLight::GetGlobalDirectionalAmbientColors(m_colorDirectionalAmbient_Restore[0],
                                                    m_colorDirectionalAmbient_Restore[1]);
        m_vSunlightDir_Restore = RedLight::GetGlobalDirectionalLight(0)->m_vDirection;
        m_fSunlightBrightness = (float)((unsigned int)m_colorSunlight_Restore.r +
                                        (unsigned int)m_colorSunlight_Restore.g +
                                        (unsigned int)m_colorSunlight_Restore.b) / 765.0f;
        m_colorFog_Restore = RedRenderer::GetFogColor();
        m_bInitRestore = true;
    }

    float fBoltHeight = 300.0f;
    float fBoltDist = 800.0f;

    m_fTimer += fDeltaTime;
    if (m_fTimer > m_fTimeNextFlash)
    {
        m_fTimer = 0.0f;
        if (m_iNumSubFlashesRemaining <= 0)
        {
            float fElevation = (m_fHorizonAngleMax - m_fHorizonAngleMin) * PblRandom::_Global.GetFloat() +
                               m_fHorizonAngleMin;
            float fHeading = PblRandom::_Global.GetFloat() * PblMath::fTwoPi;
            float fSinElevation, fCosElevation, fSinHeading, fCosHeading;
            PblMath::SinCos(&fSinElevation, &fCosElevation, fElevation);
            PblMath::SinCos(&fSinHeading, &fCosHeading, fHeading);
            m_vDir = PblVector3(-(fSinHeading * fCosElevation), -fSinElevation, -(fCosHeading * fCosElevation));

            m_iNumSubFlashesRemaining =
                (int)(((float)(m_iNumSubFlashesMax - m_iNumSubFlashesMin) + 0.9999f) *
                          PblRandom::_Global.GetFloat() +
                      (float)m_iNumSubFlashesMin);
            PlaySound(m_soundCrack);
            fBoltDist = 200.0f;
            fBoltHeight = 200.0f;
            mBoltTimer = 0.0f;
        }
        --m_iNumSubFlashesRemaining;
        if (m_iNumSubFlashesRemaining > 0)
        {
            m_fBrightness = (1.0f - m_fBrightnessMin) * PblRandom::GetGlobalFloat() + m_fBrightnessMin;
            m_fTimeNextFlash = (m_fTimeBetweenSubFlashesMax - m_fTimeBetweenSubFlashesMin) *
                                   PblRandom::_Global.GetFloat() +
                               m_fTimeBetweenSubFlashesMin;
            PlaySound(m_soundSubCrack);
        }
        else
        {
            m_fBrightness = (1.0f - m_fBrightnessMin) * PblRandom::GetGlobalFloat() + m_fBrightnessMin;
            m_fTimeNextFlash = (m_fTimeBetweenFlashesMax - m_fTimeBetweenFlashesMin) *
                                   PblRandom::_Global.GetFloat() +
                               m_fTimeBetweenFlashesMin;
        }
    }

    if (m_fTimer >= m_fTimeToFade)
    {
        if (m_fTimer - fDeltaTime < m_fTimeToFade)
        {
            RedLight::SetGlobalDirectionalLightColor(m_colorSunlight_Restore, 0);
            RedLight::SetGlobalAmbientLightColor(m_colorAmbient_Restore);
            RedLight::SetGlobalDirectionalAmbientColors(m_colorDirectionalAmbient_Restore[0],
                                                        m_colorDirectionalAmbient_Restore[1]);
            {
            const float t = m_fSkyDomeDarken;
            RedColor c;
            c.b = (unsigned char)(int)(RedColor::WHITE.b * (1.0f - t) + RedColor::BLACK.b * t + 0.5f);
            c.g = (unsigned char)(int)(RedColor::WHITE.g * (1.0f - t) + RedColor::BLACK.g * t + 0.5f);
            c.r = (unsigned char)(int)(RedColor::WHITE.r * (1.0f - t) + RedColor::BLACK.r * t + 0.5f);
            c.a = (unsigned char)(int)(RedColor::WHITE.a * (1.0f - t) + RedColor::BLACK.a * t + 0.5f);
            SkyDome::sInstance->SetSkyDomeColor(c);
            }
            RedRenderer::SetFogColor(m_colorFog_Restore);
            pcTerrainShader::_SetLightningValue(0.0f);
        }
    }
    else
    {
        float fFade = m_fTimer / m_fTimeToFade;
        float fSunlight = fFade * m_fSunlightBrightness;
        float fFlash = (1.0f - fFade) * m_fBrightness;
        float fBrightness = (PblRandom::_Global.GetFloat() - 0.5f) * (1.0f - fFlash) * fFlash + fFlash;
        if (fBrightness > fSunlight)
        {
            float fLightning = fBrightness - fSunlight;
            float fDarken = 1.0f - m_fSunlightFadeMax;

            RedColorValue colorAmbient = m_colorAmbient_Restore * fDarken
                + m_colorSunlight_Restore * 0.333f * fDarken;
            colorAmbient.a = 1.0f;
            RedColorValue colorTop = m_colorDirectionalAmbient_Restore[0] * fDarken
                + m_colorSunlight_Restore * 0.333f * fDarken;
            colorTop.a = 1.0f;
            RedColorValue colorBottom = m_colorDirectionalAmbient_Restore[1] * fDarken
                + m_colorSunlight_Restore * 0.333f * fDarken;
            colorBottom.a = 1.0f;

            float fScale = 1.0f - fLightning;
            RedColorValue colorSunlight = m_colorLightning * fScale;

            RedLight::SetGlobalAmbientLightColor(colorAmbient);
            RedLight::SetGlobalDirectionalAmbientColors(colorTop, colorBottom);
            RedLight::SetGlobalDirectionalLightColor(colorSunlight, 0);
            pcTerrainShader::_SetLightningValue(fLightning * 0.15f);
        }
        else
        {
            float fScale = 1.0f - (fSunlight - fBrightness) / m_fSunlightBrightness;
            float fDarken = 1.0f - m_fSunlightFadeMax;

            RedColorValue colorAmbient = m_colorAmbient_Restore * fDarken
                + m_colorSunlight_Restore * 0.333f * fScale * fDarken;
            RedColorValue colorTop = m_colorDirectionalAmbient_Restore[0] * fDarken
                + m_colorSunlight_Restore * 0.333f * fScale * fDarken;
            RedColorValue colorBottom = m_colorDirectionalAmbient_Restore[1] * fDarken
                + m_colorSunlight_Restore * 0.333f * fScale * fDarken;
            RedColorValue colorSunlight = m_colorSunlight_Restore * fScale;

            RedLight::SetGlobalAmbientLightColor(colorAmbient);
            RedLight::SetGlobalDirectionalAmbientColors(colorTop, colorBottom);
            RedLight::SetGlobalDirectionalLightColor(colorSunlight, 0);
            pcTerrainShader::_SetLightningValue(0.0f);
        }

        {
        const float t = m_fSkyDomeDarken * (1.0f - fBrightness);
        RedColor c;
        c.b = (unsigned char)(int)(RedColor::WHITE.b * (1.0f - t) + RedColor::BLACK.b * t + 0.5f);
        c.g = (unsigned char)(int)(RedColor::WHITE.g * (1.0f - t) + RedColor::BLACK.g * t + 0.5f);
        c.r = (unsigned char)(int)(RedColor::WHITE.r * (1.0f - t) + RedColor::BLACK.r * t + 0.5f);
        c.a = (unsigned char)(int)(RedColor::WHITE.a * (1.0f - t) + RedColor::BLACK.a * t + 0.5f);
        SkyDome::sInstance->SetSkyDomeColor(c);
        }
        {
        const float t = m_fSkyDomeDarken * (1.0f - fBrightness);
        RedColor c;
        c.b = (unsigned char)(int)(RedColor::WHITE.b * (1.0f - t) + RedColor::BLACK.b * t + 0.5f);
        c.g = (unsigned char)(int)(RedColor::WHITE.g * (1.0f - t) + RedColor::BLACK.g * t + 0.5f);
        c.r = (unsigned char)(int)(RedColor::WHITE.r * (1.0f - t) + RedColor::BLACK.r * t + 0.5f);
        c.a = (unsigned char)(int)(RedColor::WHITE.a * (1.0f - t) + RedColor::BLACK.a * t + 0.5f);
        RedRenderer::SetFogColor(c);
        }
    }

    mBoltTimer -= fDeltaTime;
    if (mBoltTimer < 0.0f)
    {
        FLEffectClass* pClass = FLEffect::Get(0x559A2771u /* "skybolt" */);
        if (pClass)
        {
            FLEffectObject* pEffect = pClass->BuildEffect();
            if (pEffect)
            {
                PblMatrix mCamera = CameraManager::sInstance->GetCamera(0)->GetMatrix();
                PblVector3 vDir;
                vDir.x = PblRandom::GetGlobalFloat() - 0.5f;
                vDir.z = PblRandom::GetGlobalFloat() - 0.5f;
                vDir.y = 0.01f;
                vDir.x -= mCamera.m[2][0];
                vDir.z -= mCamera.m[2][2];
                vDir = Normalize(vDir) * fBoltDist;
                vDir += mCamera.GetTranslation();
                vDir.y = fBoltHeight;
                pEffect->AttachEffectToVector(&vDir, false);

                float fOffsetX = (PblRandom::GetGlobalFloat() - 0.5f) * 20.0f;
                float fOffsetZ = (PblRandom::GetGlobalFloat() - 0.5f) * 20.0f;
                LightningBoltEffectObject* pBolt = (LightningBoltEffectObject*)pEffect;
                pBolt->SetTargetPoint(vDir + PblVector3(fOffsetX, -200.0f, fOffsetZ));
            }
        }
        mBoltTimer = (mBoltFrequencyMax - mBoltFrequencyMin) * PblRandom::_Global.GetFloat() + mBoltFrequencyMin;
    }
}
