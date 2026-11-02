#include <string.h>

#include "sndgsm.h"

extern int __cdecl gsm_decode(Snd::GSM* state, unsigned char* frame, short* samples);
extern void __cdecl gsm_encode(Snd::GSM* pGsm, short* pFrame, unsigned char* pOut);

namespace Snd
{

void __cdecl gsm_option(GSM* pGsm, int iOption, unsigned char* pValue);

// FUNCTION: SWBF2 0x0074bca0
void GSM::Decode(unsigned char (*pFrame)[33], short (*pSamples)[160])
{
    ::gsm_decode(this, *pFrame, *pSamples);
}

// FUNCTION: SWBF2 0x0074bcc0
void GSM::Encode(short (*pSamples)[160], unsigned char (*pFrame)[33])
{
    ::gsm_encode(this, *pSamples, *pFrame);
}

// FUNCTION: SWBF2 0x0074bce0
void GSM::Init()
{
    memset(this, 0, sizeof(GSM));
    nrp = 40;

    {
        unsigned char ucValue = 1;
        gsm_option(this, 2, &ucValue);
    }
    {
        unsigned char ucValue = 1;
        gsm_option(this, 3, &ucValue);
    }
}

}
