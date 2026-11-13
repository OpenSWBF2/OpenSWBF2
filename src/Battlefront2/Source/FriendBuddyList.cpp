#include "NetGame.h"
#include "FriendBuddyList.h"
#include "Steam/gfGalaxy.h"

namespace FriendBuddyList
{

// FUNCTION: SWBF2 0x0052e140
unsigned int GetFriendBuddyCount(const int playerIndex)
{
    return gf::friends::friendCount();
}
}

namespace FriendBuddyList
{

// FUNCTION: SWBF2 0x0052e150
int GetFriendBuddy(const int playerIndex, const unsigned int friendBuddyIndex)
{
    galaxy::api::GalaxyID id = gf::friends::friendByIndex(friendBuddyIndex);
    if (id.value)
    {
        return NetGame::FindPlayerByGSID((int)id.value, (int)(id.value >> 32));
    }
    return 0;
}
}

namespace FriendBuddyList
{

// FUNCTION: SWBF2 0x0052e190
bool IsFriendBuddy(const int localPlayerIndex, const int remotePlayerIndex)
{
    const unsigned int count = FriendBuddyList::GetFriendBuddyCount(localPlayerIndex);
    for (unsigned int i = 0; i < count; ++i)
    {
        if (FriendBuddyList::GetFriendBuddy(localPlayerIndex, i) == remotePlayerIndex)
        {
            return true;
        }
    }
    return false;
}
}
