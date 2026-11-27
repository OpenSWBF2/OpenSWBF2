#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <winsock2.h>
#include <windows.h>
#include <random>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "harness/gamerun.h"
#include "game/riderlink_client.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const char* const kMission = "geo1c_con";

std::string LinkRunArgs(const std::string& sExtra)
{
    return StandardArgs(kMission, std::string(kMission) + " " + StandardTail(false) + (sExtra.empty() ? "" : " " + sExtra));
}

bool Contains(const std::string& sText, const std::string& sWhat)
{
    return sText.find(sWhat) != std::string::npos;
}

bool ContainsLineEnd(const std::string& sText, const std::string& sWhat)
{
    return Contains(sText, sWhat + "\r") || Contains(sText, sWhat + "\n");
}

}

#ifdef OPENSWBF2_RIDERLINK

namespace
{

bool AnyLine(const std::string&)
{
    return true;
}

void ExpectRefused(RiderLinkClient& kClient, const std::string& sLine, const char* pcWhat)
{
    INFO(pcWhat);
    REQUIRE(kClient.Connect());
    CHECK(kClient.SendLine(sLine));
    std::string sGot;
    CHECK(kClient.ReadUntilClosed(sGot) == RiderLinkClient::WakeClosed);
    CHECK(sGot.empty());
}

std::string WrongToken(const std::string& sToken)
{
    std::string sWrong = sToken;
    sWrong[sWrong.size() - 1] = sWrong[sWrong.size() - 1] == '0' ? '1' : '0';
    return sWrong;
}

class RawLink
{
public:
    explicit RawLink(unsigned int uPort) : mSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)), mOk(false)
    {
        if (mSocket == INVALID_SOCKET)
        {
            return;
        }
        sockaddr_in kAddr;
        memset(&kAddr, 0, sizeof(kAddr));
        kAddr.sin_family = AF_INET;
        kAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        kAddr.sin_port = htons((unsigned short)uPort);
        mOk = connect(mSocket, (sockaddr*)&kAddr, (int)sizeof(kAddr)) == 0;
    }

    ~RawLink()
    {
        if (mSocket != INVALID_SOCKET)
        {
            closesocket(mSocket);
        }
    }

    bool Ok() const
    {
        return mOk;
    }

    bool Send(const std::string& sBytes)
    {
        size_t uSent = 0;
        while (mOk && uSent < sBytes.size())
        {
            const int iSent = send(mSocket, sBytes.data() + uSent, (int)(sBytes.size() - uSent), 0);
            if (iSent <= 0)
            {
                return false;
            }
            uSent += (size_t)iSent;
        }
        return mOk;
    }

    bool ReadLine(std::string& sLine)
    {
        for (;;)
        {
            const size_t uEnd = mBuffer.find('\n');
            if (uEnd != std::string::npos)
            {
                sLine = mBuffer.substr(0, uEnd);
                mBuffer.erase(0, uEnd + 1);
                return true;
            }
            char acBuf[4096];
            const int iGot = recv(mSocket, acBuf, (int)sizeof(acBuf), 0);
            if (iGot <= 0)
            {
                return false;
            }
            mBuffer.append(acBuf, (size_t)iGot);
        }
    }

    std::string DrainUntilClosed()
    {
        std::string sAll = mBuffer;
        mBuffer.clear();
        for (;;)
        {
            char acBuf[4096];
            const int iGot = recv(mSocket, acBuf, (int)sizeof(acBuf), 0);
            if (iGot <= 0)
            {
                return sAll;
            }
            sAll.append(acBuf, (size_t)iGot);
        }
    }

    void CloseSend()
    {
        shutdown(mSocket, SD_SEND);
    }

private:
    RawLink(const RawLink&);
    RawLink& operator=(const RawLink&);

    SOCKET mSocket;
    bool mOk;
    std::string mBuffer;
};

std::string Tail(const std::vector<std::string>& kSeen)
{
    std::string sOut;
    const size_t uFrom = kSeen.size() > 12 ? kSeen.size() - 12 : 0;
    for (size_t i = uFrom; i < kSeen.size(); ++i)
    {
        sOut += kSeen[i] + "\n";
    }
    return sOut;
}

