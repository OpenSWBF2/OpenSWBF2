#include <new>
#include "GameMemory.h"
#include "GameVoiceChat.h"
#include "GameVoiceChatTransport.h"
#include "GameVoiceChatNode.h"
#include "GameVoiceChatDecoder.h"
#include "GameVoiceChatEncoder.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "SoundFL/source/win/sndengine.h"
#include "NetGame.h"
#include "SoundFL/source/sndthreadupdate.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"

namespace GameVoiceChat
{
    // GLOBAL: SWBF2 0x007e68f2
    bool gPlayerChange = true;
}

// GLOBAL: SWBF2 0x01e563cc

GameVoiceChat::Encoder* GameVoiceChat::gEncoders[1];
namespace GameVoiceChat
{
    // GLOBAL: SWBF2 0x01e5665c
    Node* gNodes[1];
}
// GLOBAL: SWBF2 0x01e566b4

GameVoiceChat::Decoder* GameVoiceChat::gDecoders[1];

namespace GameVoiceChat
{
    // GLOBAL: SWBF2 0x01e563d0
    unsigned char _gDecoderStorage[sizeof(Decoder) + 4];
}
namespace GameVoiceChat
{
    // GLOBAL: SWBF2 0x01e56660
    unsigned char _gNodeStorage[sizeof(Node) + 4];
}
namespace GameVoiceChat
{
    // GLOBAL: SWBF2 0x01e566b8
    unsigned char _gEncoderStorage[sizeof(Encoder) + 4];
}


// SYNTHETIC: SWBF2 0x00403ea0
// GameVoiceChat::`dynamic initializer for 'gTransport''
// SYNTHETIC: SWBF2 0x00768ae0
// GameVoiceChat::`dynamic atexit destructor for 'gTransport''

// GLOBAL: SWBF2 0x01ec5fc0
// GameVoiceChat::gTransport
GameVoiceChat::Transport GameVoiceChat::gTransport;

// SYNTHETIC: SWBF2 0x00403ec0
// GameVoiceChat::`dynamic initializer for 'gEncoderStorage''

// GLOBAL: SWBF2 0x01ec5fb8
// GameVoiceChat::gEncoderStorage
unsigned char* GameVoiceChat::gEncoderStorage = (unsigned char*)(((unsigned int)GameVoiceChat::_gEncoderStorage + 3) & ~3u);

// SYNTHETIC: SWBF2 0x00403ed0
// GameVoiceChat::`dynamic initializer for 'gDecoderStorage''

// GLOBAL: SWBF2 0x01ec5fbc
// GameVoiceChat::gDecoderStorage
unsigned char* GameVoiceChat::gDecoderStorage = (unsigned char*)(((unsigned int)GameVoiceChat::_gDecoderStorage + 3) & ~3u);

// SYNTHETIC: SWBF2 0x00403ee0
// GameVoiceChat::`dynamic initializer for 'gNodeStorage''

// GLOBAL: SWBF2 0x01ec6018
// GameVoiceChat::gNodeStorage
unsigned char* GameVoiceChat::gNodeStorage = (unsigned char*)(((unsigned int)GameVoiceChat::_gNodeStorage + 3) & ~3u);

// FUNCTION: SWBF2 0x0053c500
unsigned int GameVoiceChat::GetNumNodes()
{
    return 1;
}

namespace VoiceChat
{
    void* Alloc(unsigned int uiSize); void Free(void* pBlock);
}

extern int netVoiceMode;

