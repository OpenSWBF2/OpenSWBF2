#include "third_party/doctest/doctest.h"

#include <stdint.h>
#include <string.h>
#include <windows.h>

#include "Battlefront2/Source/CollisionMesh.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"

namespace
{

class TreeStatics
{
public:
    TreeStatics()
        : mBlock(CollisionTreeNode::memblock), mBytes(CollisionTreeNode::membytes),
          mTotal(CollisionTreeNode::memtotal), mIndexPool(CollisionTreeNode::indexpool),
          mNodes(CollisionTreeNode::nodes), mLeafs(CollisionTreeNode::leafs)
    {
    }

    ~TreeStatics()
    {
        CollisionTreeNode::memblock = mBlock;
        CollisionTreeNode::membytes = mBytes;
        CollisionTreeNode::memtotal = mTotal;
        CollisionTreeNode::indexpool = mIndexPool;
        CollisionTreeNode::nodes = mNodes;
        CollisionTreeNode::leafs = mLeafs;
    }

private:
    TreeStatics(const TreeStatics&);
    TreeStatics& operator=(const TreeStatics&);

    char* mBlock;
    unsigned int mBytes;
    unsigned int mTotal;
    unsigned short* mIndexPool;
    int mNodes;
    int mLeafs;
};

const int kTreeIndices = 3 + 4 + 5;
const int kPoolBytes = 0x100;

struct BuiltTree
{
    char* pcBlock;
    CollisionTreeBranch* pRoot;
    CollisionTreeLeaf* pLeft;
    CollisionTreeBranch* pRight;
    CollisionTreeLeaf* pRightLeft;
    CollisionTreeLeaf* pRightRight;
    unsigned short* apuIndices[3];
};

void MovePoolToBlock(char* pcBlock)
{
    char* pcHeap = CollisionTreeNode::memblock;
    CollisionTreeNode::indexpool =
        (unsigned short*)(pcBlock + ((char*)CollisionTreeNode::indexpool - pcHeap));
    CollisionTreeNode::memblock = pcBlock;
    delete[] pcHeap;
}

void BuildTree(BuiltTree& kTree)
{
    kTree.pcBlock = CollisionTreeNode::memblock;
    kTree.pRoot = new CollisionTreeBranch;
    kTree.apuIndices[0] = CollisionTreeNode::indexpool;
    kTree.pLeft = new CollisionTreeLeaf(3);
    kTree.pRoot->SetLChild(kTree.pLeft);
    kTree.pRight = new CollisionTreeBranch;
    kTree.apuIndices[1] = CollisionTreeNode::indexpool;
    kTree.pRightLeft = new CollisionTreeLeaf(4);
    kTree.pRight->SetLChild(kTree.pRightLeft);
    kTree.apuIndices[2] = CollisionTreeNode::indexpool;
    kTree.pRightRight = new CollisionTreeLeaf(5);
    kTree.pRight->SetGChild(kTree.pRightRight);
    kTree.pRoot->SetGChild(kTree.pRight);
}

void CheckTree(const BuiltTree& kTree)
{
    CHECK((char*)kTree.pRoot == kTree.pcBlock);
    CHECK((char*)kTree.pLeft == (char*)kTree.pRoot + sizeof(CollisionTreeNode));
    CHECK((char*)kTree.pRightLeft == (char*)kTree.pRight + sizeof(CollisionTreeNode));
    CHECK(kTree.pRoot->GetGChild() == kTree.pRight);
    CHECK(kTree.pRight->GetGChild() == kTree.pRightRight);
    CHECK(kTree.pLeft->GetIndexPointer() == kTree.apuIndices[0]);
    CHECK(kTree.pRightLeft->GetIndexPointer() == kTree.apuIndices[1]);
    CHECK(kTree.pRightRight->GetIndexPointer() == kTree.apuIndices[2]);
    CHECK(kTree.apuIndices[1] == kTree.apuIndices[0] + 3);
    CHECK(kTree.apuIndices[2] == kTree.apuIndices[1] + 4);
    CHECK_FALSE(kTree.pRoot->IsLeaf());
    CHECK_FALSE(kTree.pRight->IsLeaf());
    CHECK(kTree.pLeft->IsLeaf());
    CHECK(kTree.pRightLeft->IsLeaf());
    CHECK(kTree.pRightRight->IsLeaf());
}

}