bool ReadUntilText(RiderLinkClient& kClient, const std::string& sWant, std::vector<std::string>& kSeen)
{
    std::string sLine;
    const RiderLinkClient::Wake eWake = kClient.ReadLineUntil(
        [&](const std::string& sGot)
        {
            kSeen.push_back(sGot);
            return Contains(sGot, sWant);
        },
        sLine);
    const std::string sTail = Tail(kSeen);
    INFO("waiting for '" << sWant << "', last lines read:\n" << sTail);
    CHECK(eWake == RiderLinkClient::WakeData);
    return eWake == RiderLinkClient::WakeData;
}

bool ReadSegment(RiderLinkClient& kClient, const std::string& sWant, std::vector<std::string>& kSeg,
                 std::vector<unsigned long>* pSim)
{
    kSeg.clear();
    if (!ReadUntilText(kClient, sWant, kSeg))
    {
        return false;
    }
    const std::string sLine = kSeg.back();
    kSeg.pop_back();
    if (pSim)
    {
        const size_t uAt = sLine.find("sim_frames=");
        pSim->push_back(uAt == std::string::npos ? 0ul : strtoul(sLine.c_str() + uAt + 11, 0, 10));
    }
    return true;
}

std::vector<unsigned long> TraceTurns(const std::vector<std::string>& kSeg)
{
    std::vector<unsigned long> kTurns;
    const std::string sKey = "Digest trace turn=";
    for (size_t i = 0; i < kSeg.size(); ++i)
    {
        const size_t uAt = kSeg[i].find(sKey);
        if (kSeg[i].compare(0, 4, "LOG ") == 0 && uAt != std::string::npos)
        {
            kTurns.push_back(strtoul(kSeg[i].c_str() + uAt + sKey.size(), 0, 10));
        }
    }
    return kTurns;
}

std::string TraceTurnsText(const std::vector<std::string>& kSeg)
{
    const std::vector<unsigned long> kTurns = TraceTurns(kSeg);
    std::string sOut;
    for (size_t i = 0; i < kTurns.size(); ++i)
    {
        sOut += std::to_string(kTurns[i]) + " ";
    }
    return sOut;
}

bool OpenHeld(RiderLinkClient& kClient, GameRun& kRun)
{
    if (!kClient.WaitForPortFile(kRun.RunDir()) || !kClient.Connect())
    {
        return false;
    }
    std::string sHello;
    std::string sState;
    const bool bOk = kClient.SendLine("HELLO 1 " + kClient.Token()) &&
                     kClient.ReadLineUntil(AnyLine, sHello) == RiderLinkClient::WakeData &&
                     kClient.ReadLineUntil(AnyLine, sState) == RiderLinkClient::WakeData;
    CHECK(sHello.compare(0, 8, "HELLO 1 ") == 0);
    CHECK(sState == "STATE Pause");
    return bOk && sState == "STATE Pause";
}

std::vector<std::string> SplitLines(const std::string& sText)
{
    std::vector<std::string> kLines;
    size_t uFrom = 0;
    while (uFrom < sText.size())
    {
        size_t uEnd = sText.find('\n', uFrom);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uFrom, uEnd - uFrom);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        kLines.push_back(sLine);
        uFrom = uEnd + 1;
    }
    return kLines;
}

struct Received
{
    bool bDropped;
    unsigned long uCount;
    std::string sText;
};

std::vector<Received> ParseReceived(const std::string& sAll)
{
    std::vector<Received> kOut;
    const std::vector<std::string> kLines = SplitLines(sAll);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        Received kGot;
        kGot.bDropped = false;
        kGot.uCount = 0;
        if (kLines[i].compare(0, 4, "LOG ") == 0)
        {
            kGot.sText = kLines[i].substr(4);
            kOut.push_back(kGot);
        }
        else if (kLines[i].compare(0, 8, "DROPPED ") == 0)
        {
            kGot.bDropped = true;
            kGot.uCount = strtoul(kLines[i].c_str() + 8, 0, 10);
            kOut.push_back(kGot);
        }
    }
    return kOut;
}

}

