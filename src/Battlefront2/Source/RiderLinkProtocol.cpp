#ifdef OPENSWBF2_RIDERLINK

#include "RiderLinkProtocol.h"

#include <stdio.h>
#include <string.h>

#define RIDERLINK_STR2(x) #x
#define RIDERLINK_STR(x) RIDERLINK_STR2(x)

namespace RiderLinkProtocol
{

void ResetAssembler(LineAssembler& kAsm)
{
    kAsm.acLine[0] = 0;
    kAsm.uLen = 0;
    kAsm.bDiscard = false;
    kAsm.bReady = false;
}

TakeResult Take(LineAssembler& kAsm, char const* pcData, unsigned int uLen, unsigned int& uPos)
{
    if (kAsm.bReady)
    {
        kAsm.uLen = 0;
        kAsm.acLine[0] = 0;
        kAsm.bReady = false;
    }
    while (uPos < uLen)
    {
        const char c = pcData[uPos];
        ++uPos;
        if (c == '\n')
        {
            if (kAsm.bDiscard)
            {
                kAsm.bDiscard = false;
                kAsm.uLen = 0;
                continue;
            }
            if (kAsm.uLen > 0 && kAsm.acLine[kAsm.uLen - 1] == '\r')
            {
                --kAsm.uLen;
            }
            kAsm.acLine[kAsm.uLen] = 0;
            kAsm.bReady = true;
            return TakeLine;
        }
        if (kAsm.bDiscard)
        {
            continue;
        }
        if (kAsm.uLen >= kInCap - 2)
        {
            kAsm.bDiscard = true;
            kAsm.uLen = 0;
            kAsm.acLine[0] = 0;
            return TakeOversized;
        }
        kAsm.acLine[kAsm.uLen] = c;
        ++kAsm.uLen;
    }
    return TakeNeedMore;
}

static bool IsTokenChar(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

bool IsMissionName(char const* pcName, unsigned int uLen)
{
    if (uLen == 0 || uLen >= kMissionCap)
    {
        return false;
    }
    for (unsigned int i = 0; i < uLen; ++i)
    {
        const char c = pcName[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
        {
            return false;
        }
    }
    return true;
}

static bool WordIs(char const* pcLine, unsigned int uWord, char const* pcWant)
{
    const unsigned int uWant = (unsigned int)strlen(pcWant);
    return uWord == uWant && memcmp(pcLine, pcWant, uWant) == 0;
}

static Message ParseCommand(char const* pcLine, unsigned int uLen, unsigned int uWord, char* pcMission)
{
    if (WordIs(pcLine, uWord, "PLAY"))
    {
        if (uWord == uLen || !IsMissionName(&pcLine[uWord + 1], uLen - uWord - 1))
        {
            return MsgBadCommand;
        }
        memcpy(pcMission, &pcLine[uWord + 1], uLen - uWord - 1);
        pcMission[uLen - uWord - 1] = 0;
        return MsgPlay;
    }
    static const struct
    {
        char const* pcWord;
        Message eMsg;
    } akBare[] = { { "PAUSE", MsgPause }, { "RESUME", MsgResume }, { "STEP", MsgStep }, { "STOP", MsgStop } };
    for (unsigned int i = 0; i < sizeof(akBare) / sizeof(akBare[0]); ++i)
    {
        if (WordIs(pcLine, uWord, akBare[i].pcWord))
        {
            return uWord == uLen ? akBare[i].eMsg : MsgBadCommand;
        }
    }
    return MsgUnknown;
}

Message Parse(char const* pcLine, unsigned int uLen, char* pcToken, unsigned int& uVersion, char* pcMission)
{
    uVersion = 0;
    pcToken[0] = 0;
    pcMission[0] = 0;
    if (uLen == 0)
    {
        return MsgEmpty;
    }
    unsigned int uWord = 0;
    while (uWord < uLen && pcLine[uWord] != ' ')
    {
        ++uWord;
    }
    if (!WordIs(pcLine, uWord, "HELLO"))
    {
        return ParseCommand(pcLine, uLen, uWord, pcMission);
    }
    unsigned int i = uWord + 1;
    unsigned int uDigits = 0;
    unsigned int uValue = 0;
    while (i < uLen && pcLine[i] >= '0' && pcLine[i] <= '9' && uDigits < 9)
    {
        uValue = uValue * 10 + (unsigned int)(pcLine[i] - '0');
        ++i;
        ++uDigits;
    }
    if (uDigits == 0 || (i < uLen && pcLine[i] != ' '))
    {
        return MsgBadHello;
    }
    uVersion = uValue;
    if (uValue != kVersion)
    {
        return MsgHelloVersion;
    }
    if (i == uLen)
    {
        return MsgBadHello;
    }
    ++i;
    if (uLen - i != kTokenLen)
    {
        return MsgBadHello;
    }
    for (unsigned int j = 0; j < kTokenLen; ++j)
    {
        if (!IsTokenChar(pcLine[i + j]))
        {
            return MsgBadHello;
        }
    }
    memcpy(pcToken, &pcLine[i], kTokenLen);
    pcToken[kTokenLen] = 0;
    return MsgHello;
}

bool TokenEqual(char const* pcA, char const* pcB)
{
    unsigned int uDiff = 0;
    for (unsigned int i = 0; i < kTokenLen; ++i)
    {
        uDiff |= (unsigned int)((unsigned char)pcA[i] ^ (unsigned char)pcB[i]);
    }
    return uDiff == 0;
}

Refusal Check(Message eCommand, LinkState eState, bool bNetEnabled, bool bGameHoldsPause)
{
    switch (eCommand)
    {
    case MsgPause:
        if (bNetEnabled)
        {
            return RefuseNetwork;
        }
        return eState == StatePlay ? RefuseNone : RefuseWrongState;
    case MsgResume:
    case MsgStep:
        if (bNetEnabled)
        {
            return RefuseNetwork;
        }
        if (eState != StatePause)
        {
            return RefuseWrongState;
        }
        return bGameHoldsPause ? RefuseGameHolds : RefuseNone;
    case MsgStop:
        return eState == StatePlay || eState == StatePause ? RefuseNone : RefuseWrongState;
    case MsgPlay:
        return eState == StateIdle ? RefuseNone : RefuseWrongState;
    default:
        return RefuseWrongState;
    }
}

char const* CommandWord(Message eCommand)
{
    switch (eCommand)
    {
    case MsgPlay:
        return "PLAY";
    case MsgPause:
        return "PAUSE";
    case MsgResume:
        return "RESUME";
    case MsgStep:
        return "STEP";
    case MsgStop:
        return "STOP";
    default:
        return "a message";
    }
}

char const* StateWord(LinkState eState)
{
    switch (eState)
    {
    case StatePlay:
        return "Play";
    case StatePause:
        return "Pause";
    default:
        return "Idle";
    }
}

char const* RefusalText(Refusal eRefusal)
{
    switch (eRefusal)
    {
    case RefuseNone:
        return "allowed";
    case RefuseWrongState:
        return "it is not valid in this state";
    case RefuseNetwork:
        return "networking is enabled";
    case RefuseGameHolds:
        return "the game's own pause holds the mission";
    default:
        return "unknown";
    }
}

void RingInit(Ring& kRing, unsigned char* pData, unsigned int uSize)
{
    kRing.pData = pData;
    kRing.uSize = uSize;
    RingClear(kRing);
}

void RingClear(Ring& kRing)
{
    kRing.uHead = 0;
    kRing.uTail = 0;
    kRing.uDropped = 0;
}

unsigned int RingUsed(Ring const& kRing)
{
    return kRing.uTail - kRing.uHead;
}

static unsigned int FirstLineLen(Ring const& kRing)
{
    const unsigned int uMask = kRing.uSize - 1;
    const unsigned int uUsed = RingUsed(kRing);
    unsigned int uLen = 0;
    while (uLen < uUsed)
    {
        const unsigned char c = kRing.pData[(kRing.uHead + uLen) & uMask];
        ++uLen;
        if (c == '\n')
        {
            break;
        }
    }
    return uLen;
}

static void Put(Ring& kRing, char const* pcText, unsigned int uLen)
{
    const unsigned int uMask = kRing.uSize - 1;
    for (unsigned int i = 0; i < uLen; ++i)
    {
        const char c = pcText[i];
        kRing.pData[kRing.uTail & uMask] = (unsigned char)(c == '\n' ? ' ' : c);
        ++kRing.uTail;
    }
}

void RingPush(Ring& kRing, char const* pcPrefix, char const* pcBody, unsigned int uBodyLen)
{
    const unsigned int uPrefix = (unsigned int)strlen(pcPrefix);
    if (uPrefix + uBodyLen + 1 > kOutCap)
    {
        uBodyLen = kOutCap - 1 - uPrefix;
    }
    const unsigned int uNeed = uPrefix + uBodyLen + 1;
    if (uNeed > kRing.uSize)
    {
        ++kRing.uDropped;
        return;
    }
    while (kRing.uSize - RingUsed(kRing) < uNeed)
    {
        kRing.uHead += FirstLineLen(kRing);
        ++kRing.uDropped;
    }
    Put(kRing, pcPrefix, uPrefix);
    Put(kRing, pcBody, uBodyLen);
    kRing.pData[kRing.uTail & (kRing.uSize - 1)] = '\n';
    ++kRing.uTail;
}

unsigned int RingRead(Ring& kRing, char* pcOut, unsigned int uCap, char const* pcState)
{
    unsigned int uOut = 0;
    if (kRing.uDropped)
    {
        const int iLen = snprintf(pcOut, uCap, "DROPPED %u\nSTATE %s\n", kRing.uDropped, pcState);
        if (iLen < 0 || (unsigned int)iLen >= uCap)
        {
            return 0;
        }
        uOut = (unsigned int)iLen;
        kRing.uDropped = 0;
    }
    const unsigned int uMask = kRing.uSize - 1;
    while (RingUsed(kRing) > 0)
    {
        const unsigned int uLen = FirstLineLen(kRing);
        if (uOut + uLen > uCap)
        {
            break;
        }
        for (unsigned int i = 0; i < uLen; ++i)
        {
            pcOut[uOut + i] = (char)kRing.pData[(kRing.uHead + i) & uMask];
        }
        uOut += uLen;
        kRing.uHead += uLen;
    }
    return uOut;
}

void ResetTap(TapBuilder& kTap)
{
    kTap.acPending[0] = 0;
    kTap.uLen = 0;
    kTap.uOwner = 0;
    kTap.bActive = false;
}

static void AppendPending(TapBuilder& kTap, char const* pcText, unsigned int uLen)
{
    const unsigned int uRoom = kOutCap - 5 - kTap.uLen;
    if (uLen > uRoom)
    {
        uLen = uRoom;
    }
    memcpy(&kTap.acPending[kTap.uLen], pcText, uLen);
    kTap.uLen += uLen;
}

void Tap(TapBuilder& kTap, Ring& kRing, char const* pcText, unsigned int uThread)
{
    const bool bOwner = !kTap.bActive || kTap.uOwner == uThread;
    char const* pc = pcText;
    while (*pc)
    {
        char const* pcEnd = strchr(pc, '\n');
        const unsigned int uSeg = pcEnd ? (unsigned int)(pcEnd - pc) : (unsigned int)strlen(pc);
        if (!bOwner)
        {
            RingPush(kRing, "LOG ", pc, uSeg);
        }
        else if (!pcEnd)
        {
            AppendPending(kTap, pc, uSeg);
            kTap.bActive = true;
            kTap.uOwner = uThread;
        }
        else
        {
            AppendPending(kTap, pc, uSeg);
            RingPush(kRing, "LOG ", kTap.acPending, kTap.uLen);
            kTap.uLen = 0;
            kTap.bActive = false;
        }
        pc += uSeg;
        if (pcEnd)
        {
            ++pc;
        }
    }
}

void ResetBudget(WarnBudget& kBudget)
{
    kBudget.uSpent = 0;
    kBudget.uSuppressed = 0;
}

bool SpendWarning(WarnBudget& kBudget)
{
    if (kBudget.uSpent < kWarnBudget)
    {
        ++kBudget.uSpent;
        return true;
    }
    ++kBudget.uSuppressed;
    return false;
}

bool ShouldStart(bool bNoRender, bool bHandsOff, bool bRiderLink)
{
    return bRiderLink || (!bNoRender && !bHandsOff);
}

char const* PlatformWord()
{
    return sizeof(void*) == 8 ? "x64" : "Win32";
}

char const* ConfigWord()
{
    return RIDERLINK_STR(OPENSWBF2_RIDERLINK);
}

}

#endif
