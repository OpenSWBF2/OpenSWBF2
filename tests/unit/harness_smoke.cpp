#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "testutil.h"
#include "nettest.h"
#include "RedEngineFL/Util/RedWarning.h"

extern NetPktGroup inQueue;

TEST_CASE_NAMED(FHarnessSmokeTest1, "Unit::HarnessSmoke::log capture sees LogMessage", "[SmokeFilter]")
{
    LogCapture log;
    RedWarning::SetLogData(RedWarning::RED_SEVERITY_WARNING, "Source\\Smoke.cpp", 1, "Jan  1 2000", "00:00:00");
    RedWarning::LogMessage("smoke marker %d", 42);
    CHECK(log.Contains("smoke marker 42"));
}

TEST_CASE_NAMED(FHarnessSmokeTest2, "Unit::HarnessSmoke::log capture restores callback", "[SmokeFilter]")
{
    void(__cdecl* pBefore)(char const*, ...) = RedWarning::g_fnLogCallback;
    RedWarning::RED_ERROR_SEVERITY eBefore = RedWarning::GetSeverityToDestination(RED_LOG_DESTINATION_INDEX_CALLBACK);
    {
        LogCapture log;
        CHECK(RedWarning::g_fnLogCallback != pBefore);
    }
    CHECK(RedWarning::g_fnLogCallback == pBefore);
    CHECK(RedWarning::GetSeverityToDestination(RED_LOG_DESTINATION_INDEX_CALLBACK) == eBefore);
}

TEST_CASE_NAMED(FHarnessSmokeTest3, "Unit::HarnessSmoke::net fixture pool is 512", "[SmokeFilter]")
{
    NetTestReset();
    CHECK(NetCountPackets(0) == 512);
    CHECK(freeBigPackets.pktCount == 256);
    CHECK(freeSmallPackets.pktCount == 256);
}

TEST_CASE_NAMED(FHarnessSmokeTest4, "Unit::HarnessSmoke::net fixture single packet round trip", "[SmokeFilter]")
{
    NetTestReset();
    NetPktGroup g;
    g.BeginWrite();
    g.Write(0xCAFE);
    g.EndWrite();
    NetPacket* pPacket = g.GetFront();
    g.Reset();
    NetStampAndQueue(pPacket, 7, PKT_GAME_MOVE, 3, 0, 1, 0);

    NetPktGroup out;
    CHECK(NetComm::Recv(out, PKT_GAME_MOVE));
    out.BeginRead();
    int iValue = 0;
    out.Read(iValue);
    CHECK(iValue == 0xCAFE);
    CHECK(NetCountPackets(out.pktCount) == 512);
}
