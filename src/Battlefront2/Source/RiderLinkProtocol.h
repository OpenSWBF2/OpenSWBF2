#pragma once

#ifdef OPENSWBF2_RIDERLINK

namespace RiderLinkProtocol
{

const unsigned int kVersion = 1;
const unsigned int kInCap = 256;
const unsigned int kOutCap = 1028;
const unsigned int kRingSize = 262144;
const unsigned int kTokenLen = 32;
const unsigned int kWarnBudget = 8;
const unsigned int kMissionCap = 64;

enum TakeResult
{
    TakeNeedMore,
    TakeLine,
    TakeOversized
};

enum Message
{
    MsgEmpty,
    MsgHello,
    MsgHelloVersion,
    MsgBadHello,
    MsgUnknown,
    MsgBadCommand,
    MsgPlay,
    MsgPause,
    MsgResume,
    MsgStep,
    MsgStop
};

enum LinkState
{
    StateIdle,
    StatePlay,
    StatePause
};

enum Refusal
{
    RefuseNone,
    RefuseWrongState,
    RefuseNetwork,
    RefuseGameHolds
};

struct LineAssembler
{
    char acLine[kInCap];
    unsigned int uLen;
    bool bDiscard;
    bool bReady;
};

struct Ring
{
    unsigned char* pData;
    unsigned int uSize;
    unsigned int uHead;
    unsigned int uTail;
    unsigned int uDropped;
};

struct TapBuilder
{
    char acPending[kOutCap];
    unsigned int uLen;
    unsigned int uOwner;
    bool bActive;
};

struct WarnBudget
{
    unsigned int uSpent;
    unsigned int uSuppressed;
};

void ResetAssembler(LineAssembler& kAsm);
TakeResult Take(LineAssembler& kAsm, char const* pcData, unsigned int uLen, unsigned int& uPos);

Message Parse(char const* pcLine, unsigned int uLen, char* pcToken, unsigned int& uVersion, char* pcMission);
bool TokenEqual(char const* pcA, char const* pcB);
bool IsMissionName(char const* pcName, unsigned int uLen);
Refusal Check(Message eCommand, LinkState eState, bool bNetEnabled, bool bGameHoldsPause);
char const* CommandWord(Message eCommand);
char const* StateWord(LinkState eState);
char const* RefusalText(Refusal eRefusal);

void RingInit(Ring& kRing, unsigned char* pData, unsigned int uSize);
void RingClear(Ring& kRing);
unsigned int RingUsed(Ring const& kRing);
void RingPush(Ring& kRing, char const* pcPrefix, char const* pcBody, unsigned int uBodyLen);
unsigned int RingRead(Ring& kRing, char* pcOut, unsigned int uCap, char const* pcState);

void ResetTap(TapBuilder& kTap);
void Tap(TapBuilder& kTap, Ring& kRing, char const* pcText, unsigned int uThread);

void ResetBudget(WarnBudget& kBudget);
bool SpendWarning(WarnBudget& kBudget);

bool ShouldStart(bool bNoRender, bool bHandsOff, bool bRiderLink);

char const* PlatformWord();
char const* ConfigWord();

}

#endif
