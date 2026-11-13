#pragma once

#include <stddef.h>
#include "sndsourcetransport.h"

namespace Snd
{

class StreamPacket;

// SYNTHETIC: SWBF2 0x0073c810
// Snd::StreamNodeSource::`scalar deleting destructor'

// VTABLE: SWBF2 0x0078686c
class StreamNodeSource
{
public:
    // FUNCTION: SWBF2 0x0073c790
    virtual ~StreamNodeSource()
    {
    }
    virtual StreamPacket* GetPacket(unsigned int uiOutput);
};

 

struct StreamNodeConnection
{

    // FUNCTION: SWBF2 0x0073d0f0
    StreamNodeConnection() : mInputNode(0), mInputNodeOutputIndex(0) {}

    // FUNCTION: SWBF2 0x0053c650
    ~StreamNodeConnection() {}

    void Initialize(StreamNodeSource* const pNode, unsigned int uiOutput)
    {
        mInputNode = pNode;
        mInputNodeOutputIndex = uiOutput;
    }

    StreamNodeSource* GetInputNode() const { return mInputNode; }

    StreamPacket* GetPacket() const
    {
        return mInputNode ? mInputNode->GetPacket(mInputNodeOutputIndex) : 0;
    }

    StreamNodeSource* mInputNode;
    unsigned int mInputNodeOutputIndex;
};

class StreamNodeSink
{
public:
    void Connect(StreamNodeSource* pSource, unsigned int uiOutput)
    {
        mConnection.mInputNode = pSource;
        mConnection.mInputNodeOutputIndex = uiOutput;
    }

    StreamNodeSource* ConnectionGetNode() const
    {
        return mConnection.GetInputNode();
    }

    StreamPacket* ConnectionGetPacket() const
    {
        return mConnection.GetPacket();
    }

    StreamNodeConnection mConnection;
};

class StreamNodeSinkNInput
{
public:

    StreamNodeSinkNInput();
    ~StreamNodeSinkNInput();

    void Connect(StreamNodeSource* const pSource, unsigned int uiOutput,
                 unsigned int uiInput);

    StreamNodeSource* ConnectionGetNode(unsigned int uiInput) const;

    unsigned int GetNumInputs() const;

    StreamPacket* ConnectionGetPacket(unsigned int uiInput) const;

    void SetConnectionArray(StreamNodeConnection* const pArray,
                            unsigned int uiCount);

    StreamNodeConnection* mConnections;
    unsigned int mNumConnections;
};

class StreamNode : public SourceTransport
{
public:
    StreamNode() : mFlags(0) {}

    bool IsStarving() const;

    bool IsFlushing() const;

protected:

    void SetStarved(bool bStarved);

    void StartFlush();
    void EndFlush();

    bool UpdateNode(bool bAllowPausing);

public:
    unsigned char mFlags;
};

 
 
 

 
 

}
