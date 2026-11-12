#pragma once

#include <d3d9.h>

#include "PebbleFL/Common/PblList.h"

class PblFile;

#include "pcRedRenderer.h"

namespace RedAutoSettings
{

extern RenderOptionsT _RenderSettings;

RenderOptionsT const& GetRenderOptions();

bool DeviceOptionsFound();
int GetEffectiveProcessorSpeed();
void GetIdealResolution(int& iWidth, int& iHeight);
int GetProcessorSpeed();
bool IsAthlonProcessor();

RenderOptionsT* GetCustomPreset(int iPreset);
void GetCustomPresetResolution(int iPreset, int& iWidth, int& iHeight);
extern RenderOptionsT sCustomRenderPresets[3];
extern bool _bAthlon;
extern int _ProcessorSpeed;
extern unsigned int _NonMaskableCount;
extern char* _NonMaskableStr[8];
extern bool optionsFounds_;

void SetMasterQuality(RenderOptionsT* pOptions, int iLevel);
void _SetGeneration(RenderOptionsT* pOptions);

void _LoadVendorDeviceOverrides();

void DoAutoDetection();

}

unsigned long GetCpuSpeed();

int axtoi(char const* pszHex, int iMaxDigits);

bool atob(char const* pszValue);
BackBufferCopyT atobbc(char const* pszValue);
GeometryModeT atogeom(char const* pszValue);
PipelineT atopipe(char const* pszValue);
Quality2T atoq2(char const* pszValue);
QualityT atoq(char const* pszValue);
SkinningModeT atoskin(char const* pszValue);
ShadowVolumeModeT atosv(char const* pszValue);
TextureCapsT atotexc(char const* pszValue);
TextureNonPow2SupportT atotexpow2(char const* pszValue);
TextureQualityT atotexq(char const* pszValue);
int fskip(PblFile& rFile, char const* pcToken);

namespace RedRenderCapOverrides
{

void _LoadVendorDeviceRenderCapOverrides();

void _ApplyVendorDeviceRenderCapOverrides(RenderCapsT& rCaps);

}

class VidMode
{
public:

    VidMode() {}

    int Width;
    int Height;
    bool bSupported;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    unsigned long MultiSampleQuality;
    int VidMemUsed;

    PblList<VidMode>::Node mNode;
};
 

class VidConfig
{
public:
    VidConfig(unsigned long ulVendorId, unsigned long ulDeviceId, unsigned long ulHint);

    static VidConfig* GetConfig(unsigned long ulVendorId, unsigned long ulDeviceId,
                                unsigned long ulHint);
    static void LoadConfigFile();
    static void SaveConfigFile();

    void AddVidMode(int iWidth, int iHeight, D3DMULTISAMPLE_TYPE eMultiSampleType,
                    unsigned long ulMultiSampleQuality, bool bSupported);
    bool VidModeTested(int iWidth, int iHeight, D3DMULTISAMPLE_TYPE eMultiSampleType,
                       unsigned long ulMultiSampleQuality);

    bool VidModeAvailable(int iWidth, int iHeight, D3DMULTISAMPLE_TYPE eMultiSampleType,
                          unsigned long ulMultiSampleQuality);

    bool GetMaxMultisamplingModeForScreenSize(int iWidth, int iHeight,
                                              D3DMULTISAMPLE_TYPE& rMultiSampleType,
                                              unsigned long& rMultiSampleQuality);
    bool GetMaxScreenSizeForMultisamplingMode(D3DMULTISAMPLE_TYPE eMultiSampleType,
                                              unsigned long ulMultiSampleQuality,
                                              int& rWidth, int& rHeight);

    bool MultisamplingModeAvailable(D3DMULTISAMPLE_TYPE eMultiSampleType,
                                    unsigned long ulMultiSampleQuality)
    {
        for (PblList<VidMode>::Itor mode(VidModeList); mode; ++mode)
        {
            if (mode->MultiSampleType == eMultiSampleType
                && mode->MultiSampleQuality == ulMultiSampleQuality && mode->bSupported)
            {
                return true;
            }
        }
        return false;
    }

    unsigned long mVendorId;
    unsigned long mDeviceId;
    unsigned long mHint;
    PblList<VidMode> VidModeList;
    PblList<VidConfig>::Node mNode;

    static PblList<VidConfig> sConfigs;
};
 

namespace RedVideoConfig
{

extern IDirect3D9* m_pD3D;
extern bool m_MultiSampleDriverControlled;
extern unsigned int m_MemoryHint;

void VidMode_Init(IDirect3D9* pD3D);
void VidMode_StartupVoodoo(IDirect3D9* pD3D);

bool VidMode_IsDriverMultisampling();

bool VidMode_GetMaxMultisamplingModeForScreenSize(int iWidth, int iHeight,
                                                  D3DMULTISAMPLE_TYPE& rMultiSampleType,
                                                  unsigned long& rMultiSampleQuality);
bool VidMode_GetMaxScreenSizeForMultisamplingMode(D3DMULTISAMPLE_TYPE eMultiSampleType,
                                                  unsigned long ulMultiSampleQuality,
                                                  int& rWidth, int& rHeight);
char const* VidMode_GetMultiSampleDescription(D3DMULTISAMPLE_TYPE eMultiSampleType,
                                              unsigned long ulMultiSampleQuality);
int VidMode_GetNumMultiSampleQualityModes();
bool VidMode_MultisamplingModeUseable(D3DMULTISAMPLE_TYPE eMultiSampleType,
                                      unsigned long ulMultiSampleQuality);

bool VidMode_Usable(int iWidth, int iHeight, D3DMULTISAMPLE_TYPE eMultiSampleType,
                    unsigned long ulMultiSampleQuality);

void VidMode_ApplyRenderCapOverrides(RenderCapsT& rCaps);

}