namespace GameVoiceChat
{

// FUNCTION: SWBF2 0x0053c040
bool Open(float fSeconds, bool bSoundWorking)
{
    gHeap = GameMemory::ApplicationHeap;
    int iOldHeap = RedSetCurrentHeap(GameMemory::ApplicationHeap);

    if (GameVoiceChat::gTransport.Open(0x32, 1))
    {
        if (VoiceChat::Node::OpenStatic(&GameVoiceChat::gTransport, fSeconds, 1000.0f, 1000.0f))
        {
            unsigned int uiNodes = (unsigned int)GameVoiceChat::GetNumNodes();
            bool bAllOpen = true;

            for (unsigned int i = 0; i < uiNodes; ++i)
            {
                GameVoiceChat::gEncoders[i] = new ((GameVoiceChat::Encoder*)GameVoiceChat::gEncoderStorage + i) GameVoiceChat::Encoder;
                GameVoiceChat::gDecoders[i] = new ((GameVoiceChat::Decoder*)GameVoiceChat::gDecoderStorage + i) GameVoiceChat::Decoder;
                GameVoiceChat::gNodes[i] = new ((GameVoiceChat::Node*)GameVoiceChat::gNodeStorage + i)
                    GameVoiceChat::Node(GameVoiceChat::gEncoders[i],
                                        GameVoiceChat::gDecoders[i]);
            }

            GameVoiceChat::gPlayerChange = true;

            VoiceChat::Transport::Method cMethod(
                (VoiceChat::Transport::Method::Type)(netVoiceMode ? netVoiceMode : 2), 0x32);
            cMethod.SetNumRelayNodes(cMethod.CalculateOptimalRelayNodes(0x32));
            GameVoiceChat::gTransport.SetMethod(cMethod, true);

            if (!bSoundWorking)
            {
                RedSetCurrentHeap(iOldHeap);
                return true;
            }

            unsigned int i;

            for (i = 0; i < uiNodes; ++i)
            {
                if (!bAllOpen)
                {
                    break;
                }
                bAllOpen &= GameVoiceChat::gNodes[i]->Open(0xFF, 0xFF, 0x0C);
            }

            if (bAllOpen)
            {
                for (i = 0; i < uiNodes; ++i)
                {
                    GameVoiceChat::gNodes[i]->SetSendMask(GameVoiceChat::Transport::mask_TeamFriend, true, false);
                    GameVoiceChat::gNodes[i]->SetReceiveMask(GameVoiceChat::Transport::mask_Everyone, true, false);
                }
                RedSetCurrentHeap(iOldHeap);
                return true;
            }

            for (i = 0; i < uiNodes; ++i)
            {
                if (GameVoiceChat::gNodes[i]->IsOpen())
                {
                    GameVoiceChat::gNodes[i]->Close();
                }
            }

            for (i = 0; i < uiNodes; ++i)
            {
                (GameVoiceChat::gEncoders[i])->~Encoder();

                (GameVoiceChat::gDecoders[i])->~Decoder();

                GameVoiceChat::gEncoders[i] = 0;
                GameVoiceChat::gDecoders[i] = 0;
                GameVoiceChat::gNodes[i] = 0;
            }

            VoiceChat::Node::CloseStatic();
        }
        GameVoiceChat::gTransport.Close();
    }
    RedSetCurrentHeap(iOldHeap);
    return false;
}
}

// FUNCTION: SWBF2 0x0053c370
void GameVoiceChat::Reset()
{
    for (unsigned int i = (unsigned int)GameVoiceChat::GetNumNodes(); i; --i)
    {
        GameVoiceChat::Node* pNode = GameVoiceChat::GetNode(i - 1);
        if (!pNode)
        {
            continue;
        }
        pNode->SetSendMask(GameVoiceChat::Transport::mask_TeamFriend, true, false);
        pNode->SetReceiveMask(GameVoiceChat::Transport::mask_Everyone, true, false);
        pNode->SetID(0xFF);
    }
}

// FUNCTION: SWBF2 0x0053c2c0
void GameVoiceChat::Close()
{
    unsigned int uiNodes = (unsigned int)GameVoiceChat::GetNumNodes();
    int iOldHeap = RedSetCurrentHeap(gHeap);

    for (unsigned int i = 0; i < uiNodes; ++i)
    {
        if (!GameVoiceChat::gNodes[i])
        {
            continue;
        }

        if (GameVoiceChat::gNodes[i]->IsOpen())
        {
            GameVoiceChat::gNodes[i]->SetID(0xFF);
            GameVoiceChat::gNodes[i]->Close();
        }
        GameVoiceChat::gNodes[i] = 0;

        if (GameVoiceChat::gEncoders[i])
        {
            GameVoiceChat::gEncoders[i]->~Encoder();
            GameVoiceChat::gEncoders[i] = 0;
        }

        if (GameVoiceChat::gDecoders[i])
        {
            GameVoiceChat::gDecoders[i]->~Decoder();
            GameVoiceChat::gDecoders[i] = 0;
        }
    }

    VoiceChat::Node::CloseStatic();
    GameVoiceChat::gTransport.Close();
    GameVoiceChat::gPlayerChange = false;
    RedSetCurrentHeap(iOldHeap);
}

// FUNCTION: SWBF2 0x0053c3e0
bool GameVoiceChat::IsSuspended()
{
    return false;
}

// FUNCTION: SWBF2 0x0053c590
GameVoiceChat::Transport* GameVoiceChat::GetTransport()
{
    return &gTransport;
}

// FUNCTION: SWBF2 0x0053c5b0
bool GameVoiceChat::IsBanned()
{
    return false;
}