TEST_CASE_NAMED(FRiderLinkGameTest1, "Game::RiderLink::refuse", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-refuse");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkhold"));
    kRun.Start();
    const bool bPublished = kClient.WaitForPortFile(kRun.RunDir());
    CHECK(bPublished);
    std::string sHello;
    std::string sState;
    if (bPublished)
    {
        CHECK(std::regex_match(kClient.PortFileText(), std::regex("[1-9][0-9]{0,4} [0-9a-f]{32}\n")));
        const std::vector<unsigned long> kAddresses = RiderLinkClient::LocalAddresses();
        for (size_t i = 0; i < kAddresses.size(); ++i)
        {
            INFO("address 0x" << std::hex << kAddresses[i]);
            CHECK(kClient.ConnectTo(kAddresses[i]) == WSAECONNREFUSED);
        }
        ExpectRefused(kClient, "HELLO 1 " + WrongToken(kClient.Token()), "wrong token");
        ExpectRefused(kClient, "PLAY " + std::string(kMission), "not HELLO");
        ExpectRefused(kClient, "HELLO 2 " + kClient.Token(), "version 2");
        REQUIRE(kClient.Connect());
        CHECK(kClient.SendLine("HELLO 1 " + kClient.Token()));
        CHECK(kClient.ReadLineUntil(AnyLine, sHello) == RiderLinkClient::WakeData);
        CHECK(kClient.ReadLineUntil(AnyLine, sState) == RiderLinkClient::WakeData);
        std::string sRest;
        CHECK(kClient.ReadUntilClosed(sRest) == RiderLinkClient::WakeClosed);
    }
    kRun.Wait();
    char acWant[96];
    _snprintf(acWant, sizeof(acWant) - 1, "HELLO 1 %lu %s %s", kClient.Pid(), GameRun::Platform().c_str(),
              GameRun::Config().c_str());
    acWant[sizeof(acWant) - 1] = 0;
    CHECK(sHello == acWant);
    CHECK(sState == "STATE Pause");
    const std::string sLog = ReadRunLog(kRun);
    char acListen[64];
    _snprintf(acListen, sizeof(acListen) - 1, "RiderLink listening 127.0.0.1:%u", kClient.Port());
    acListen[sizeof(acListen) - 1] = 0;
    CHECK(ContainsLineEnd(sLog, acListen));
    CHECK(Contains(sLog, "RiderLink closed a client that sent a wrong token"));
    CHECK(Contains(sLog, "RiderLink closed a client that sent a line that is not HELLO"));
    CHECK(Contains(sLog, "RiderLink closed a client that speaks protocol 2, this game speaks 1"));
    CHECK(Contains(sLog, "RiderLink client connected"));
    CHECK(!kClient.PortFileExists());
}

TEST_CASE_NAMED(FRiderLinkGameTest2, "Game::RiderLink::digest", "[ProductFilter][Standard]")
{
    std::string sRow;
    REQUIRE(DigestRow(kMission, sRow));
    {
        GameRun kRun("riderlink-off");
        kRun.AddArg(LinkRunArgs(std::string()));
        AddStandardTests(kRun);
        kRun.Start();
        kRun.Wait();
        CheckStandardRun(kRun, kMission, 0);
        CHECK(!Contains(ReadRunLog(kRun), "RiderLink"));
    }
    {
        RiderLinkClient kIdle;
        GameRun kRun("riderlink-idle");
        kRun.AddArg(LinkRunArgs(kIdle.Args()));
        AddStandardTests(kRun);
        kRun.Start();
        kRun.Wait();
        CheckStandardRun(kRun, kMission, 0);
        const std::string sLog = ReadRunLog(kRun);
        CHECK(Contains(sLog, "RiderLink listening 127.0.0.1:"));
        CHECK(!Contains(sLog, "RiderLink client connected"));
        CHECK(!kIdle.PortFileExists());
    }
    {
        RiderLinkClient kListen;
        GameRun kRun("riderlink-listen");
        kRun.AddArg(LinkRunArgs(kListen.Args() + " -riderlinkhold"));
        AddStandardTests(kRun);
        kRun.Start();
        const bool bPublished = kListen.WaitForPortFile(kRun.RunDir());
        CHECK(bPublished);
        std::string sAll;
        if (bPublished && kListen.Connect())
        {
            CHECK(kListen.SendLine("HELLO 1 " + kListen.Token()));
            CHECK(kListen.ReadUntilClosed(sAll) == RiderLinkClient::WakeClosed);
        }
        kRun.Wait();
        CheckStandardRun(kRun, kMission, 0);
        CHECK(sAll.compare(0, 8, "HELLO 1 ") == 0);
        CHECK(Contains(sAll, "\nLOG "));
        CHECK(Contains(ReadRunLog(kRun), "RiderLink client connected"));
        CHECK(!kListen.PortFileExists());
    }
}

