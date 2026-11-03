#pragma once

namespace Snd
{

class Bus;
class SourceTransport;
class Source;

class SourceBus
{
public:
    SourceBus();

    ~SourceBus();

    void Stop(SourceTransport* pTransport);

    float GetGain() const;

    void SetGain(Source* pSource, float fGain);

    void SetBus(Bus* pBus);

    void Play(SourceTransport* pTransport);

    Bus* GetBus() const;

protected:

    void Update(Source* pSource, SourceTransport* pTransport);

public:

    Bus* mBus;
    float mGain;
    unsigned char mPlayState;
};

 

}
