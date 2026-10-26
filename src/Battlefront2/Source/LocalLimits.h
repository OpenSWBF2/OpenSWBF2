#pragma once

namespace LocalLimits
{

const int kMaxPosts = 16;
const int kMaxNdObjects = 256;
const int kNdIdBase = 64;
const int kMaxTeams = 8;
const int kMaxTeamMembers = 600;
const unsigned int kLobbyNameSize = 64;
const unsigned int kLobbyTagSize = 32;
const bool kUploadsDefault = false;

inline void CopyLobbyString(char* pcDst, const char* pcSrc, unsigned int uiSize)
{
    if (!pcDst || uiSize == 0)
    {
        return;
    }
    unsigned int uiIndex = 0;
    if (pcSrc)
    {
        while (uiIndex + 1 < uiSize && pcSrc[uiIndex] != '\0')
        {
            pcDst[uiIndex] = pcSrc[uiIndex];
            uiIndex++;
        }
    }
    pcDst[uiIndex] = '\0';
}

inline bool UploadsAllowed(bool bSwitch)
{
    return bSwitch;
}

inline bool IsValidClientOf(int iClientOf)
{
    return iClientOf == 1 || iClientOf == 2;
}

inline int ChoosePostIndex(int iForced, int iFound, int iCount)
{
    if (iForced >= 0)
    {
        return iForced < kMaxPosts ? iForced : -1;
    }
    if (iFound >= 0)
    {
        return iFound;
    }
    return iCount >= 0 && iCount < kMaxPosts ? iCount : -1;
}

inline bool CanAddNdObject(unsigned int uiCount)
{
    return uiCount < (unsigned int)kMaxNdObjects;
}

inline bool IsValidNdId(int iUniqueId)
{
    return (unsigned int)(iUniqueId - kNdIdBase) < (unsigned int)kMaxNdObjects;
}

inline bool IsValidTeam(int iTeam)
{
    return (unsigned int)iTeam < (unsigned int)kMaxTeams;
}

inline bool CanAddTeamMember(int iTeam, int iCount)
{
    return (unsigned int)iTeam < (unsigned int)kMaxTeams && iCount >= 0 && iCount < kMaxTeamMembers;
}

}