// FUNCTION: SWBF2 0x0053c5a0
void GameVoiceChat::PlayerChange()
{
    GameVoiceChat::gPlayerChange = true;
}

namespace GameVoiceChat
{
    extern Node* gNodes[1];
}
namespace GameVoiceChat
{
    unsigned int GetNumNodes();
}

// FUNCTION: SWBF2 0x0053c510
GameVoiceChat::Node* GameVoiceChat::GetEnabledNode(unsigned int uiEnabledIndex, unsigned int& uiIndex)
{
    const unsigned int uiNumNodes = GameVoiceChat::GetNumNodes();
    uiIndex = 0;
    for (unsigned int i = 0; i < uiNumNodes; ++i)
    {
        GameVoiceChat::Node* pNode = GameVoiceChat::gNodes[i];
        if (pNode->GetID() == 0xFF)
        {
            continue;
        }
        if (uiEnabledIndex == uiIndex)
        {
            return pNode;
        }
        ++uiIndex;
    }
    return 0;
}

// FUNCTION: SWBF2 0x0053c560
GameVoiceChat::Node* GameVoiceChat::GetEnabledNode(unsigned int uiEnabledIndex)
{
    unsigned int uiIndex;
    return GetEnabledNode(uiEnabledIndex, uiIndex);
}

// FUNCTION: SWBF2 0x0053c570
unsigned int GameVoiceChat::GetNumEnabledNodes()
{
    unsigned int uiCount;
    GetEnabledNode(GameVoiceChat::GetNumNodes(), uiCount);
    return uiCount;
}

namespace GameVoiceChat
{
    struct SuspendState
    {
        unsigned char mNodeID;
        unsigned char pad[3];
        unsigned int mCaptureDeviceIndex;
        unsigned int mRenderDeviceIndex;
    };
     
    // GLOBAL: SWBF2 0x007ec214
    SuspendState gSuspend[1] = { { 0xff, { 0, 0, 0 }, 0xffffffffu, 0xffffffffu } };
}

// FUNCTION: SWBF2 0x0053c3b0
void GameVoiceChat::SetNodeID(Node* const pNode, unsigned char uNodeID)
{
    unsigned int count = (unsigned int)GameVoiceChat::GetNumNodes();
    for (unsigned int i = 0; i < count; ++i)
    {
        if (GameVoiceChat::gNodes[i] == pNode)
        {
            GameVoiceChat::gSuspend[i].mNodeID = uNodeID;
            return;
        }
    }
}

// FUNCTION: SWBF2 0x0053c2b0
bool GameVoiceChat::IsOpen()
{
    return VoiceChat::Node::IsOpenStatic();
}

// FUNCTION: SWBF2 0x0053c3f0
void GameVoiceChat::Update(float fDeltaTime)
{
    if (!IsOpen())
    {
        return;
    }

    Snd::Engine::GetThreadUpdate()->Pause(false);

    SpawnDisplay* pSpawnDisplay = SpawnDisplay::Instance(0);
    bool abNotSelectingTeam[1];
    abNotSelectingTeam[0] = !pSpawnDisplay || !pSpawnDisplay->IsSelectingTeam();

    const unsigned int uiNodes = (unsigned int)GameVoiceChat::GetNumNodes();
    for (unsigned int i = 0; i < uiNodes; ++i)
    {
        VoiceChat::Node* pNode = GameVoiceChat::gNodes[i];
        if (pNode && pNode->GetID() != 0xFF)
        {
            const int iLocal = NetGame::GetJoystickIndex(pNode->GetID());
            if (GameVoiceChat::gPlayerChange && iLocal >= 0 && abNotSelectingTeam[iLocal])
            {
                ((GameVoiceChat::Node*)pNode)->Update();
            }
        }
    }

    GameVoiceChat::gPlayerChange = false;
    for (unsigned int i = 0; i < uiNodes; ++i)
    {
        if (!abNotSelectingTeam[i])
        {
            GameVoiceChat::gPlayerChange = true;
        }
    }

    GameVoiceChat::Node::Update(fDeltaTime);
    Snd::Engine::GetThreadUpdate()->Resume();
}

namespace GameVoiceChat
{

// FUNCTION: SWBF2 0x0053c4e0
Node* GetNode(const unsigned int nodeIndex)
{
    return GameVoiceChat::gNodes[0];
}

}

namespace GameVoiceChat
{

// FUNCTION: SWBF2 0x0053c4f0
Node* FindNodeByDeviceID(const unsigned int deviceID)
{
    return GameVoiceChat::gNodes[0];
}

}

