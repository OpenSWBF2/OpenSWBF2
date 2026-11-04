#ifndef GAMEVOICECHATTRANSPORT_H
#define GAMEVOICECHATTRANSPORT_H

#include "VoiceChatTransport.h"

namespace GameVoiceChat
{
class Transport : public VoiceChat::Transport
{
public:

    enum Mask
    {
        mask_Invalid = 0,
        mask_TeamFriend = 1,
        mask_TeamEnemy = 2,
        mask_FriendsBuddies = 3,
        mask_Everyone = 4,
        mask_Noone = 5,
        mask_Custom = 6,
        mask_Max = 7
    };

    struct NetPlayerTeamInfo
    {
        int player;
        int team;
    };

    Transport() {}
    // SYNTHETIC: SWBF2 0x0053c030
    // GameVoiceChat::Transport::~Transport

    void SetSendEnableUsingMask(unsigned char node, Mask mask, bool bEnable, bool bKeepCurrent);
    void SetReceiveEnableUsingMask(unsigned char node, Mask mask, bool bEnable, bool bKeepCurrent);

protected:

    void SetSendReceiveEnableUsingMask(const unsigned char nodeID, const Mask mask, bool enable,
                                       const bool send, const bool maskExisting);

private:

    unsigned int BuildPlayerTeamInfo(NetPlayerTeamInfo* teamInfo, unsigned int teamInfoSize,
                                     const int queryNode, int* queryTeam) const;
};

extern Transport gTransport;
}

#endif
