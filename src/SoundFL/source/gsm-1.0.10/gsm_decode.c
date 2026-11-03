#include "private.h"

static void Postprocessing(state, samples)
struct gsm_state* state;
short* samples;
{
    int k;
    short previous = state->msr;
    short temp;
    longword ltmp;
    for (k = 160; k--; ++samples)
    {
        temp = (word)GSM_MULT_R(previous, 28180);
        previous = (word)GSM_ADD(*samples, temp);
        *samples = (word)(GSM_ADD(previous, previous) & 0xfff8);
    }
    state->msr = previous;
}

// FUNCTION: SWBF2 0x00750630
// Gsm_Decoder
void __cdecl Gsm_Decoder(state, codedRatios, lag, gain, grid, maximum, coded, output)
struct gsm_state* state;
const short* codedRatios;
const short* lag;
const short* gain;
const short* grid;
const short* maximum;
const short* coded;
short* output;
{
    int j, k;
    short residual[40], samples[160];
    short* history = state->dp0 + 120;
    for (j = 0; j <= 3; ++j, ++maximum, ++gain, ++lag, ++grid, coded += 13)
    {
        Gsm_RPE_Decoding(state, *maximum, *grid, coded, residual);
        Gsm_Long_Term_Synthesis_Filtering(state, *lag, *gain, residual, history);
        for (k = 0; k <= 39; ++k) samples[j * 40 + k] = history[k];
    }
    Gsm_Short_Term_Synthesis_Filter(state, codedRatios, samples, output);
    Postprocessing(state, output);
}

