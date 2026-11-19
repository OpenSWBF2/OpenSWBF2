#ifndef GF_VENDOR_H
#define GF_VENDOR_H

#include <stddef.h>
#include <stdio.h>

namespace galaxy
{
namespace api
{
class IGalaxy;
class IErrorManager;
}
}

namespace gf
{
namespace vendor
{

enum Service
{
    SERVICE_STEAM,
    SERVICE_GALAXY
};

void Probe();
bool IsOn(Service eService);
void WriteLog();
void SetSteamInterfaces(unsigned int uMask);
const char* LastDelayLoadFailure();

galaxy::api::IGalaxy* GalaxyCreateInstance();
galaxy::api::IGalaxy* GalaxyGetInstance();
void GalaxyResetInstance();
galaxy::api::IErrorManager* GalaxyGetErrorManager();

inline const char* DllName(const char* pcX86, const char* pcX64)
{
    return sizeof(void*) == 8 ? pcX64 : pcX86;
}

inline const char* LobbyOffText(bool b64Bit)
{
    return b64Bit ? "Online lobbies are not available in the 64-bit build. LAN and direct IP games work."
                  : "Online lobbies are not available because Galaxy could not be loaded. LAN and direct IP games work.";
}

inline void LoadFailureReason(bool bFilePresent, unsigned long dwError, char* pcOut, size_t uSize)
{
    if (dwError == 193)
    {
        snprintf(pcOut, uSize, "wrong bitness");
    }
    else if (!bFilePresent)
    {
        snprintf(pcOut, uSize, "missing");
    }
    else
    {
        snprintf(pcOut, uSize, "load failed (error %lu)", dwError);
    }
}

}
}

#endif
