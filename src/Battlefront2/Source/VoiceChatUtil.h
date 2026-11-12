#ifndef VOICECHATUTIL_H
#define VOICECHATUTIL_H

namespace VoiceChat
{
unsigned char* GetBitmapElementAndMask(unsigned char* const bitmap, unsigned int bit,
                                       unsigned char& mask);
void SetBit(unsigned char* const bitmap, unsigned int bit, bool value);

const unsigned char* GetBitmapElementAndMask(const unsigned char* const bitmap, unsigned int bit,
                                             unsigned char& mask);
bool GetBit(const unsigned char* const bitmap, unsigned int bit);

unsigned int GetMSBIndex(unsigned int uiValue);

void SetBits(unsigned char* const bitmap, unsigned int value, unsigned int bit, unsigned int count);
unsigned int GetBits(const unsigned char* const bitmap, unsigned int bit, unsigned int count);

struct AudioPacket
{
    void* mData;
    unsigned int mSize;
    void* mAppData;
    bool mDiscontinuity : 1;
};

class Encoder
{
public:
    Encoder();
    AudioPacket* GetData();

    void FreeAudioPacket(AudioPacket* packet);
    void Play();

    bool Open(unsigned int captureDeviceIndex, float minBufferSizeMs, unsigned int minBuffers);
    void Close();
    void Stop();
    bool IsEnabled();
    unsigned int GetMaxOutputPacketSizeBytes() const;
    float GetMaxOutputPacketSizeMs() const;
    void SetGain(float fGain);

    void* mAppData;
};

class Decoder
{
public:
    Decoder();
    void Play();
    bool IsEnabled();

    bool ReceivePacket(unsigned int input, const AudioPacket* packet);
    bool Update();
    bool IsStarving(unsigned int input, bool force);

    bool Open(unsigned int playbackDevice, float fPacketMs, unsigned int minBuffers, unsigned int iSlots);
    void Close();
    void Stop();

    void SetGain(float fGain, unsigned int uiOutput);
    float GetGain(unsigned int uiOutput) const;
    unsigned int GetNumOutputs() const;

    void* mAppData;
};

}

#endif
