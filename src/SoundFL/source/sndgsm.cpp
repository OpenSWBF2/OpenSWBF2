#include <string.h>

#include "sndgsm.h"

struct gsm_state;
extern "C" int __cdecl gsm_decode(gsm_state* state, unsigned char* frame, short* samples);
extern "C" void __cdecl gsm_encode(char* pGsm, short* pFrame, unsigned char* pOut);

namespace Snd
{

extern "C" int __cdecl gsm_option(void* pGsm, int iOption, int* pValue);

// FUNCTION: SWBF2 0x0074bca0
void GSM::Decode(unsigned char (*pFrame)[33], short (*pSamples)[160])
{
    ::gsm_decode((gsm_state*)this, *pFrame, *pSamples);
}

// FUNCTION: SWBF2 0x0074bcc0
void GSM::Encode(short (*pSamples)[160], unsigned char (*pFrame)[33])
{
    ::gsm_encode((char*)this, *pSamples, *pFrame);
}

// FUNCTION: SWBF2 0x0074bce0
void GSM::Init()
{
    memset(this, 0, sizeof(GSM));
    nrp = 40;

    {
        int ucValue = 1;
        gsm_option(this, 2, &ucValue);
    }
    {
        int ucValue = 1;
        gsm_option(this, 3, &ucValue);
    }
}

}
