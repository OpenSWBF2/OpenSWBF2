#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <algorithm>
#include <string.h>
#include <stdlib.h>

#include "testutil.h"
#include "nettest.h"
#include "corpus_replay.h"
#include "Battlefront2/Source/NetWire.h"

static const unsigned int MAX_FILE = 65536;
static const int REC_HEADER = 9;
static const int MAX_PAYLOAD = 998;
static const int MAX_RECORDS = 128;

std::vector<std::string> ListCorpus(const char* pcReader, bool bKnownBug)
{
    std::vector<std::string> names;
    std::string sDir = std::string("corpus\\") + pcReader + (bKnownBug ? "\\known_bug\\" : "\\");
    WIN32_FIND_DATAA find;
    HANDLE hFind = FindFirstFileA((sDir + "*.bin").c_str(), &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return names;
    }
    do
    {
        if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            names.push_back(sDir + find.cFileName);
        }
    } while (FindNextFileA(hFind, &find));
    FindClose(hFind);
    std::sort(names.begin(), names.end());
    return names;
}

bool LoadCorpusFile(const std::string& sPath, ByteVec& bytes)
{
    bytes.clear();
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    fseek(pFile, 0, SEEK_END);
    long lSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    bool bOk = lSize >= 0 && (unsigned long)lSize <= MAX_FILE;
    if (bOk && lSize > 0)
    {
        bytes.resize(lSize);
        bOk = fread(&bytes[0], 1, lSize, pFile) == (size_t)lSize;
    }
    fclose(pFile);
    return bOk;
}

static void DrainAndCheck(bool& bOk)
{
    for (int iType = 0; iType < PKT_NONE; ++iType)
    {
        NetPktGroup out;
        out.Reset();
        while (NetComm::Recv(&out, iType))
        {
            out.Free();
            out.Reset();
        }
    }
    bOk = NetCountPackets(0) == 512;
}

bool ReplayRecvGroupBytes(const ByteVec& bytes, bool* pbConsumed)
{
    NetTestReset();
    size_t uPos = 0;
    for (int iRec = 0; iRec < MAX_RECORDS && uPos + REC_HEADER <= bytes.size(); ++iRec)
    {
        const unsigned char* p = &bytes[uPos];
        unsigned int uLen = p[7] | (p[8] << 8);
        if (uLen > (unsigned int)MAX_PAYLOAD || uPos + REC_HEADER + uLen > bytes.size())
        {
            break;
        }
        NetPacket* pPacket = NetMakePacket(p + REC_HEADER, (int)uLen);
        NetStampAndQueue(pPacket, (unsigned short)(p[0] | (p[1] << 8)), p[2], p[3], p[4], p[5], p[6]);
        uPos += REC_HEADER + uLen;
    }
    if (pbConsumed)
    {
        *pbConsumed = uPos == bytes.size();
    }
    bool bOk;
    DrainAndCheck(bOk);
    return bOk;
}

bool ReplayRecvPacketBytes(const unsigned char* pData, unsigned int uiSize)
{
    NetTestReset();
    NetDeliverDatagram(pData, uiSize, 3);
    bool bOk;
    DrainAndCheck(bOk);
    return bOk;
}

TEST_CASE("corpus replay recvgroup" * doctest::test_suite("corpus"))
{
    std::vector<std::string> files = ListCorpus("recvgroup", false);
    REQUIRE(!files.empty());
    for (size_t i = 0; i < files.size(); ++i)
    {
        ByteVec bytes;
        REQUIRE(LoadCorpusFile(files[i], bytes));
        WatchdogArm(files[i].c_str());
        INFO(files[i]);
        bool bConsumed = false;
        CHECK(ReplayRecvGroupBytes(bytes, &bConsumed));
        CHECK(bConsumed);
    }
}

TEST_CASE("corpus replay recvpacket" * doctest::test_suite("corpus"))
{
    std::vector<std::string> files = ListCorpus("recvpacket", false);
    REQUIRE(!files.empty());
    for (size_t i = 0; i < files.size(); ++i)
    {
        ByteVec bytes;
        REQUIRE(LoadCorpusFile(files[i], bytes));
        WatchdogArm(files[i].c_str());
        INFO(files[i]);
        CHECK(ReplayRecvPacketBytes(bytes.empty() ? 0 : &bytes[0], (unsigned int)bytes.size()));
    }
}

