#include "CollisionMesh.h"

#include "PebbleFL/Common/PblChunk.h"

// FUNCTION: SWBF2 0x00460f00
void CollisionTreeNode::AddAllocPool(int iBytes, int iIndices)
{
    membytes = 0;
    memtotal = iBytes;
    memblock = new char[iBytes];
    nodes = 0;
    leafs = 0;
    indexpool = (unsigned short*)(memblock + iBytes - 2 * iIndices);
}

// FUNCTION: SWBF2 0x00460fd0
CollisionTreeNode* ReadTree(PblFileChunk& chunk)
{
    PblFileChunk child = chunk.ReadNextChild();
    switch (child._iId)
    {
    case CHUNK_ID('N', 'O', 'D', 'E'):
    {
        CollisionTreeBranch* pBranch = new CollisionTreeBranch;
        child._pFile->ReadData(&pBranch->aabb, 6);
        pBranch->SetLChild(ReadTree(chunk));
        pBranch->SetGChild(ReadTree(chunk));
        return pBranch;
    }
    case CHUNK_ID('L', 'E', 'A', 'F'):
    {
        unsigned char uNumVerts = child._pFile->Read8();
        CollisionTreeLeaf* pLeaf = new CollisionTreeLeaf(uNumVerts);
        child._pFile->ReadData(&pLeaf->aabb, 6);
        unsigned short* pIndices = pLeaf->GetIndexPointer();
        if (pIndices == 0)
        {
            return 0;
        }
        pIndices[0] = child._pFile->Read16();
        for (int i = pLeaf->numVerts - 1; i > 0; --i)
        {
            pIndices[i] = child._pFile->Read16();
        }
        return pLeaf;
    }
    }
    return 0;
}

// GLOBAL: SWBF2 0x01e30728
char* CollisionTreeNode::memblock;

// GLOBAL: SWBF2 0x01e30730
unsigned int CollisionTreeNode::membytes;

// GLOBAL: SWBF2 0x01e3072c
unsigned int CollisionTreeNode::memtotal;

// GLOBAL: SWBF2 0x01e3039c
unsigned short* CollisionTreeNode::indexpool;

// GLOBAL: SWBF2 0x01e30398
int CollisionTreeNode::nodes;

// GLOBAL: SWBF2 0x01e30370
int CollisionTreeNode::leafs;
