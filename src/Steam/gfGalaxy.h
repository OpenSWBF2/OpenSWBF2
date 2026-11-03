#ifndef GF_GALAXY_H
#define GF_GALAXY_H

#include <new>

#include "Battlefront2/Source/GameMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "galaxy_api.h"

namespace gf
{

__declspec(dllexport) bool initialize();
__declspec(dllexport) void deinitialize();
__declspec(dllexport) bool isInitialized();
__declspec(dllexport) bool checkError();

namespace utils
{
__declspec(dllexport) const char* ToString(bool bValue);
__declspec(dllexport) const char* ToString(int iValue);
__declspec(dllexport) const char* ToString(unsigned int uValue);
__declspec(dllexport) bool FromString(const char* pcValue, bool bDefault);
__declspec(dllexport) int FromString(const char* pcValue, int iDefault);
__declspec(dllexport) unsigned int FromString(const char* pcValue, unsigned int uDefault);
}

namespace friends
{

__declspec(dllexport) bool start();
__declspec(dllexport) void stop();
__declspec(dllexport) void reset();
__declspec(dllexport) unsigned int friendCount();
__declspec(dllexport) galaxy::api::GalaxyID friendByIndex(unsigned int uIndex);
__declspec(dllexport) const char* friendNameByIndex(unsigned int uIndex);
__declspec(dllexport) bool getAndResetDirty();
__declspec(dllexport) bool isFriend(const galaxy::api::GalaxyID& id);
__declspec(dllexport) bool isJoinableByIndex(unsigned int index);
__declspec(dllexport) const char* connectionStringByIndex(unsigned int index);
}

template <typename T>
__declspec(dllexport) T* Allocate()
{
    return new (RedAllocFromHeap(GameMemory::GamespyHeap, sizeof(T), 0)) T();
}

template <typename T, typename A1>
__declspec(dllexport) T* Allocate(A1&& a1)
{
    return new (RedAllocFromHeap(GameMemory::GamespyHeap, sizeof(T), 0)) T(a1);
}

// VTABLE: SWBF2 0x007ab928
class GalaxyAuthListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::IAuthListener>
{
public:
    GalaxyAuthListener(int iViewport) : m_iViewport(iViewport) {}

    virtual void OnAuthSuccess();
    virtual void OnAuthFailure(int failureReason);
    virtual void OnAuthLost();

private:
    int m_iViewport;
};

// VTABLE: SWBF2 0x007ab988
class GalaxyLobbyCreatedListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyCreatedListener>
{
public:
    virtual void OnLobbyCreated(const galaxy::api::GalaxyID& lobbyID, int result);
};

// VTABLE: SWBF2 0x007ab91c
class GalaxyLobbyLeftListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyLeftListener>
{
public:
    virtual void OnLobbyLeft(const galaxy::api::GalaxyID& lobbyID, bool ioFailure);
};

// VTABLE: SWBF2 0x007aba48
class GalaxyLobbyMemberStateListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyMemberStateListener>
{
public:
    virtual void OnLobbyMemberStateChanged(const galaxy::api::GalaxyID& lobbyID,
                                           const galaxy::api::GalaxyID& memberID,
                                           int memberStateChange);
};

// VTABLE: SWBF2 0x007ab9c4
class GalaxyLobbyEnteredListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyEnteredListener>
{
public:
    virtual void OnLobbyEntered(const galaxy::api::GalaxyID& lobbyID, int result);
};

// VTABLE: SWBF2 0x007ab9d0
class GalaxyLobbyListListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyListListener>
{
public:
    virtual void OnLobbyList(unsigned int lobbyCount, bool ioFailure);
};

// VTABLE: SWBF2 0x007ab93c
class GalaxyLobbyDataListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyDataListener>
{
public:
    virtual void OnLobbyDataUpdated(const galaxy::api::GalaxyID& lobbyID,
                                    const galaxy::api::GalaxyID& memberID);
};

// VTABLE: SWBF2 0x007ab9b8
class GalaxyLobbyOwnerChangeListener
    : public galaxy::api::SelfRegisteringListener<galaxy::api::ILobbyOwnerChangeListener>
{
public:
    virtual void OnLobbyOwnerChanged(const galaxy::api::GalaxyID& lobbyID,
                                     const galaxy::api::GalaxyID& newOwnerID);
};

}

#endif
