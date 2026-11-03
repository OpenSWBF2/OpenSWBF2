#include "private.h"

// FUNCTION: SWBF2 0x00751c10
// Coefficients_0_12
static void __cdecl Coefficients_0_12(first, second, output)
const short* first;
const short* second;
short* output;
{
    long ltmp;
    for (int i = 1; i <= 8; ++i, ++first, ++second, ++output)
    {
        *output = (short)GSM_ADD(SASR(*first, 2), SASR(*second, 2));
        *output = (short)GSM_ADD(*output, SASR(*first, 1));
    }
}

// FUNCTION: SWBF2 0x00751ce0
// Coefficients_13_26
static void __cdecl Coefficients_13_26(first, second, output)
const short* first;
const short* second;
short* output;
{
    long ltmp;
    for (int i = 1; i <= 8; ++i, ++first, ++second, ++output)
    {
        *output = (short)GSM_ADD(SASR(*first, 1), SASR(*second, 1));
    }
}

// FUNCTION: SWBF2 0x00751d60
// Coefficients_27_39
static void __cdecl Coefficients_27_39(first, second, output)
const short* first;
const short* second;
short* output;
{
    long ltmp;
    for (int i = 1; i <= 8; ++i, ++first, ++second, ++output)
    {
        *output = (short)GSM_ADD(SASR(*first, 2), SASR(*second, 2));
        *output = (short)GSM_ADD(*output, SASR(*second, 1));
    }
}

// FUNCTION: SWBF2 0x007524e0
// LARp_to_rp
static void __cdecl LARp_to_rp(ratios)
short* ratios;
{
    short temp;
    long ltmp;
    for (int i = 1; i <= 8; ++i, ++ratios)
    {
        if (*ratios < 0)
        {
            temp = *ratios == MIN_WORD ? MAX_WORD : -*ratios;
            *ratios = (short)-(temp < 11059 ? temp << 1
                : (temp < 20070 ? temp + 11059 : GSM_ADD(temp >> 2, 26112)));
        }
        else
        {
            temp = *ratios;
            *ratios = (short)(temp < 11059 ? temp << 1
                : (temp < 20070 ? temp + 11059 : GSM_ADD(temp >> 2, 26112)));
        }
    }
}

// FUNCTION: SWBF2 0x00751e30
static void __cdecl Decoding_of_the_coded_Log_Area_Ratios(const short* LARc, short* LARpp)
{
    word temp1;
    longword ltmp;

#define STEP(B, MIC, INVA) \
        temp1 = (word)(GSM_ADD(*LARc++, MIC) << 10); \
        temp1 = (word)GSM_SUB(temp1, B << 1); \
        temp1 = (word)GSM_MULT_R(INVA, temp1); \
        *LARpp++ = (word)GSM_ADD(temp1, temp1);

    STEP(0, -32, 13107);
    STEP(0, -32, 13107);
    STEP(2048, -16, 13107);
    STEP(-2560, -16, 13107);

    STEP(94, -8, 19223);
    STEP(-1792, -8, 17476);
    STEP(-341, -4, 31454);
    STEP(-1144, -4, 29708);

#undef STEP
}

static void Coefficients_40_159(current, output)
const short* current;
short* output;
{
    int i;
    for (i = 1; i <= 8; ++i, ++current, ++output) *output = *current;
}

// FUNCTION: SWBF2 0x007525d0
// Short_term_analysis_filtering
static void __cdecl Short_term_analysis_filtering(state, coefficients, count, samples)
struct gsm_state* state;
const short* coefficients;
int count;
short* samples;
{
    short* memory = state->u;
    int i;
    short di, zzz, ui, saved, coefficient;
    longword ltmp;
    for (; count--; ++samples)
    {
        di = saved = *samples;
        for (i = 0; i < 8; ++i)
        {
            ui = memory[i];
            coefficient = coefficients[i];
            memory[i] = saved;
            zzz = (word)GSM_MULT_R(coefficient, di);
            saved = (word)GSM_ADD(ui, zzz);
            zzz = (word)GSM_MULT_R(coefficient, ui);
            di = (word)GSM_ADD(di, zzz);
        }
        *samples = di;
    }
}

// FUNCTION: SWBF2 0x007526f0
// Short_term_synthesis_filtering
static void __cdecl Short_term_synthesis_filtering(state, ratios, count, input, output)
struct gsm_state* state;
const short* ratios;
int count;
const short* input;
short* output;
{
    short* memory = state->v;
    int i;
    short sample, temp1, temp2;
    longword ltmp;
    while (count--)
    {
        sample = *input++;
        for (i = 8; i--;)
        {
            temp1 = ratios[i];
            temp2 = memory[i];
            temp2 = (word)(temp1 == MIN_WORD && temp2 == MIN_WORD ? MAX_WORD
                : 0xffff & (((longword)temp1 * (longword)temp2 + 16384) >> 15));
            sample = (word)GSM_SUB(sample, temp2);
            temp1 = (word)(temp1 == MIN_WORD && sample == MIN_WORD ? MAX_WORD
                : 0xffff & (((longword)temp1 * (longword)sample + 16384) >> 15));
            memory[i + 1] = (word)GSM_ADD(memory[i], temp1);
        }
        *output++ = memory[0] = sample;
    }
}

// FUNCTION: SWBF2 0x007522c0
// Gsm_Short_Term_Analysis_Filter
void __cdecl Gsm_Short_Term_Analysis_Filter(state, coded, samples)
struct gsm_state* state;
const short* coded;
short* samples;
{
    short* current = state->LARpp[state->j];
    short* previous = state->LARpp[state->j ^= 1];
    short ratios[8];
    Decoding_of_the_coded_Log_Area_Ratios(coded, current);
    Coefficients_0_12(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_analysis_filtering(state, ratios, 13, samples);
    Coefficients_13_26(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_analysis_filtering(state, ratios, 14, samples + 13);
    Coefficients_27_39(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_analysis_filtering(state, ratios, 13, samples + 27);
    Coefficients_40_159(current, ratios);
    LARp_to_rp(ratios);
    Short_term_analysis_filtering(state, ratios, 120, samples + 40);
}

// FUNCTION: SWBF2 0x007523c0
// Gsm_Short_Term_Synthesis_Filter
void __cdecl Gsm_Short_Term_Synthesis_Filter(state, coded, input, output)
struct gsm_state* state;
const short* coded;
const short* input;
short* output;
{
    short* current = state->LARpp[state->j];
    short* previous = state->LARpp[state->j ^= 1];
    short ratios[8];
    Decoding_of_the_coded_Log_Area_Ratios(coded, current);
    Coefficients_0_12(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_synthesis_filtering(state, ratios, 13, input, output);
    Coefficients_13_26(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_synthesis_filtering(state, ratios, 14, input + 13, output + 13);
    Coefficients_27_39(previous, current, ratios);
    LARp_to_rp(ratios);
    Short_term_synthesis_filtering(state, ratios, 13, input + 27, output + 27);
    Coefficients_40_159(current, ratios);
    LARp_to_rp(ratios);
    Short_term_synthesis_filtering(state, ratios, 120, input + 40, output + 40);
}