static void BuildNetWireGroup(NetPktGroup& group, const unsigned char* pData, unsigned int uLen)
{
    group.Reset();
    group.BeginWrite();
    group.WriteBytes(uLen ? (const char*)pData : "", (int)uLen);
    group.EndWrite();
    group.BeginRead();
}

static bool CanaryOk(const unsigned char* pBuf, unsigned int uFrom, unsigned int uTo)
{
    for (unsigned int i = uFrom; i < uTo; ++i)
    {
        if (pBuf[i] != 0xAA)
        {
            return false;
        }
    }
    return true;
}

static bool ReplayChat(const unsigned char* pData, unsigned int uLen, bool bFromClient)
{
    NetPktGroup group;
    BuildNetWireGroup(group, pData, uLen);
    unsigned int uHeader = 0;
    group.ReadBytes((char*)&uHeader, 4);
    unsigned char acDst[292];
    memset(acDst, 0xAA, sizeof(acDst));
    NetWire::ReadChatPayload(group, acDst, 276, uHeader & 0xffff);
    unsigned int uPlayerID = 0;
    int iCode = 0;
    memcpy(&uPlayerID, &acDst[0], sizeof(uPlayerID));
    memcpy(&iCode, &acDst[16], sizeof(iCode));
    NetWire::IsValidChatFields((int)uPlayerID, iCode, bFromClient);
    bool bOk = CanaryOk(acDst, 276, 292);
    group.Free();
    return bOk;
}

static bool ReplayUpload(const unsigned char* pData, unsigned int uLen)
{
    bool bOk = true;
    {
        NetPktGroup group;
        BuildNetWireGroup(group, pData, uLen);
        unsigned char acDst[1028];
        memset(acDst, 0xAA, sizeof(acDst));
        unsigned int uLength = 0;
        NetWire::ReadUploadChunk(group, (char*)acDst, 1012, 800, uLength);
        bOk = bOk && CanaryOk(acDst, 1012, 1028);
        group.Free();
    }
    {
        NetPktGroup group;
        BuildNetWireGroup(group, pData, uLen);
        unsigned char acName[32];
        memset(acName, 0xAA, sizeof(acName));
        NetWire::ReadString(group, (char*)acName, 16);
        NetWire::IsSafeUploadName((const char*)acName);
        bOk = bOk && CanaryOk(acName, 16, 32);
        group.Free();
    }
    return bOk;
}

static bool ReplayRcon(const unsigned char* pData, unsigned int uLen)
{
    bool bOk = true;
    if (uLen >= 9)
    {
        unsigned short ausOut[72];
        memset(ausOut, 0xAA, sizeof(ausOut));
        NetWire::ReadRconLine((const char*)pData + 9, (int)uLen - 9, pData[8], ausOut);
        for (int i = 64; i < 72; ++i)
        {
            bOk = bOk && ausOut[i] == 0xAAAA;
        }
    }
    NetWire::IsRconAuthorized((const char*)pData, (int)uLen, 'k', 'm');
    return bOk;
}

static bool ReplayBitReader(const unsigned char* pData, unsigned int uLen, bool bTurnRate)
{
    NetPktGroup group;
    BuildNetWireGroup(group, pData, uLen);
    unsigned int uFirst = 0;
    unsigned int uSecond = 0;
    if (bTurnRate)
    {
        group.ReadBits(uFirst, 10);
        group.ReadBits(uSecond, 3);
        NetWire::IsValidTurnRate((int)uFirst, (int)uSecond);
    }
    else
    {
        group.ReadBits(uFirst, 7);
        NetWire::ClampPlayerCount(uFirst);
    }
    group.Free();
    return true;
}

bool ReplayNetWireBytes(const char* pcReader, const unsigned char* pData, unsigned int uLen)
{
    NetTestReset();
    bool bOk = true;
    if (strcmp(pcReader, "chat-client") == 0)
    {
        bOk = ReplayChat(pData, uLen, true);
    }
    else if (strcmp(pcReader, "chat-server") == 0)
    {
        bOk = ReplayChat(pData, uLen, false);
    }
    else if (strcmp(pcReader, "upload") == 0)
    {
        bOk = ReplayUpload(pData, uLen);
    }
    else if (strcmp(pcReader, "rcon") == 0)
    {
        bOk = ReplayRcon(pData, uLen);
    }
    else if (strcmp(pcReader, "load") == 0)
    {
        bOk = ReplayBitReader(pData, uLen, false);
    }
    else if (strcmp(pcReader, "turnrate") == 0)
    {
        bOk = ReplayBitReader(pData, uLen, true);
    }
    else
    {
        return false;
    }
    return bOk && NetCountPackets(0) == 512;
}