TEST_CASE_NAMED(FRiderLinkGameTest4, "Game::RiderLink::log", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-log");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkpause"));
    kRun.Start();
    std::string sAll;
    std::string sSecond = "not read";
    bool bSecondConnected = false;
    if (OpenHeld(kClient, kRun))
    {
        RawLink kSecond(kClient.Port());
        bSecondConnected = kSecond.Ok();
        if (bSecondConnected)
        {
            sSecond = kSecond.DrainUntilClosed();
        }
        CHECK(kClient.SendLine("RESUME"));
        CHECK(kClient.ReadUntilClosed(sAll) == RiderLinkClient::WakeClosed);
    }
    kRun.Wait();
    CHECK(kRun.Result().HasEndMarker);
    CHECK(bSecondConnected);
    CHECK(sSecond.empty());
    const std::string sLog = ReadRunLog(kRun);
    CHECK(Contains(sLog, "RiderLink refused a second client"));

    const std::vector<Received> kGot = ParseReceived(sAll);
    size_t uFirst = kGot.size();
    size_t uLast = kGot.size();
    unsigned long uDroppedTotal = 0;
    for (size_t i = 0; i < kGot.size(); ++i)
    {
        if (kGot[i].bDropped)
        {
            uDroppedTotal += kGot[i].uCount;
            continue;
        }
        if (uFirst == kGot.size())
        {
            uFirst = i;
        }
        uLast = i;
    }
    MESSAGE("riderlink log: payloads=" << kGot.size() << " dropped=" << uDroppedTotal);
    REQUIRE(uFirst < kGot.size());

    const std::vector<std::string> kFile = SplitLines(sLog);
    size_t uLine = 0;
    while (uLine < kFile.size() && kFile[uLine] != kGot[uFirst].sText)
    {
        ++uLine;
    }
    INFO("first payload: " << kGot[uFirst].sText);
    REQUIRE(uLine < kFile.size());

    size_t uAt = uFirst;
    unsigned long uMayMiss = 0;
    bool bWalked = true;
    while (uAt <= uLast)
    {
        if (kGot[uAt].bDropped)
        {
            uMayMiss = kGot[uAt].uCount;
            ++uAt;
            continue;
        }
        if (uLine >= kFile.size())
        {
            INFO("payload with no file line left: " << kGot[uAt].sText);
            CHECK(uLine < kFile.size());
            bWalked = false;
            break;
        }
        if (kFile[uLine] == kGot[uAt].sText)
        {
            ++uLine;
            ++uAt;
            uMayMiss = 0;
            continue;
        }
        if (uMayMiss > 0)
        {
            --uMayMiss;
            ++uLine;
            continue;
        }
        const size_t uLineNumber = uLine + 1;
        INFO("run.log line " << uLineNumber << ": " << kFile[uLine]);
        INFO("next payload: " << kGot[uAt].sText);
        CHECK(kFile[uLine] == kGot[uAt].sText);
        bWalked = false;
        break;
    }
    CHECK(bWalked);
}

TEST_CASE_NAMED(FRiderLinkGameTest5, "Game::RiderLink::malformed", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-malformed");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkpause"));
    kRun.Start();
    bool bBadServed = false;
    bool bStepped = false;
    bool bResumed = false;
    std::string sRest;
    if (kClient.WaitForPortFile(kRun.RunDir()))
    {
        {
            RawLink kBad(kClient.Port());
            std::string sHello;
            std::string sState;
            bBadServed = kBad.Ok() && kBad.Send("HELLO 1 " + kClient.Token() + "\n") && kBad.ReadLine(sHello) &&
                         kBad.ReadLine(sState) && sState == "STATE Pause";
            if (bBadServed)
            {
                CHECK(kBad.Send(std::string(300, 'A') + "\n"));
                CHECK(kBad.Send("FROB 1\n"));
                CHECK(kBad.Send("PAUSE\n"));
                std::mt19937 kRandom(51202u);
                std::string sNoise;
                for (int i = 0; i < 4096; ++i)
                {
                    sNoise += (char)(kRandom() & 0xff);
                }
                CHECK(kBad.Send(sNoise));
                CHECK(kBad.Send("\nSTE"));
                kBad.CloseSend();
                kBad.DrainUntilClosed();
            }
        }
        std::vector<std::string> kSeen;
        if (kClient.Connect())
        {
            std::string sHello;
            std::string sState;
            CHECK(kClient.SendLine("HELLO 1 " + kClient.Token()));
            CHECK(kClient.ReadLineUntil(AnyLine, sHello) == RiderLinkClient::WakeData);
            CHECK(kClient.ReadLineUntil(AnyLine, sState) == RiderLinkClient::WakeData);
            CHECK(sHello.compare(0, 8, "HELLO 1 ") == 0);
            CHECK(sState == "STATE Pause");
            CHECK(kClient.SendLine("STEP"));
            bStepped = ReadUntilText(kClient, "RiderLink step sim_frames=", kSeen);
            CHECK(kClient.SendLine("RESUME"));
            bResumed = ReadUntilText(kClient, "STATE Play", kSeen);
            CHECK(kClient.ReadUntilClosed(sRest) == RiderLinkClient::WakeClosed);
        }
    }
    kRun.Wait();
    CHECK(bBadServed);
    CHECK(bStepped);
    CHECK(bResumed);
    CHECK(kRun.Result().HasEndMarker);
    CHECK(kRun.Result().MarkerExitCode == 0);
    const std::string sLog = ReadRunLog(kRun);
    CHECK(Contains(sLog, "RiderLink ignored an oversized line from the client"));
    CHECK(Contains(sLog, "RiderLink ignored an unknown message from the client"));
    CHECK(Contains(sLog, "RiderLink refused PAUSE: it is not valid in this state"));
    CHECK(Contains(sLog, "RiderLink client disconnected"));
    CHECK(Contains(sLog, "RiderLink step sim_frames=1"));
}

