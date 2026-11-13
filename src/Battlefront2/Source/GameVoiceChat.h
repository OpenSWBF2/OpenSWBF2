#pragma once

namespace GameVoiceChat
{
    void Close();
    void Reset();

    class Encoder;
    class Decoder;
    class Node;
    extern Encoder* gEncoders[1];
    extern Decoder* gDecoders[1];
    extern Node* gNodes[1];
    extern int gHeap;
    extern unsigned char* gEncoderStorage;
    extern unsigned char* gDecoderStorage;
    extern unsigned char* gNodeStorage;
    Node* GetNode(unsigned int nodeIndex);
    Node* FindNodeByDeviceID(const unsigned int deviceID);
    Node* GetEnabledNode(unsigned int uiEnabledIndex, unsigned int& uiIndex);
    Node* GetEnabledNode(unsigned int uiEnabledIndex);
    unsigned int GetNumEnabledNodes();
    void SetNodeID(Node* const pNode, unsigned char uNodeID);
    void Update(float fDeltaTime);
    bool IsOpen();

    class Transport;
    Transport* GetTransport();
    extern Transport gTransport;

    bool IsSuspended();
    unsigned int GetNumNodes();
    void PlayerChange();
    bool IsBanned();
    unsigned int CalculatePlayCaptureBufferSizeSamples(float fMilliseconds);
    unsigned int CalculateEncodedPacketSize(unsigned int uiSamples);
}
