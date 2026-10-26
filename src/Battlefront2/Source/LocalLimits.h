#pragma once

namespace LocalLimits
{

const int kMaxPosts = 16;
const int kMaxNdObjects = 256;
const int kNdIdBase = 64;
const int kMaxTeams = 8;
const int kMaxTeamMembers = 600;

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
