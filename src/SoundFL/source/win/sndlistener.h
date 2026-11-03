#pragma once

#include "SoundFL/source/sndlistenerbase.h"

namespace Snd
{

class Listener : public ListenerBase
{
public:

    // FUNCTION: SWBF2 0x00733e80
    Listener()
    {
    }
    // FUNCTION: SWBF2 0x00733e90
    ~Listener()
    {
    }

    void Update();
};

 

}
