#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <algorithm>

#include "testutil.h"
#include "nettest.h"
#include "corpus_replay.h"

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
    if (uiSize >= 5)
    {
        NetDeliverDatagram(pData, uiSize, 3);
    }
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

static void ReplayKnownBug(const char* pcName)
{
    ByteVec bytes;
    if (!LoadCorpusFile(std::string("corpus\\recvgroup\\known_bug\\") + pcName + ".bin", bytes))
    {
        HarnessError("known_bug input missing");
    }
    WatchdogArm(pcName);
    CHECK(ReplayRecvGroupBytes(bytes));
}

TEST_CASE("corpus known bug frag-idx-ge-count" * doctest::test_suite("known_bug"))
{
    ReplayKnownBug("frag-idx-ge-count");
}

TEST_CASE("corpus known bug frag-classic-gap" * doctest::test_suite("known_bug"))
{
    ReplayKnownBug("frag-classic-gap");
}