TEST_CASE_NAMED(FRiderLinkGameTest6, "Game::RiderLink::pause-step", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-pause-step");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkpause"));
    kRun.Start();
    std::vector<unsigned long> kSim;
    std::vector<std::string> kSeg;
    bool bOk = OpenHeld(kClient, kRun);
    bOk = bOk && kClient.SendLine("STEP") && ReadSegment(kClient, "RiderLink step sim_frames=", kSeg, &kSim);
    if (bOk)
    {
        const std::string sHeld = TraceTurnsText(kSeg);
        INFO("trace lines while held before the first step: " << sHeld);
        CHECK(TraceTurns(kSeg).empty());
    }
    unsigned int uSteps = 1;
    std::vector<unsigned long> kTurns;
    while (bOk && kTurns.empty())
    {
        CHECK(uSteps < 2000);
        bOk = bOk && uSteps < 2000 && kClient.SendLine("STEP") &&
              ReadSegment(kClient, "RiderLink step sim_frames=", kSeg, &kSim);
        ++uSteps;
        kTurns = TraceTurns(kSeg);
    }
    const unsigned int uWalked = uSteps - 1;
    MESSAGE("riderlink pause-step: first turn after " << uWalked << " steps");
    std::vector<unsigned long> kStepped;
    for (int i = 0; bOk && i < 2; ++i)
    {
        bOk = kClient.SendLine("STEP") && ReadSegment(kClient, "RiderLink step sim_frames=", kSeg, &kSim);
        const std::vector<unsigned long> kOne = TraceTurns(kSeg);
        const std::string sOne = TraceTurnsText(kSeg);
        INFO("turns of one stepped frame: " << sOne);
        CHECK(kOne.size() == 1);
        kStepped.insert(kStepped.end(), kOne.begin(), kOne.end());
    }
    bOk = bOk && kClient.SendLine("RESUME\nPAUSE") && ReadSegment(kClient, "RiderLink resume sim_frames=", kSeg, 0);
    if (bOk)
    {
        const std::vector<unsigned long> kOne = TraceTurns(kSeg);
        const std::string sOne = TraceTurnsText(kSeg);
        INFO("turns of the third stepped frame: " << sOne);
        CHECK(kOne.size() == 1);
        kStepped.insert(kStepped.end(), kOne.begin(), kOne.end());
    }
    bOk = bOk && ReadSegment(kClient, "RiderLink pause sim_frames=", kSeg, 0);
    if (bOk)
    {
        CHECK(TraceTurns(kSeg).empty());
    }
    bOk = bOk && kClient.SendLine("STEP") && ReadSegment(kClient, "RiderLink step sim_frames=", kSeg, &kSim);
    bOk = bOk && kClient.SendLine("RESUME") && ReadSegment(kClient, "RiderLink resume sim_frames=", kSeg, 0);
    if (bOk)
    {
        const std::vector<unsigned long> kOne = TraceTurns(kSeg);
        const std::string sOne = TraceTurnsText(kSeg);
        INFO("turns of the step after the pause: " << sOne);
        CHECK(kOne.size() == 1);
        kStepped.insert(kStepped.end(), kOne.begin(), kOne.end());
    }
    bOk = bOk && ReadSegment(kClient, "STATE Play", kSeg, 0);
    if (bOk)
    {
        std::string sRest;
        CHECK(kClient.ReadUntilClosed(sRest) == RiderLinkClient::WakeClosed);
    }
    kRun.Wait();
    CHECK(bOk);
    REQUIRE(kTurns.size() >= 1);
    REQUIRE(kStepped.size() == 4);
    for (size_t i = 0; i < kStepped.size(); ++i)
    {
        INFO("stepped turn " << i);
        CHECK(kStepped[i] == kTurns.back() + 1 + i);
        CHECK(kStepped[i] <= 1500);
    }
    for (size_t i = 0; i < kSim.size(); ++i)
    {
        INFO("step line " << i);
        CHECK(kSim[i] == i + 1);
    }
    CHECK(kRun.Result().HasEndMarker);
    CheckStandardRun(kRun, kMission, 0);
}