static void ReplayReaderCorpus(const char* pcReader)
{
    std::vector<std::string> files = ListCorpus(pcReader, false);
    REQUIRE(!files.empty());
    for (size_t i = 0; i < files.size(); ++i)
    {
        ByteVec bytes;
        REQUIRE(LoadCorpusFile(files[i], bytes));
        WatchdogArm(files[i].c_str());
        INFO(files[i]);
        CHECK(ReplayNetWireBytes(pcReader, bytes.empty() ? 0 : &bytes[0], (unsigned int)bytes.size()));
    }
}

TEST_CASE("corpus replay chat-client" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("chat-client");
}

TEST_CASE("corpus replay chat-server" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("chat-server");
}

TEST_CASE("corpus replay upload" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("upload");
}

TEST_CASE("corpus replay rcon" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("rcon");
}

TEST_CASE("corpus replay load" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("load");
}

TEST_CASE("corpus replay turnrate" * doctest::test_suite("corpus"))
{
    ReplayReaderCorpus("turnrate");
}

static void PutWord(ByteVec& b, unsigned int uValue)
{
    for (int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)(uValue >> (8 * i)));
    }
}

static void PutBits64(ByteVec& b, unsigned long long ullValue)
{
    PutWord(b, (unsigned int)ullValue);
    PutWord(b, (unsigned int)(ullValue >> 32));
}

static void SaveCorpus(const char* pcReader, const char* pcName, const ByteVec& bytes)
{
    std::string sDir = std::string("corpus\\") + pcReader;
    CreateDirectoryA(sDir.c_str(), 0);
    std::string sPath = sDir + "\\" + pcName;
    FILE* pFile = fopen(sPath.c_str(), "wb");
    REQUIRE(pFile);
    fwrite(&bytes[0], 1, bytes.size(), pFile);
    fclose(pFile);
}

static ByteVec ChatPayload(unsigned int uLen)
{
    ByteVec b;
    PutWord(b, 0xffff0000u | uLen);
    b.insert(b.end(), uLen, 0x41);
    PutWord(b, 0xDEADBEEFu);
    return b;
}

TEST_CASE("corpus generate netwire" * doctest::test_suite("corpus"))
{
    const char* pcGen = getenv("OPENSWBF2_GEN_CORPUS");
    if (!pcGen || pcGen[0] != '1')
    {
        return;
    }
    SaveCorpus("chat-client", "chat-client-oversize.bin", ChatPayload(600));
    SaveCorpus("chat-server", "chat-server-oversize.bin", ChatPayload(400));

    ByteVec chunk;
    PutWord(chunk, 1023);
    chunk.insert(chunk.end(), 1023, 0x41);
    PutWord(chunk, 0xDEADBEEFu);
    SaveCorpus("upload", "upload-oversize-chunk.bin", chunk);

    const char acName[] = "..\\evil";
    ByteVec name;
    name.push_back((unsigned char)(sizeof(acName) - 1));
    name.insert(name.end(), acName, acName + sizeof(acName) - 1);
    SaveCorpus("upload", "upload-traversal-name.bin", name);

    ByteVec line;
    unsigned char acHeader[9] = {0, 0, 0, 0, 0, 'J', 0, 'k', 200};
    line.insert(line.end(), acHeader, acHeader + 9);
    line.insert(line.end(), 200, 0x41);
    SaveCorpus("rcon", "rcon-long-line.bin", line);

    ByteVec shortAuth;
    unsigned char acShort[7] = {0, 0, 0, 0, 0, ')', 0};
    shortAuth.insert(shortAuth.end(), acShort, acShort + 7);
    SaveCorpus("rcon", "rcon-short-auth.bin", shortAuth);

    ByteVec load;
    PutBits64(load, 127ull | ((unsigned long long)0xDEADBEEFu << 7));
    SaveCorpus("load", "load-players-127.bin", load);

    ByteVec turn;
    PutBits64(turn, (unsigned long long)0xDEADBEEFu << 13);
    SaveCorpus("turnrate", "turnrate-zero.bin", turn);
}
