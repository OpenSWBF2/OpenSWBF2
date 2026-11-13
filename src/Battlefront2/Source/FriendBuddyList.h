#pragma once

namespace FriendBuddyList
{
unsigned int GetFriendBuddyCount(const int playerIndex);
int GetFriendBuddy(const int playerIndex, const unsigned int friendBuddyIndex);
bool IsFriendBuddy(const int localPlayerIndex, const int remotePlayerIndex);
}