TEST_CASE_NAMED(FRiderLinkGameTest7, "Game::RiderLink::stop-play", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-stop-play");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkpause"));
    kRun.Start();
    std::vector<std::string> kSeen;
    bool bDone = false;
    bool bOk = OpenHeld(kClient, kRun);
    bOk = bOk && kClient.SendLine("PLAY " + std::string(kMission)) &&
          ReadUntilText(kClient, "RiderLink refused PLAY: it is not valid in this state", kSeen);
    bOk = bOk && kClient.SendLine("STOP") && ReadUntilText(kClient, "STATE Idle", kSeen);
    bOk = bOk && kClient.SendLine("PAUSE") &&
          ReadUntilText(kClient, "RiderLink refused PAUSE: it is not valid in this state", kSeen);
    bOk = bOk && kClient.SendLine("PLAY no_such_mission_51_2") &&
          ReadUntilText(kClient, "RiderLink: no mission named no_such_mission_51_2", kSeen);
    const size_t uFrom = kSeen.size();
    bOk = bOk && kClient.SendLine("STEP") &&
          ReadUntilText(kClient, "RiderLink refused STEP: it is not valid in this state", kSeen);
    for (size_t i = uFrom; bOk && i < kSeen.size(); ++i)
    {
        INFO("line after the unknown mission: " << kSeen[i]);
        CHECK(kSeen[i].compare(0, 6, "STATE ") != 0);
    }
    bOk = bOk && kClient.SendLine("PLAY " + std::string(kMission)) && ReadUntilText(kClient, "STATE Pause", kSeen);
    bOk = bOk && kClient.SendLine("RESUME") && ReadUntilText(kClient, "STATE Play", kSeen);
    if (bOk)
    {
        std::string sRest;
        CHECK(kClient.ReadUntilClosed(sRest) == RiderLinkClient::WakeClosed);
        bDone = true;
    }
    kClient.Disconnect();
    kRun.Wait();
    CHECK(bDone);
    CHECK(kRun.Result().HasEndMarker);
    CHECK(kRun.Result().MarkerExitCode == 0);
    const std::string sLog = ReadRunLog(kRun);
    CHECK(Contains(sLog, "RiderLink stop sim_frames=0"));
    CHECK(Contains(sLog, std::string("RiderLink play ") + kMission));
    CHECK(!Contains(sLog, "RiderLink play no_such_mission_51_2"));
    CHECK(!Contains(sLog, "Could not open"));
}

#else

TEST_CASE_NAMED(FRiderLinkGameTest3, "Game::RiderLink::absent", "[ProductFilter][Standard]")
{
    RiderLinkClient kClient;
    GameRun kRun("riderlink-absent");
    kRun.AddArg(LinkRunArgs(kClient.Args() + " -riderlinkhold"));
    kRun.Start();
    kRun.Wait();
    const std::string sLog = ReadRunLog(kRun);
    CHECK(Contains(sLog, "LogCore: Warning: -riderlink is not in this build"));
    CHECK(!Contains(sLog, "RiderLink listening"));
    CHECK(!kClient.PortFileExists());
    CHECK(kRun.Result().HasEndMarker);
    CHECK(ContainsLineEnd(sLog, std::string("Mission running ") + kMission));
}

#endif
