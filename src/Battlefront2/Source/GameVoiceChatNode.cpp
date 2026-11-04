#include "GameVoiceChatNode.h"
#include "GameVoiceChatDecoder.h"
#include "GameVoiceChatEncoder.h"
#include "GameVoiceChatTransport.h"
#include "VoiceChatNode.h"

// FUNCTION: SWBF2 0x0053db30
GameVoiceChat::Node::Node(Encoder* const encoder, Decoder* const decoder)
    : VoiceChat::Node(encoder, decoder)
{
}

// FUNCTION: SWBF2 0x0053dbe0
void GameVoiceChat::Node::SetSendMask(Transport::Mask mask, bool enable, bool maskExisting)
{
    mSendMask.mMask = mask;
    mSendMask.mEnable = enable;
    mSendMask.mMaskExisting = maskExisting;
}

// FUNCTION: SWBF2 0x0053dc20
void GameVoiceChat::Node::SetReceiveMask(Transport::Mask mask, bool enable, bool maskExisting)
{
    mReceiveMask.mMask = mask;
    mReceiveMask.mEnable = enable;
    mReceiveMask.mMaskExisting = maskExisting;
}

// FUNCTION: SWBF2 0x0053dc60
void GameVoiceChat::Node::Update()
{
    const unsigned char id = GetID();
    if (id == 0xff)
    {
        return;
    }
    GameVoiceChat::Transport* transport = (GameVoiceChat::Transport*)VoiceChat::Node::sTransport;
    transport->SetSendEnableUsingMask(id, mSendMask.mMask, mSendMask.mEnable, mSendMask.mMaskExisting);
    transport->SetReceiveEnableUsingMask(id, mReceiveMask.mMask, mReceiveMask.mEnable,
                                         mReceiveMask.mMaskExisting);
}

// FUNCTION: SWBF2 0x0053dcb0
void GameVoiceChat::Node::Update(float fDeltaTime)
{
    VoiceChat::Node::Update(fDeltaTime);
}
