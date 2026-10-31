#include "third_party/doctest/doctest.h"

#include "Steam/galaxy_api.h"

namespace
{

enum CallId
{
    CALL_NONE,
    CALL_SIGNED_IN,
    CALL_USER_OTHER,
    CALL_REQUEST_FRIEND_LIST,
    CALL_FRIENDS_OTHER
};

class RecUser : public galaxy::api::IUser
{
public:
    RecUser() : last(CALL_NONE), bSignedIn(true) {}

    bool SignedIn() { last = CALL_SIGNED_IN; return bSignedIn; }
    galaxy::api::GalaxyID GetGalaxyID() { last = CALL_USER_OTHER; return galaxy::api::GalaxyID(); }
    void SignInWithServerKey(const char*) { last = CALL_USER_OTHER; }
    void SignInWithXboxOne(unsigned int) { last = CALL_USER_OTHER; }
    void SignInWithPS4(const char*, const char*, const char*, unsigned int) { last = CALL_USER_OTHER; }
    void SignInWithGalaxyCheckConnectivity(bool) { last = CALL_USER_OTHER; }
    void SignInWithSteam(const char*, int, const char*) { last = CALL_USER_OTHER; }

    CallId last;
    bool bSignedIn;
};

class RecFriends : public galaxy::api::IFriends
{
public:
    RecFriends() : last(CALL_NONE) {}

    void RequestUserInformation(galaxy::api::GalaxyID, bool) { last = CALL_FRIENDS_OTHER; }
    const char* GetPersonaName() { last = CALL_FRIENDS_OTHER; return 0; }
    void GetPersonaNameCopy(char*, unsigned int) { last = CALL_FRIENDS_OTHER; }
    int GetPersonaState() { last = CALL_FRIENDS_OTHER; return 0; }
    const char* GetFriendPersonaName(galaxy::api::GalaxyID) { last = CALL_FRIENDS_OTHER; return 0; }
    void GetFriendPersonaNameCopy(galaxy::api::GalaxyID, char*, unsigned int) { last = CALL_FRIENDS_OTHER; }
    int GetFriendPersonaState(galaxy::api::GalaxyID) { last = CALL_FRIENDS_OTHER; return 0; }
    const char* GetFriendAvatarUrl(galaxy::api::GalaxyID, int) { last = CALL_FRIENDS_OTHER; return 0; }
    void GetFriendAvatarUrlCopy(galaxy::api::GalaxyID, int, char*, unsigned int) { last = CALL_FRIENDS_OTHER; }
    unsigned int GetFriendAvatarImageID(galaxy::api::GalaxyID, int) { last = CALL_FRIENDS_OTHER; return 0; }
    void GetFriendAvatarImageRGBA(galaxy::api::GalaxyID, int, void*, unsigned int) { last = CALL_FRIENDS_OTHER; }
    bool IsFriendAvatarImageRGBAAvailable(galaxy::api::GalaxyID, int) { last = CALL_FRIENDS_OTHER; return false; }
    void RequestFriendList() { last = CALL_REQUEST_FRIEND_LIST; }
    bool IsFriend(galaxy::api::GalaxyID) { last = CALL_FRIENDS_OTHER; return false; }
    unsigned int GetFriendCount() { last = CALL_FRIENDS_OTHER; return 0; }
    galaxy::api::GalaxyID GetFriendByIndex(unsigned int) { last = CALL_FRIENDS_OTHER; return galaxy::api::GalaxyID(); }
    void SendFriendInvitation(galaxy::api::GalaxyID) { last = CALL_FRIENDS_OTHER; }
    void RequestFriendInvitationList() { last = CALL_FRIENDS_OTHER; }
    unsigned int GetFriendInvitationCount() { last = CALL_FRIENDS_OTHER; return 0; }
    void GetFriendInvitationByIndex(unsigned int, galaxy::api::GalaxyID&, unsigned int&) { last = CALL_FRIENDS_OTHER; }
    void RespondToFriendInvitation(galaxy::api::GalaxyID, bool) { last = CALL_FRIENDS_OTHER; }
    void DeleteFriend(galaxy::api::GalaxyID) { last = CALL_FRIENDS_OTHER; }
    void SetRichPresence(const char*, const char*) { last = CALL_FRIENDS_OTHER; }
    void DeleteRichPresence(const char*) { last = CALL_FRIENDS_OTHER; }

    CallId last;
};

typedef bool(__thiscall* RawBoolCall)(void*);
typedef void(__thiscall* RawVoidCall)(void*);

#ifndef _WIN64
const unsigned int kUserSignedInOffset = 0x4;
const unsigned int kFriendsRequestFriendListOffset = 0x34;
#else
const unsigned int kUserSignedInOffset = 1 * sizeof(void*);
const unsigned int kFriendsRequestFriendListOffset = 13 * sizeof(void*);
#endif

void* RawSlot(void* pObject, unsigned int uiOffset)
{
    return *(void**)(*(char**)pObject + uiOffset);
}

}

TEST_CASE("galaxy header IUser retail offset 0x4 is SignedIn" * doctest::test_suite("unit"))
{
    RecUser kUser;
    galaxy::api::IUser* pUser = &kUser;
    void* pThis = pUser;
    RawBoolCall pCall = (RawBoolCall)RawSlot(pThis, kUserSignedInOffset);
    CHECK(pCall(pThis));
    CHECK(kUser.last == CALL_SIGNED_IN);
    kUser.bSignedIn = false;
    kUser.last = CALL_NONE;
    CHECK_FALSE(pCall(pThis));
    CHECK(kUser.last == CALL_SIGNED_IN);
}

TEST_CASE("galaxy header IFriends retail offset 0x34 is RequestFriendList" * doctest::test_suite("unit"))
{
    RecFriends kFriends;
    galaxy::api::IFriends* pFriends = &kFriends;
    void* pThis = pFriends;
    RawVoidCall pCall = (RawVoidCall)RawSlot(pThis, kFriendsRequestFriendListOffset);
    pCall(pThis);
    CHECK(kFriends.last == CALL_REQUEST_FRIEND_LIST);
}

TEST_CASE("galaxy header typed calls hit the retail offsets" * doctest::test_suite("unit"))
{
    RecUser kUser;
    RecFriends kFriends;

    CHECK(static_cast<galaxy::api::IUser*>(&kUser)->SignedIn());
    CallId eTypedUser = kUser.last;
    kUser.last = CALL_NONE;
    void* pUserThis = static_cast<galaxy::api::IUser*>(&kUser);
    CHECK(((RawBoolCall)RawSlot(pUserThis, kUserSignedInOffset))(pUserThis));
    CHECK(eTypedUser == CALL_SIGNED_IN);
    CHECK(kUser.last == eTypedUser);

    static_cast<galaxy::api::IFriends*>(&kFriends)->RequestFriendList();
    CallId eTypedFriends = kFriends.last;
    kFriends.last = CALL_NONE;
    void* pFriendsThis = static_cast<galaxy::api::IFriends*>(&kFriends);
    ((RawVoidCall)RawSlot(pFriendsThis, kFriendsRequestFriendListOffset))(pFriendsThis);
    CHECK(eTypedFriends == CALL_REQUEST_FRIEND_LIST);
    CHECK(kFriends.last == eTypedFriends);
}
