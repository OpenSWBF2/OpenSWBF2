#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"
#include "RedEngineFL/Util/RedWarning.h"

extern NetPktGroup inQueue;

TEST_CASE("log capture sees LogMessage" * doctest::test_suite("unit"))
{
    LogCapture log;
    RedWarning::SetLogData(RedWarning::RED_SEVERITY_WARNING, "Source\\Smoke.cpp", 1, __DATE__, __TIME__);
    RedWarning::LogMessage("smoke marker %d", 42);
    CHECK(log.Contains("smoke marker 42"));
}

TEST_CASE("log capture restores callback" * doctest::test_suite("unit"))
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

TEST_CASE("net fixture pool is 512" * doctest::test_suite("unit"))
{
    NetTestReset();
    CHECK(NetCountPackets(0) == 512);
    CHECK(freeBigPackets.pktCount == 256);
    CHECK(freeSmallPackets.pktCount == 256);
}

TEST_CASE("net fixture single packet round trip" * doctest::test_suite("unit"))
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
    CHECK(NetComm::Recv(&out, PKT_GAME_MOVE));
    out.BeginRead();
    int iValue = 0;
    out.Read(iValue);
    CHECK(iValue == 0xCAFE);
    CHECK(NetCountPackets(out.pktCount) == 512);
}