TEST_CASE("collision tree offsets resolve above 4 GB" * doctest::test_suite("regression"))
{
    TreeStatics kSaved;
    const SIZE_T uSize = 0x10000;
    char* pcBlock = (char*)VirtualAlloc(nullptr, uSize, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
    REQUIRE(pcBlock != (char*)0);
    CHECK((sizeof(void*) == 4 || (uint64_t)(uintptr_t)pcBlock > 0xFFFFFFFFull));

    CollisionTreeNode::AddAllocPool((int)uSize, kTreeIndices);
    MovePoolToBlock(pcBlock);
    CHECK((char*)CollisionTreeNode::indexpool == pcBlock + uSize - 2 * kTreeIndices);

    BuiltTree kTree;
    BuildTree(kTree);
    CHECK(CollisionTreeNode::nodes == 2);
    CHECK(CollisionTreeNode::leafs == 3);
    CHECK(CollisionTreeNode::membytes == 5 * sizeof(CollisionTreeNode));
    CHECK(kTree.pcBlock == pcBlock);
    CheckTree(kTree);

    CHECK(VirtualFree(pcBlock, 0, MEM_RELEASE));
}

TEST_CASE("collision tree offsets survive a second mesh" * doctest::test_suite("regression"))
{
    TreeStatics kSaved;

    CollisionTreeNode::AddAllocPool(kPoolBytes, kTreeIndices);
    BuiltTree kFirst;
    BuildTree(kFirst);

    CollisionTreeNode::AddAllocPool(kPoolBytes, kTreeIndices);
    BuiltTree kSecond;
    BuildTree(kSecond);
    REQUIRE(kFirst.pcBlock != kSecond.pcBlock);
    REQUIRE(CollisionTreeNode::memblock == kSecond.pcBlock);

    CheckTree(kFirst);
    CheckTree(kSecond);

    delete[] kFirst.pcBlock;
    delete[] kSecond.pcBlock;
}

extern const unsigned int* gStubChunkIds;
extern int gStubChunkIdsLeft;
extern const unsigned char* gStubFileBytes;
extern int gStubFileBytesLeft;

TEST_CASE("collision tree reader builds nodes and leaves from the chunk" * doctest::test_suite("regression"))
{
    TreeStatics kSaved;
    const unsigned int auIds[] = {CHUNK_ID('N', 'O', 'D', 'E'), CHUNK_ID('L', 'E', 'A', 'F'), CHUNK_ID('N', 'O', 'D', 'E'),
                                  CHUNK_ID('L', 'E', 'A', 'F'), CHUNK_ID('L', 'E', 'A', 'F')};
    const unsigned char auBytes[] = {
        1, 2, 3, 4, 5, 6,
        3, 7, 8, 9, 10, 11, 12, 100, 0, 102, 0, 101, 0,
        13, 14, 15, 16, 17, 18,
        4, 19, 20, 21, 22, 23, 24, 200, 0, 203, 0, 202, 0, 201, 0,
        5, 25, 26, 27, 28, 29, 30, 44, 1, 48, 1, 47, 1, 46, 1, 45, 1,
    };
    gStubChunkIds = auIds;
    gStubChunkIdsLeft = (int)(sizeof(auIds) / sizeof(auIds[0]));
    gStubFileBytes = auBytes;
    gStubFileBytesLeft = (int)sizeof(auBytes);

    CollisionTreeNode::AddAllocPool(kPoolBytes, kTreeIndices);
    char* pcBlock = CollisionTreeNode::memblock;
    unsigned short* puIndices = CollisionTreeNode::indexpool;
    alignas(PblFile) unsigned char acFile[sizeof(PblFile)];
    alignas(PblFileChunk) unsigned char acChunk[sizeof(PblFileChunk)];
    memset(acChunk, 0, sizeof(acChunk));
    PblFileChunk& kChunk = *reinterpret_cast<PblFileChunk*>(acChunk);
    kChunk._pFile = reinterpret_cast<PblFile*>(acFile);
    CollisionTreeNode* pRoot = ReadTree(kChunk);
    const int iIdsLeft = gStubChunkIdsLeft;
    const int iBytesLeft = gStubFileBytesLeft;
    gStubChunkIdsLeft = 0;
    gStubFileBytesLeft = 0;

    CHECK(iIdsLeft == 0);
    CHECK(iBytesLeft == 0);
    REQUIRE((char*)pRoot == pcBlock);
    CHECK(CollisionTreeNode::nodes == 2);
    CHECK(CollisionTreeNode::leafs == 3);
    CHECK(CollisionTreeNode::membytes == 5 * sizeof(CollisionTreeNode));

    const unsigned char auRoot[] = {1, 2, 3, 4, 5, 6};
    CHECK_FALSE(pRoot->IsLeaf());
    CHECK(memcmp(&pRoot->aabb, auRoot, sizeof(auRoot)) == 0);

    CollisionTreeLeaf* pLeft = static_cast<CollisionTreeLeaf*>(pRoot + 1);
    REQUIRE(pLeft->IsLeaf());
    CHECK(pLeft->numVerts == 3);
    const unsigned char auLeft[] = {7, 8, 9, 10, 11, 12};
    CHECK(memcmp(&pLeft->aabb, auLeft, sizeof(auLeft)) == 0);
    REQUIRE(pLeft->GetIndexPointer() == puIndices);
    CHECK(puIndices[0] == 100);
    CHECK(puIndices[1] == 101);
    CHECK(puIndices[2] == 102);

    CollisionTreeNode* pRight = pRoot->GetGChild();
    REQUIRE((char*)pRight == pcBlock + 2 * sizeof(CollisionTreeNode));
    CHECK_FALSE(pRight->IsLeaf());
    const unsigned char auRight[] = {13, 14, 15, 16, 17, 18};
    CHECK(memcmp(&pRight->aabb, auRight, sizeof(auRight)) == 0);

    CollisionTreeLeaf* pRightLeft = static_cast<CollisionTreeLeaf*>(pRight + 1);
    REQUIRE(pRightLeft->IsLeaf());
    CHECK(pRightLeft->numVerts == 4);
    REQUIRE(pRightLeft->GetIndexPointer() == puIndices + 3);
    CHECK(puIndices[3] == 200);
    CHECK(puIndices[4] == 201);
    CHECK(puIndices[5] == 202);
    CHECK(puIndices[6] == 203);

    CollisionTreeLeaf* pRightRight = static_cast<CollisionTreeLeaf*>(pRight->GetGChild());
    REQUIRE((char*)pRightRight == pcBlock + 4 * sizeof(CollisionTreeNode));
    REQUIRE(pRightRight->IsLeaf());
    CHECK(pRightRight->numVerts == 5);
    const unsigned char auRightRight[] = {25, 26, 27, 28, 29, 30};
    CHECK(memcmp(&pRightRight->aabb, auRightRight, sizeof(auRightRight)) == 0);
    REQUIRE(pRightRight->GetIndexPointer() == puIndices + 7);
    CHECK(puIndices[7] == 300);
    CHECK(puIndices[8] == 301);
    CHECK(puIndices[9] == 302);
    CHECK(puIndices[10] == 303);
    CHECK(puIndices[11] == 304);

    delete[] pcBlock;
}