// FUNCTION: SWBF2 0x00750820
// gsm_decode
int __cdecl gsm_decode(s, c, target)
struct gsm_state* s;
unsigned char* c;
short* target;
{
    short LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13 * 4];

    if (((*c >> 4) & 0x0F) != 0xD)
    {
        return -1;
    }

    LARc[0]  = (short)((*c++ & 0xF) << 2);
    LARc[0] |= (*c >> 6) & 0x3;
    LARc[1]  = *c++ & 0x3F;
    LARc[2]  = (*c >> 3) & 0x1F;
    LARc[3]  = (short)((*c++ & 0x7) << 2);
    LARc[3] |= (*c >> 6) & 0x3;
    LARc[4]  = (*c >> 2) & 0xF;
    LARc[5]  = (short)((*c++ & 0x3) << 2);
    LARc[5] |= (*c >> 6) & 0x3;
    LARc[6]  = (*c >> 3) & 0x7;
    LARc[7]  = *c++ & 0x7;

    Nc[0]  = (*c >> 1) & 0x7F;
    bc[0]  = (short)((*c++ & 0x1) << 1);
    bc[0] |= (*c >> 7) & 0x1;
    Mc[0]  = (*c >> 5) & 0x3;
    xmaxc[0]  = (short)((*c++ & 0x1F) << 1);
    xmaxc[0] |= (*c >> 7) & 0x1;
    xmc[0]  = (*c >> 4) & 0x7;
    xmc[1]  = (*c >> 1) & 0x7;
    xmc[2]  = (short)((*c++ & 0x1) << 2);
    xmc[2] |= (*c >> 6) & 0x3;
    xmc[3]  = (*c >> 3) & 0x7;
    xmc[4]  = *c++ & 0x7;
    xmc[5]  = (*c >> 5) & 0x7;
    xmc[6]  = (*c >> 2) & 0x7;
    xmc[7]  = (short)((*c++ & 0x3) << 1);
    xmc[7] |= (*c >> 7) & 0x1;
    xmc[8]  = (*c >> 4) & 0x7;
    xmc[9]  = (*c >> 1) & 0x7;
    xmc[10]  = (short)((*c++ & 0x1) << 2);
    xmc[10] |= (*c >> 6) & 0x3;
    xmc[11]  = (*c >> 3) & 0x7;
    xmc[12]  = *c++ & 0x7;

    Nc[1]  = (*c >> 1) & 0x7F;
    bc[1]  = (short)((*c++ & 0x1) << 1);
    bc[1] |= (*c >> 7) & 0x1;
    Mc[1]  = (*c >> 5) & 0x3;
    xmaxc[1]  = (short)((*c++ & 0x1F) << 1);
    xmaxc[1] |= (*c >> 7) & 0x1;
    xmc[13]  = (*c >> 4) & 0x7;
    xmc[14]  = (*c >> 1) & 0x7;
    xmc[15]  = (short)((*c++ & 0x1) << 2);
    xmc[15] |= (*c >> 6) & 0x3;
    xmc[16]  = (*c >> 3) & 0x7;
    xmc[17]  = *c++ & 0x7;
    xmc[18]  = (*c >> 5) & 0x7;
    xmc[19]  = (*c >> 2) & 0x7;
    xmc[20]  = (short)((*c++ & 0x3) << 1);
    xmc[20] |= (*c >> 7) & 0x1;
    xmc[21]  = (*c >> 4) & 0x7;
    xmc[22]  = (*c >> 1) & 0x7;
    xmc[23]  = (short)((*c++ & 0x1) << 2);
    xmc[23] |= (*c >> 6) & 0x3;
    xmc[24]  = (*c >> 3) & 0x7;
    xmc[25]  = *c++ & 0x7;

    Nc[2]  = (*c >> 1) & 0x7F;
    bc[2]  = (short)((*c++ & 0x1) << 1);
    bc[2] |= (*c >> 7) & 0x1;
    Mc[2]  = (*c >> 5) & 0x3;
    xmaxc[2]  = (short)((*c++ & 0x1F) << 1);
    xmaxc[2] |= (*c >> 7) & 0x1;
    xmc[26]  = (*c >> 4) & 0x7;
    xmc[27]  = (*c >> 1) & 0x7;
    xmc[28]  = (short)((*c++ & 0x1) << 2);
    xmc[28] |= (*c >> 6) & 0x3;
    xmc[29]  = (*c >> 3) & 0x7;
    xmc[30]  = *c++ & 0x7;
    xmc[31]  = (*c >> 5) & 0x7;
    xmc[32]  = (*c >> 2) & 0x7;
    xmc[33]  = (short)((*c++ & 0x3) << 1);
    xmc[33] |= (*c >> 7) & 0x1;
    xmc[34]  = (*c >> 4) & 0x7;
    xmc[35]  = (*c >> 1) & 0x7;
    xmc[36]  = (short)((*c++ & 0x1) << 2);
    xmc[36] |= (*c >> 6) & 0x3;
    xmc[37]  = (*c >> 3) & 0x7;
    xmc[38]  = *c++ & 0x7;

    Nc[3]  = (*c >> 1) & 0x7F;
    bc[3]  = (short)((*c++ & 0x1) << 1);
    bc[3] |= (*c >> 7) & 0x1;
    Mc[3]  = (*c >> 5) & 0x3;
    xmaxc[3]  = (short)((*c++ & 0x1F) << 1);
    xmaxc[3] |= (*c >> 7) & 0x1;
    xmc[39]  = (*c >> 4) & 0x7;
    xmc[40]  = (*c >> 1) & 0x7;
    xmc[41]  = (short)((*c++ & 0x1) << 2);
    xmc[41] |= (*c >> 6) & 0x3;
    xmc[42]  = (*c >> 3) & 0x7;
    xmc[43]  = *c++ & 0x7;
    xmc[44]  = (*c >> 5) & 0x7;
    xmc[45]  = (*c >> 2) & 0x7;
    xmc[46]  = (short)((*c++ & 0x3) << 1);
    xmc[46] |= (*c >> 7) & 0x1;
    xmc[47]  = (*c >> 4) & 0x7;
    xmc[48]  = (*c >> 1) & 0x7;
    xmc[49]  = (short)((*c++ & 0x1) << 2);
    xmc[49] |= (*c >> 6) & 0x3;
    xmc[50]  = (*c >> 3) & 0x7;
    xmc[51]  = *c++ & 0x7;

    Gsm_Decoder(s, LARc, Nc, bc, Mc, xmaxc, xmc, target);
    return 0;
}
