#include <windows.h>
#include <delayimp.h>
#include <wchar.h>

#include "RedEngineFL/Util/RedLog.h"
#include "galaxy_api.h"
#include "gfVendor.h"
#include "steamworks.h"

namespace
{

struct ServiceState
{
    bool bOn;
    const char* pcDll;
    char acReason[48];
};

const unsigned int kPathChars = 1024;

bool sProbed;
ServiceState sServices[2];
char sacDelayLoadFailure[64];
unsigned int suSteamInterfaces;

auto WINAPI DelayLoadFailure(unsigned, PDelayLoadInfo pdli) -> decltype(pdli->pfnCur)
{
    if (pdli && pdli->szDll)
    {
        size_t i = 0;
        for (; pdli->szDll[i] != 0 && i < sizeof(sacDelayLoadFailure) - 1; ++i)
        {
            sacDelayLoadFailure[i] = pdli->szDll[i];
        }
        sacDelayLoadFailure[i] = 0;
    }
    return 0;
}

int BindFilter(EXCEPTION_POINTERS* pInfo, DWORD* pdwError)
{
    const DWORD dwCode = pInfo->ExceptionRecord->ExceptionCode;
    if (dwCode != VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND) &&
        dwCode != VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    *pdwError = dwCode & 0xFFFF;
    if (pInfo->ExceptionRecord->NumberParameters >= 1)
    {
        const PDelayLoadInfo pdli = (PDelayLoadInfo)pInfo->ExceptionRecord->ExceptionInformation[0];
        if (pdli && pdli->dwLastError != 0)
        {
            *pdwError = pdli->dwLastError;
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

DWORD BindDll(const char* pcName)
{
    DWORD dwError = 0;
    __try
    {
        const HRESULT hr = __HrLoadAllImportsForDll(pcName);
        if (FAILED(hr))
        {
            dwError = HRESULT_CODE(hr);
        }
    }
    __except (BindFilter(GetExceptionInformation(), &dwError))
    {
    }
    return dwError;
}

bool FileInExeFolder(const wchar_t* pwcName)
{
    wchar_t awcPath[kPathChars];
    const DWORD uLen = GetModuleFileNameW(0, awcPath, kPathChars);
    if (uLen == 0 || uLen >= kPathChars)
    {
        return true;
    }
    wchar_t* pwcSlash = wcsrchr(awcPath, L'\\');
    if (!pwcSlash || (size_t)(pwcSlash + 1 - awcPath) + wcslen(pwcName) >= kPathChars)
    {
        return true;
    }
    wcscpy(pwcSlash + 1, pwcName);
    return GetFileAttributesW(awcPath) != INVALID_FILE_ATTRIBUTES;
}

bool ProbeDll(const char* pcName, ServiceState& kState)
{
    wchar_t awcName[64];
    size_t i = 0;
    for (; pcName[i] != 0 && i < 63; ++i)
    {
        awcName[i] = (wchar_t)(unsigned char)pcName[i];
    }
    awcName[i] = 0;
    if (LoadLibraryExW(awcName, 0, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32))
    {
        return true;
    }
    const DWORD dwError = GetLastError();
    kState.pcDll = pcName;
    gf::vendor::LoadFailureReason(FileInExeFolder(awcName), dwError, kState.acReason, sizeof(kState.acReason));
    return false;
}

bool ProbeAndBind(const char* pcName, ServiceState& kState)
{
    if (!ProbeDll(pcName, kState))
    {
        return false;
    }
    const DWORD dwError = BindDll(pcName);
    if (dwError == 0)
    {
        return true;
    }
    kState.pcDll = pcName;
    gf::vendor::LoadFailureReason(true, dwError, kState.acReason, sizeof(kState.acReason));
    return false;
}

const char* LoadedPath(const wchar_t* pwcModule, char* pcOut, int iSize)
{
    HMODULE hModule = GetModuleHandleW(pwcModule);
    if (!hModule)
    {
        return "none";
    }
    wchar_t awcPath[kPathChars];
    const DWORD uLen = GetModuleFileNameW(hModule, awcPath, kPathChars);
    if (uLen == 0 || uLen >= kPathChars ||
        WideCharToMultiByte(CP_ACP, 0, awcPath, -1, pcOut, iSize, 0, 0) == 0)
    {
        return "unknown";
    }
    return pcOut;
}

}

extern "C" const PfnDliHook __pfnDliFailureHook2 = DelayLoadFailure;

void gf::vendor::Probe()
{
    if (sProbed)
    {
        return;
    }
    sProbed = true;
    ServiceState& kGalaxy = sServices[SERVICE_GALAXY];
    kGalaxy.bOn = ProbeAndBind(DllName("Galaxy.dll", "Galaxy64.dll"), kGalaxy) &&
                  ProbeDll(DllName("GalaxyPeer.dll", "GalaxyPeer64.dll"), kGalaxy);
    ServiceState& kSteam = sServices[SERVICE_STEAM];
    kSteam.bOn = ProbeAndBind(DllName("steam_api.dll", "steam_api64.dll"), kSteam);
}

const char* gf::vendor::LastDelayLoadFailure()
{
    return sacDelayLoadFailure;
}

bool gf::vendor::IsOn(Service eService)
{
    return sServices[eService].bOn;
}

void gf::vendor::WriteLog()
{
    static const char* const apcServices[2] = {"steam", "galaxy"};
    for (int i = 0; i < 2; ++i)
    {
        if (!sServices[i].bOn)
        {
            LOG(LogCore, Warning, "OPENSWBF2 VENDOR %s off: %s %s", apcServices[i],
                sServices[i].pcDll ? sServices[i].pcDll : "none", sServices[i].acReason);
        }
    }
    char acD3dx[kPathChars];
    LOG(LogCore, Inform, "OPENSWBF2 VENDOR steam=%s galaxy=%s d3dx9=%s msvcr120=%d msvcp120=%d",
        sServices[SERVICE_STEAM].bOn ? "on" : "off", sServices[SERVICE_GALAXY].bOn ? "on" : "off",
        LoadedPath(L"d3dx9_43.dll", acD3dx, (int)sizeof(acD3dx)), GetModuleHandleW(L"msvcr120.dll") != 0 ? 1 : 0,
        GetModuleHandleW(L"msvcp120.dll") != 0 ? 1 : 0);
    for (int i = 0; i < CSteamAPIContext::kInterfaceCount; ++i)
    {
        if (suSteamInterfaces & (1u << i))
        {
            LOG(LogCore, Inform, "OPENSWBF2 STEAM iface=%s ok", CSteamAPIContext::InterfaceName(i));
        }
    }
}

void gf::vendor::SetSteamInterfaces(unsigned int uMask)
{
    suSteamInterfaces = uMask;
}

galaxy::api::IGalaxy* gf::vendor::GalaxyCreateInstance()
{
    if (!sServices[SERVICE_GALAXY].bOn)
    {
        return 0;
    }
    return galaxy::api::GalaxyFactory::CreateInstance();
}

galaxy::api::IGalaxy* gf::vendor::GalaxyGetInstance()
{
    if (!sServices[SERVICE_GALAXY].bOn)
    {
        return 0;
    }
    return galaxy::api::GalaxyFactory::GetInstance();
}

void gf::vendor::GalaxyResetInstance()
{
    if (sServices[SERVICE_GALAXY].bOn)
    {
        galaxy::api::GalaxyFactory::ResetInstance();
    }
}

galaxy::api::IErrorManager* gf::vendor::GalaxyGetErrorManager()
{
    if (!sServices[SERVICE_GALAXY].bOn)
    {
        return 0;
    }
    return galaxy::api::GalaxyFactory::GetErrorManager();
}
