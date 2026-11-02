#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/VoiceChatNode.h"
#include "Battlefront2/Source/VoiceChatUtil.h"

namespace
{

void BuildHeader(unsigned char* pBitmap, bool bRelay, unsigned int uiSender, unsigned int uiReceiver)
{
    const unsigned int uiLog2 = 6;
    memset(pBitmap, 0, 9);
    VoiceChat::SetBits(pBitmap, 0, bRelay ? 1 : 0, 1);
    VoiceChat::SetBits(pBitmap, 12, uiSender, uiLog2);
    if (!bRelay)
    {
        VoiceChat::SetBits(pBitmap, 12 + uiLog2, uiReceiver, uiLog2);
    }
}

}

TEST_CASE("voice chat header rejects out of range node ids" * doctest::test_suite("regression"))
{
    const unsigned int uiNodes = 0x32;
    const unsigned int uiLog2 = 6;
    unsigned char acBitmap[9];

    BuildHeader(acBitmap, true, 63, 0);
    CHECK_FALSE(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));
    BuildHeader(acBitmap, true, 50, 0);
    CHECK_FALSE(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));
    BuildHeader(acBitmap, true, 49, 0);
    CHECK(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));

    BuildHeader(acBitmap, false, 63, 1);
    CHECK_FALSE(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));
    BuildHeader(acBitmap, false, 1, 63);
    CHECK_FALSE(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));
    BuildHeader(acBitmap, false, 1, 50);
    CHECK_FALSE(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));
    BuildHeader(acBitmap, false, 0, 49);
    CHECK(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2));

    for (unsigned int uiSender = 0; uiSender < 64; ++uiSender)
    {
        BuildHeader(acBitmap, true, uiSender, 0);
        CHECK(VoiceChat::Node::Header::HasValidNodes(acBitmap, uiNodes, uiLog2) == (uiSender < uiNodes));
    }
}

TEST_CASE("voice chat header read rejects out of range node ids" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\VoiceChatNode.cpp", sText));
    const size_t uRead = sText.find("bool Node::Header::Read(const Transport& transport, NetPacketAudio* const pPacket)");
    REQUIRE(uRead != std::string::npos);
    const size_t uEnd = sText.find("\n}", uRead);
    REQUIRE(uEnd != std::string::npos);
    const std::string sBody = sText.substr(uRead, uEnd - uRead);
    const size_t uCheck = sBody.find("if (!HasValidNodes(mBitmap, transport.GetNumNodes(), transport.GetNumNodesLog2()))");
    REQUIRE(uCheck != std::string::npos);
    const size_t uBits = sBody.find("mBits = nBits;");
    REQUIRE(uBits != std::string::npos);
    CHECK(uCheck < uBits);
}

TEST_CASE("voice chat local node ids stay below the transport node count" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\VoiceChatNode.cpp", sText));
    const size_t uSetID = sText.find("void VoiceChat::Node::SetID(unsigned char uChannel)");
    REQUIRE(uSetID != std::string::npos);
    const size_t uOld = sText.find("const unsigned char uOld = mNodeID;", uSetID);
    REQUIRE(uOld != std::string::npos);
    const std::string sHead = sText.substr(uSetID, uOld - uSetID);
    CHECK(sHead.find("uChannel >= VoiceChat::Node::sTransport->GetNumNodes()") != std::string::npos);
    CHECK(sHead.find("uChannel = 0xFF;") != std::string::npos);

    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\GameVoiceChatTransport.cpp", sText));
    CHECK(sText.find("if (other < 0 || other == node || (unsigned int)other >= mNumNodes)") != std::string::npos);
}

TEST_CASE("voice chat header bitmap holds a relay header" * doctest::test_suite("regression"))
{
    const unsigned int uiNodes = 0x32;
    const unsigned int uiNodesLog2 = 6;
    CHECK(VoiceChat::Node::Header::GetMaxBytes(uiNodes, uiNodesLog2) == 9);
    CHECK(VoiceChat::Node::Header::GetMaxBytes(uiNodes, uiNodesLog2) * 8 >= 12 + uiNodesLog2 + uiNodes);
}

TEST_CASE("voice chat header bitmap holds every header format" * doctest::test_suite("regression"))
{
    for (unsigned int uiNodes = 1; uiNodes < 256; ++uiNodes)
    {
        for (unsigned int uiNodesLog2 = 1; uiNodesLog2 <= 8; ++uiNodesLog2)
        {
            const unsigned int uiBits = VoiceChat::Node::Header::GetMaxBytes(uiNodes, uiNodesLog2) * 8;
            CHECK(uiBits >= 12 + uiNodesLog2 + uiNodes);
            CHECK(uiBits >= 12 + 2 * uiNodesLog2);
        }
    }
}
