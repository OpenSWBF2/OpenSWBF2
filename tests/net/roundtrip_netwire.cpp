#include "third_party/doctest/doctest.h"

#include <string.h>

#include "testutil.h"
#include "nettest.h"
#include "Battlefront2/Source/NetWire.h"

static void FillPattern(unsigned char* pDst, unsigned int uLen)
{
    for (unsigned int i = 0; i < uLen; ++i)
    {
        pDst[i] = (unsigned char)(i * 13 + 5);
    }
}

static void StartWrite(NetPktGroup& group)
{
    group.Reset();
    group.BeginWrite();
}

static void FinishWrite(NetPktGroup& group)
{
    group.EndWrite();
}

static void ExpectBytes(NetPktGroup& group, const unsigned char* pExpected, unsigned int uCount)
{
    NetPacket* pPacket = group.GetFront();
    REQUIRE(pPacket);
    REQUIRE((unsigned int)(pPacket->mMaxIndex - pPacket->mDataIndex) >= uCount);
    CHECK(memcmp(pPacket->mData + pPacket->mDataIndex, pExpected, uCount) == 0);
}

TEST_CASE("chat at 276 bytes round trips and pins the header" * doctest::test_suite("regression"))
{
    NetTestReset();
    unsigned char acSrc[276];
    FillPattern(acSrc, sizeof(acSrc));
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteChatPayload(group, acSrc, 276);
    FinishWrite(group);
    const unsigned char acHeader[4] = {0x14, 0x01, 0xff, 0xff};
    ExpectBytes(group, acHeader, 4);
    group.BeginRead();
    unsigned int uHeader = 0;
    group.ReadBytes((char*)&uHeader, 4);
    CHECK(uHeader == 0xffff0114u);
    unsigned char acDst[276];
    memset(acDst, 0xAA, sizeof(acDst));
    CHECK(NetWire::ReadChatPayload(group, acDst, 276, uHeader & 0xffff));
    CHECK(memcmp(acDst, acSrc, 274) == 0);
    CHECK(acDst[274] == 0);
    CHECK(acDst[275] == 0);
    CHECK(netRejectCounts[NET_REJECT_CHAT_OVERSIZE] == 0);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("chat at 22 bytes round trips and pins the header" * doctest::test_suite("regression"))
{
    NetTestReset();
    unsigned char acSrc[22];
    FillPattern(acSrc, sizeof(acSrc));
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteChatPayload(group, acSrc, 22);
    FinishWrite(group);
    const unsigned char acHeader[4] = {0x16, 0x00, 0xff, 0xff};
    ExpectBytes(group, acHeader, 4);
    group.BeginRead();
    unsigned int uHeader = 0;
    group.ReadBytes((char*)&uHeader, 4);
    CHECK(uHeader == 0xffff0016u);
    unsigned char acDst[276];
    memset(acDst, 0, sizeof(acDst));
    CHECK(NetWire::ReadChatPayload(group, acDst, 276, uHeader & 0xffff));
    CHECK(memcmp(acDst, acSrc, 22) == 0);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("empty chat writes only the pinned header" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteChatPayload(group, 0, 0);
    FinishWrite(group);
    const unsigned char acHeader[4] = {0x00, 0x00, 0xff, 0xff};
    ExpectBytes(group, acHeader, 4);
    NetPacket* pPacket = group.GetFront();
    REQUIRE(pPacket);
    CHECK(pPacket->mMaxIndex - pPacket->mDataIndex == 8);
    group.BeginRead();
    unsigned int uHeader = 0;
    group.ReadBytes((char*)&uHeader, 4);
    CHECK(uHeader == 0xffff0000u);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("upload header round trips through a retail shaped read and pins bytes" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteUploadHeader(group, 3, true, "ab", 5000);
    FinishWrite(group);
    const unsigned char acExpected[12] = {0x07, 0x00, 0x00, 0x00, 0x0a, 0x84, 0x89, 0x21, 0x4e, 0x00, 0x00, 0x00};
    ExpectBytes(group, acExpected, 12);
    group.BeginRead();
    bool bUploading = false;
    bool bMore = false;
    unsigned int uSeq = 0;
    unsigned int uSize = 0;
    char acName[32];
    memset(acName, 0x7f, sizeof(acName));
    group.Read(bUploading);
    group.ReadBits(uSeq, 32);
    group.Read(bMore);
    NetWire::ReadString(group, acName, 32);
    group.ReadBits(uSize, 32);
    CHECK(bUploading);
    CHECK(uSeq == 3);
    CHECK(bMore);
    CHECK(strcmp(acName, "ab") == 0);
    CHECK(uSize == 5000);
    CHECK(!group.IsBad());
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("upload header with a full journal name round trips" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteUploadHeader(group, 77, true, "journal_1_2.dat", 5000);
    FinishWrite(group);
    group.BeginRead();
    bool bUploading = false;
    bool bMore = false;
    unsigned int uSeq = 0;
    unsigned int uSize = 0;
    char acName[32];
    group.Read(bUploading);
    group.ReadBits(uSeq, 32);
    group.Read(bMore);
    NetWire::ReadString(group, acName, 32);
    group.ReadBits(uSize, 32);
    CHECK(bUploading);
    CHECK(uSeq == 77);
    CHECK(bMore);
    CHECK(strcmp(acName, "journal_1_2.dat") == 0);
    CHECK(NetWire::IsSafeUploadName(acName));
    CHECK(uSize == 5000);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("upload header without more files writes no name or size" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteUploadHeader(group, 9, false, "", 0);
    FinishWrite(group);
    const unsigned char acExpected[8] = {0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ExpectBytes(group, acExpected, 8);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("upload chunk pins bytes for a small payload" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteUploadChunk(group, 3, "ABCD", 4);
    FinishWrite(group);
    const unsigned char acExpected[12] = {0x06, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x41, 0x42, 0x43, 0x44};
    ExpectBytes(group, acExpected, 12);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("upload chunk at 1012 bytes round trips" * doctest::test_suite("regression"))
{
    NetTestReset();
    unsigned char acSrc[1012];
    FillPattern(acSrc, sizeof(acSrc));
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteUploadChunk(group, 41, (const char*)acSrc, 1012);
    FinishWrite(group);
    group.BeginRead();
    bool bUploading = true;
    unsigned int uSeq = 0;
    group.Read(bUploading);
    group.ReadBits(uSeq, 32);
    unsigned char acDst[1012];
    unsigned int uLength = 0;
    CHECK(NetWire::ReadUploadChunk(group, (char*)acDst, 1012, 5000, uLength));
    CHECK(!bUploading);
    CHECK(uSeq == 41);
    CHECK(uLength == 1012);
    CHECK(memcmp(acDst, acSrc, 1012) == 0);
    CHECK(!group.IsBad());
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("turn rate round trips for every retail value" * doctest::test_suite("regression"))
{
    NetTestReset();
    for (unsigned int uRate = 10; uRate <= 255; ++uRate)
    {
        NetPktGroup group;
        StartWrite(group);
        NetWire::WriteTurnRate(group, uRate, 1);
        FinishWrite(group);
        const unsigned int uWord = uRate | (1u << 10);
        const unsigned char acExpected[4] = {(unsigned char)(uWord & 0xff), (unsigned char)(uWord >> 8), 0, 0};
        ExpectBytes(group, acExpected, 4);
        group.BeginRead();
        unsigned int uReadRate = 0;
        unsigned int uReadScale = 0;
        group.ReadBits(uReadRate, 10);
        group.ReadBits(uReadScale, 3);
        CHECK(uReadRate == uRate);
        CHECK(uReadScale == 1);
        CHECK(NetWire::IsValidTurnRate((int)uReadRate, (int)uReadScale));
        group.Free();
    }
    CHECK(netRejectCounts[NET_REJECT_TURN_RATE] == 0);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("turn rate pins the default twenty" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPktGroup group;
    StartWrite(group);
    NetWire::WriteTurnRate(group, 20, 1);
    FinishWrite(group);
    const unsigned char acExpected[4] = {0x14, 0x04, 0x00, 0x00};
    ExpectBytes(group, acExpected, 4);
    group.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("player counts one through sixty four round trip unchanged" * doctest::test_suite("regression"))
{
    NetTestReset();
    for (unsigned int uCount = 1; uCount <= 64; ++uCount)
    {
        NetPktGroup group;
        StartWrite(group);
        NetWire::WritePlayerCount(group, uCount);
        FinishWrite(group);
        const unsigned char acExpected[4] = {(unsigned char)uCount, 0, 0, 0};
        ExpectBytes(group, acExpected, 4);
        group.BeginRead();
        unsigned int uRead = 0;
        group.ReadBits(uRead, 7);
        CHECK(uRead == uCount);
        CHECK(NetWire::ClampPlayerCount(uRead) == (int)uCount);
        group.Free();
    }
    CHECK(netRejectCounts[NET_REJECT_PLAYER_COUNT] == 0);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("rcon line of 63 characters passes auth and decodes" * doctest::test_suite("regression"))
{
    NetTestReset();
    char acPacket[9 + 63];
    memset(acPacket, 0, sizeof(acPacket));
    acPacket[5] = 'J';
    acPacket[6] = 0;
    acPacket[7] = 'k';
    acPacket[8] = 63;
    for (int i = 0; i < 63; ++i)
    {
        acPacket[9 + i] = (char)('a' + (i % 26));
    }
    CHECK(NetWire::IsRconAuthorized(acPacket, (int)sizeof(acPacket), 'k', 'm'));
    unsigned short ausLine[64];
    CHECK(NetWire::ReadRconLine(acPacket + 9, 63, (unsigned char)acPacket[8], ausLine) == 63);
    for (int i = 0; i < 63; ++i)
    {
        CHECK(ausLine[i] == (unsigned short)('a' + (i % 26)));
    }
    CHECK(ausLine[63] == 0);
    CHECK(netRejectCounts[NET_REJECT_RCON_LINE] == 0);
}
