#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef struct gsm_state gsm_state;

#include "private.h"

// FUNCTION: SWBF2 0x007500b0
// gsm_asr
short __cdecl gsm_asr(value, shift)
short value;
int shift;
{
    if (shift >= 16)
    {
        return -(value < 0);
    }
    if (shift <= -16)
    {
        return 0;
    }
    if (shift < 0)
    {
        return value << -shift;
    }
    return value >> shift;
}

// FUNCTION: SWBF2 0x00750540
// gsm_mult
short __cdecl gsm_mult(a, b)
short a;
short b;
{
    if (a == (short)0x8000 && b == (short)0x8000)
    {
        return 0x7fff;
    }
    return (short)SASR((long)a * (long)b, 15);
}

// FUNCTION: SWBF2 0x00750590
// gsm_norm
short __cdecl gsm_norm(value)
int value;
{
    // GLOBAL: SWBF2 0x00786d98
    static const unsigned char bitoff[256] = {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    if (!value)
    {
        _wassert(
// STRING: SWBF2 0x7871b0
L"a != 0", 
// STRING: SWBF2 0x786e98
L"source\\gsm-1.0.10\\gsm_encode.c", 0x66c);
    }
    if (value < 0)
    {
        if (value <= -1073741824)
        {
            return 0;
        }
        value = ~value;
    }
    return value & 0xffff0000
        ? (value & 0xff000000
            ? -1 + bitoff[0xff & (value >> 24)]
            : 7 + bitoff[0xff & (value >> 16)])
        : (value & 0xff00
            ? 15 + bitoff[0xff & (value >> 8)]
            : 23 + bitoff[0xff & value]);
}

// FUNCTION: SWBF2 0x00750000
// Weighting_filter
void __cdecl Weighting_filter(input, output)
const short* input;
short* output;
{
    longword result;
    int k;
    input -= 5;
    for (k = 0; k <= 39; ++k)
    {
#define STEP(i, H) (input[k + i] * (longword)(H))
        result = 8192 >> 1;
        result += STEP(0, -134) + STEP(1, -374) + STEP(3, 2054)
            + STEP(4, 5741) + STEP(5, 8192) + STEP(6, 5741)
            + STEP(7, 2054) + STEP(9, -374) + STEP(10, -134);
#undef STEP
        result = SASR(result, 13);
        output[k] = (word)(result < MIN_WORD ? MIN_WORD
            : (result > MAX_WORD ? MAX_WORD : result));
    }
}

// FUNCTION: SWBF2 0x0074f490
// RPE_grid_positioning
void __cdecl RPE_grid_positioning(grid, input, output)
short grid;
const short* input;
short* output;
{
    int i = 13;
// STRING: SWBF2 0x787130
// L"0 <= Mc && Mc <= 3"
    if (!(0 <= grid && grid <= 3))
        _wassert(L"0 <= Mc && Mc <= 3", GSM_ENCODE_C, 0x517);
    switch (grid)
    {
    case 3: *output++ = 0;
    case 2: do {
                *output++ = 0;
    case 1:     *output++ = 0;
    case 0:     *output++ = *input++;
            } while (--i);
    }
    while (++grid < 4) *output++ = 0;
}



short __cdecl gsm_add(a, b)
short a;
short b;
{
    const int sum = (int)a + b;
    return sum < MIN_WORD ? MIN_WORD : sum > MAX_WORD ? MAX_WORD : sum;
}

short __cdecl gsm_sub(a, b)
short a;
short b;
{
    const int difference = (int)a - b;
    return difference < MIN_WORD ? MIN_WORD : difference > MAX_WORD ? MAX_WORD : difference;
}

short __cdecl gsm_asl(value, shift)
short value;
int shift;
{
    if (shift >= 16) return 0;
    if (shift <= -16) return -(value < 0);
    if (shift < 0) return gsm_asr(value, -shift);
    return value << shift;
}

// FUNCTION: SWBF2 0x0074e510
// APCM_quantization_xmaxc_to_exp_mant
void __cdecl APCM_quantization_xmaxc_to_exp_mant(value, exponent, mantissa)
short value;
short* exponent;
short* mantissa;
{
    short exp = 0;
    short mant;
    if (value > 15) exp = (short)(SASR(value, 3) - 1);
    mant = (short)(value - (exp << 3));
    if (mant == 0)
    {
        exp = -4;
        mant = 7;
    }
    else
    {
        while (mant <= 7)
        {
            mant = (short)(mant << 1 | 1);
            --exp;
        }
        mant -= 8;
    }
// STRING: SWBF2 0x786f98
// L"exp >= -4 && exp <= 6"
    GSM_ASSERT(exp >= -4 && exp <= 6, GSM_ENCODE_C, 0x47e);
// STRING: SWBF2 0x786fc4
// L"mant >= 0 && mant <= 7"
    GSM_ASSERT(mant >= 0 && mant <= 7, GSM_ENCODE_C, 0x47f);
    *exponent = exp;
    *mantissa = mant;
}







// FUNCTION: SWBF2 0x0074fee0
// Transformation_to_Log_Area_Ratios
void __cdecl Transformation_to_Log_Area_Ratios(r)
word* r;
{
    word temp;
    int i;
    for (i = 1; i <= 8; ++i, ++r)
    {
        temp = *r;
        temp = GSM_ABS(temp);
// STRING: SWBF2 0x786f68
// L"temp >= 0"
        GSM_ASSERT(temp >= 0, GSM_ENCODE_C, 0x337);
        if (temp < 22118)
        {
            temp >>= 1;
        }
        else if (temp < 31130)
        {
// STRING: SWBF2 0x786f7c
// L"temp >= 11059"
            GSM_ASSERT(temp >= 11059, GSM_ENCODE_C, 0x33c);
            temp -= 11059;
        }
        else
        {
            temp -= 26112;
            temp <<= 2;
        }
        *r = *r < 0 ? -temp : temp;
// STRING: SWBF2 0x786f48
// L"*r != MIN_WORD"
        GSM_ASSERT(*r != MIN_WORD, GSM_ENCODE_C, 0x345);
    }
}



// GLOBAL: SWBF2 0x007e3ca8
short gsm_FAC[8] = { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767 };
// GLOBAL: SWBF2 0x007e3c98
short gsm_NRFAC[8] = { 29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384 };

// FUNCTION: SWBF2 0x0074e070
// APCM_inverse_quantization
void __cdecl APCM_inverse_quantization(coded, mantissa, exponent, output)
const short* coded;
short mantissa;
short exponent;
short* output;
{
    int i;
    short temp, temp1, temp2, temp3;
    longword ltmp;
    if (!(mantissa >= 0 && mantissa <= 7))
        _wassert(L"mant >= 0 && mant <= 7", GSM_ENCODE_C, 0x4f1);
    temp1 = gsm_FAC[mantissa];
    temp2 = gsm_sub(6, exponent);
    temp3 = gsm_asl(1, gsm_sub(temp2, 1));
    for (i = 13; i--;)
    {
// STRING: SWBF2 0x7870d0
// L"*xMc <= 7 && *xMc >= 0"
        if (!(*coded <= 7 && *coded >= 0))
            _wassert(L"*xMc <= 7 && *xMc >= 0", GSM_ENCODE_C, 0x4f9);
        temp = (short)((*coded++ << 1) - 7);
// STRING: SWBF2 0x787100
// L"temp <= 7 && temp >= -7"
        GSM_ASSERT(temp <= 7 && temp >= -7, GSM_ENCODE_C, 0x4fd);
        temp <<= 12;
        temp = (short)GSM_MULT_R(temp1, temp);
        temp = (short)GSM_ADD(temp, temp3);
        *output++ = gsm_asr(temp, temp2);
    }
}

// FUNCTION: SWBF2 0x0074e240
// APCM_quantization
void __cdecl APCM_quantization(input, coded, mantissa, exponent, xmaxc)
const short* input;
short* coded;
short* mantissa;
short* exponent;
short* xmaxc;
{
    int i, itest;
    short maximum, encodedMaximum, temp, temp1, temp2, exp, mant;
    maximum = 0;
    for (i = 0; i <= 12; ++i)
    {
        temp = input[i];
        temp = GSM_ABS(temp);
        if (temp > maximum) maximum = temp;
    }
    exp = 0;
    temp = (short)SASR(maximum, 9);
    itest = 0;
    for (i = 0; i <= 5; ++i)
    {
        itest |= temp <= 0;
        temp = (short)SASR(temp, 1);
// STRING: SWBF2 0x786ff4
// L"exp <= 5"
        GSM_ASSERT(exp <= 5, GSM_ENCODE_C, 0x4aa);
        if (itest == 0) ++exp;
    }
// STRING: SWBF2 0x787008
// L"exp <= 6 && exp >= 0"
    GSM_ASSERT(exp <= 6 && exp >= 0, GSM_ENCODE_C, 0x4ae);
    temp = (short)(exp + 5);
// STRING: SWBF2 0x787034
// L"temp <= 11 && temp >= 0"
    GSM_ASSERT(temp <= 11 && temp >= 0, GSM_ENCODE_C, 0x4b1);
    encodedMaximum = gsm_add((short)SASR(maximum, temp), (short)(exp << 3));
    APCM_quantization_xmaxc_to_exp_mant(encodedMaximum, &exp, &mant);
// STRING: SWBF2 0x787064
// L"exp <= 4096 && exp >= -4096"
    GSM_ASSERT(exp <= 4096 && exp >= -4096, GSM_ENCODE_C, 0x4c8);
    GSM_ASSERT(mant >= 0 && mant <= 7, GSM_ENCODE_C, 0x4c9);
    temp1 = (short)(6 - exp);
    temp2 = gsm_NRFAC[mant];
    for (i = 0; i <= 12; ++i)
    {
// STRING: SWBF2 0x78709c
// L"temp1 >= 0 && temp1 < 16"
        GSM_ASSERT(temp1 >= 0 && temp1 < 16, GSM_ENCODE_C, 0x4d0);
        temp = (short)(input[i] << temp1);
        temp = (short)GSM_MULT(temp, temp2);
        temp = (short)SASR(temp, 12);
        coded[i] = (short)(temp + 4);
    }
    *mantissa = mant;
    *exponent = exp;
    *xmaxc = encodedMaximum;
}






 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x0074ec60
// Gsm_Preprocess
void __cdecl Gsm_Preprocess(state, input, output)
struct gsm_state* state;
const short* input;
short* output;
{
    word z1 = state->z1;
    longword L_z2 = state->L_z2;
    word mp = (word)state->mp;
    word s1, msp, lsp, SO;
    longword L_s2, L_temp, ltmp;
    ulongword utmp;
    int k = 160;
    while (k--)
    {
        SO = (word)(SASR(*input, 3) << 2);
        ++input;
// STRING: SWBF2 0x787158
// L"SO >= -0x4000"
        GSM_ASSERT(SO >= -0x4000, GSM_ENCODE_C, 0x5b1);
// STRING: SWBF2 0x787174
// L"SO <= 0x3FFC"
        GSM_ASSERT(SO <= 0x3FFC, GSM_ENCODE_C, 0x5b2);
        s1 = SO - z1;
        z1 = SO;
// STRING: SWBF2 0x787190
// L"s1 != MIN_WORD"
        GSM_ASSERT(s1 != MIN_WORD, GSM_ENCODE_C, 0x5c2);
        L_s2 = s1;
        L_s2 <<= 15;
        msp = (word)SASR(L_z2, 15);
        lsp = (word)(L_z2 - ((longword)msp << 15));
        L_s2 += GSM_MULT_R(lsp, 32735);
        L_temp = (longword)msp * 32735;
        L_z2 = GSM_L_ADD(L_temp, L_s2);
        L_temp = GSM_L_ADD(L_z2, 16384);
        msp = (word)GSM_MULT_R(mp, -28180);
        mp = (word)SASR(L_temp, 15);
        *output++ = (word)GSM_ADD(mp, msp);
    }
    state->z1 = z1;
    state->L_z2 = L_z2;
    state->mp = mp;
}













extern void __cdecl Autocorrelation(short*, int*);
extern void __cdecl Reflection_coefficients(const int*, short*);
extern void __cdecl Quantization_and_coding(short*);
extern void __cdecl RPE_grid_selection(const short*, short*, short*);

void __cdecl Gsm_LPC_Analysis(state, samples, codedRatios)
struct gsm_state* state;
short* samples;
short* codedRatios;
{
    int correlations[9];
    Autocorrelation(samples, correlations);
    Reflection_coefficients(correlations, codedRatios);
    Transformation_to_Log_Area_Ratios(codedRatios);
    Quantization_and_coding(codedRatios);
}

void __cdecl Gsm_RPE_Encoding(state, residual, maximum, grid, codedSamples)
struct gsm_state* state;
short* residual;
short* maximum;
short* grid;
short* codedSamples;
{
    short weighted[40], selected[13], reconstructed[13];
    short mantissa, exponent;
    Weighting_filter(residual, weighted);
    RPE_grid_selection(weighted, selected, grid);
    APCM_quantization(selected, codedSamples, &mantissa, &exponent, maximum);
    APCM_inverse_quantization(codedSamples, mantissa, exponent, reconstructed);
    RPE_grid_positioning(*grid, reconstructed, residual);
}

// FUNCTION: SWBF2 0x0074ea10
// Gsm_Coder
void __cdecl Gsm_Coder(state, input, codedRatios, lag, gain, grid, maximum, codedSamples)
struct gsm_state* state;
const short* input;
short* codedRatios;
short* lag;
short* gain;
short* grid;
short* maximum;
short* codedSamples;
{
    int k;
    short* history = state->dp0 + 120;
    short* prediction = history;
    short samples[160];
    // GLOBAL: SWBF2 0x01e2f838
    static short e[50];
    Gsm_Preprocess(state, input, samples);
    Gsm_LPC_Analysis(state, samples, codedRatios);
    Gsm_Short_Term_Analysis_Filter(state, codedRatios, samples);
    for (k = 0; k <= 3; ++k, codedSamples += 13)
    {
        Gsm_Long_Term_Predictor(state, samples + k * 40, history,
            e + 5, prediction, lag++, gain++);
        Gsm_RPE_Encoding(state, e + 5, maximum++, grid++, codedSamples);
        {
            int i;
            longword ltmp;
            for (i = 0; i <= 39; ++i)
                history[i] = (word)GSM_ADD(e[5 + i], prediction[i]);
        }
        history += 40;
        prediction += 40;
    }
    memcpy(state->dp0, state->dp0 + 160, 120 * sizeof(*state->dp0));
}




// GLOBAL: SWBF2 0x007e3c78
word gsm_QLB[4] = { 3277, 11469, 21299, 32767 };

// GLOBAL: SWBF2 0x007e3c70
word gsm_DLB[4] = { 6554, 16384, 26214, 32767 };

static word gsm_div(num, denum)
word num;
word denum;
{
    longword L_num = num;
    longword L_denum = denum;
    word div = 0;
    int k = 15;

// STRING: SWBF2 0x7871c0
// L"num >= 0 && denum >= num"
    GSM_ASSERT(num >= 0 && denum >= num, GSM_ENCODE_C, 0x6bf);
    if (num == 0)
    {
        return 0;
    }

    while (k--)
    {
        div <<= 1;
        L_num <<= 1;
        if (L_num >= L_denum)
        {
            L_num -= L_denum;
            div++;
        }
    }
    return div;
}

// FUNCTION: SWBF2 0x0074e5d0
void __cdecl Autocorrelation(short* s, int* L_ACF)
{
    int k, i;
    word temp, smax, scalauto;

    smax = 0;
    for (k = 0; k <= 159; k++)
    {
        temp = GSM_ABS(s[k]);
        if (temp > smax)
        {
            smax = temp;
        }
    }

    if (smax == 0)
    {
        scalauto = 0;
    }
    else
    {
// STRING: SWBF2 0x786ed8
// L"smax > 0"
        GSM_ASSERT(smax > 0, GSM_ENCODE_C, 0x265);
        scalauto = (word)(4 - gsm_norm((longword)smax << 16));
    }

    if (scalauto > 0)
    {
#define SCALE(n) \
        case n: for (k = 0; k <= 159; k++) \
                    s[k] = (word)GSM_MULT_R(s[k], 16384 >> (n - 1)); \
            break;

        switch (scalauto)
        {
        SCALE(1)
        SCALE(2)
        SCALE(3)
        SCALE(4)
        }
#undef SCALE
    }

    {
        word* sp = s;
        word sl = *sp;

#define STEP(k) L_ACF[k] += ((longword)sl * sp[-(k)]);
#define NEXTI sl = *++sp

        for (k = 9; k--; L_ACF[k] = 0);

        STEP(0);
        NEXTI;
        STEP(0); STEP(1);
        NEXTI;
        STEP(0); STEP(1); STEP(2);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6); STEP(7);

        for (i = 8; i <= 159; i++)
        {
            NEXTI;

            STEP(0);
            STEP(1); STEP(2); STEP(3); STEP(4);
            STEP(5); STEP(6); STEP(7); STEP(8);
        }

        for (k = 9; k--; L_ACF[k] <<= 1);
#undef STEP
#undef NEXTI
    }

    if (scalauto > 0)
    {
// STRING: SWBF2 0x786eec
// L"scalauto <= 4"
        GSM_ASSERT(scalauto <= 4, GSM_ENCODE_C, 0x2ba);
        for (k = 160; k--; *s++ <<= scalauto);
    }
}

// FUNCTION: SWBF2 0x0074fac0
void __cdecl Reflection_coefficients(const int* L_ACF, short* r)
{
    int i, m, n;
    word temp;
    longword ltmp;
    word ACF[9];
    word P[9];
    word K[9];

    if (L_ACF[0] == 0)
    {
        for (i = 8; i--; *r++ = 0);
        return;
    }

    GSM_ASSERT(L_ACF[0] != 0, GSM_ENCODE_C, 0x2f1);
    temp = (word)gsm_norm(L_ACF[0]);

// STRING: SWBF2 0x786f08
// L"temp >= 0 && temp < 32"
    GSM_ASSERT(temp >= 0 && temp < 32, GSM_ENCODE_C, 0x2f4);

    for (i = 0; i <= 8; i++)
    {
        ACF[i] = (word)SASR(L_ACF[i] << temp, 16);
    }

    for (i = 1; i <= 7; i++)
    {
        K[i] = ACF[i];
    }
    for (i = 0; i <= 8; i++)
    {
        P[i] = ACF[i];
    }

    for (n = 1; n <= 8; n++, r++)
    {
        temp = P[1];
        temp = GSM_ABS(temp);
        if (P[0] < temp)
        {
            for (i = n; i <= 8; i++)
            {
                *r++ = 0;
            }
            return;
        }

        *r = gsm_div(temp, P[0]);

// STRING: SWBF2 0x786f38
// L"*r >= 0"
        GSM_ASSERT(*r >= 0, GSM_ENCODE_C, 0x30c);
        if (P[1] > 0)
        {
            *r = -*r;
        }
        GSM_ASSERT(*r != MIN_WORD, GSM_ENCODE_C, 0x30e);
        if (n == 8)
        {
            return;
        }

        temp = (word)GSM_MULT_R(P[1], *r);
        P[0] = (word)GSM_ADD(P[0], temp);

        for (m = 1; m <= 8 - n; m++)
        {
            temp = (word)GSM_MULT_R(K[m], *r);
            P[m] = (word)GSM_ADD(P[m + 1], temp);

            temp = (word)GSM_MULT_R(P[m + 1], *r);
            K[m] = (word)GSM_ADD(K[m], temp);
        }
    }
}

// FUNCTION: SWBF2 0x0074ef00
void __cdecl Quantization_and_coding(short* LAR)
{
    word temp;
    longword ltmp;

#define STEP(A, B, MAC, MIC) \
        temp = (word)GSM_MULT(A, *LAR); \
        temp = (word)GSM_ADD(temp, B); \
        temp = (word)GSM_ADD(temp, 256); \
        temp = (word)SASR(temp, 9); \
        *LAR = temp > MAC ? MAC - MIC : (temp < MIC ? 0 : temp - MIC); \
        LAR++;

    STEP(20480, 0, 31, -32);
    STEP(20480, 0, 31, -32);
    STEP(20480, 2048, 15, -16);
    STEP(20480, -2560, 15, -16);

    STEP(13964, 94, 7, -8);
    STEP(15360, -1792, 7, -8);
    STEP(8534, -341, 3, -4);
    STEP(9036, -1144, 3, -4);

#undef STEP
}

// FUNCTION: SWBF2 0x0074f550
void __cdecl RPE_grid_selection(const short* x, short* xM, short* Mc_out)
{
    int i;
    longword L_result, L_temp;
    longword EM;
    word Mc;
    longword L_common_0_3;

    EM = 0;
    Mc = 0;

#define STEP(m, i) L_temp = SASR(x[m + 3 * i], 2); \
                   L_result += L_temp * L_temp;

    L_result = 0;
    STEP(0, 1); STEP(0, 2); STEP(0, 3); STEP(0, 4);
    STEP(0, 5); STEP(0, 6); STEP(0, 7); STEP(0, 8);
    STEP(0, 9); STEP(0, 10); STEP(0, 11); STEP(0, 12);
    L_common_0_3 = L_result;

    STEP(0, 0);
    L_result <<= 1;
    EM = L_result;

    L_result = 0;
    STEP(1, 0);
    STEP(1, 1); STEP(1, 2); STEP(1, 3); STEP(1, 4);
    STEP(1, 5); STEP(1, 6); STEP(1, 7); STEP(1, 8);
    STEP(1, 9); STEP(1, 10); STEP(1, 11); STEP(1, 12);
    L_result <<= 1;
    if (L_result > EM)
    {
        Mc = 1;
        EM = L_result;
    }

    L_result = 0;
    STEP(2, 0);
    STEP(2, 1); STEP(2, 2); STEP(2, 3); STEP(2, 4);
    STEP(2, 5); STEP(2, 6); STEP(2, 7); STEP(2, 8);
    STEP(2, 9); STEP(2, 10); STEP(2, 11); STEP(2, 12);
    L_result <<= 1;
    if (L_result > EM)
    {
        Mc = 2;
        EM = L_result;
    }

    L_result = L_common_0_3;
    STEP(3, 12);
    L_result <<= 1;
    if (L_result > EM)
    {
        Mc = 3;
        EM = L_result;
    }
#undef STEP

    for (i = 0; i <= 12; i++)
    {
        xM[i] = x[Mc + 3 * i];
    }
    *Mc_out = Mc;
}





// FUNCTION: SWBF2 0x007500f0
void __cdecl gsm_encode(char* pGsm, short* pFrame, unsigned char* pOut)
{
    word LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13 * 4];
    unsigned char* c = pOut;

    Gsm_Coder(pGsm, pFrame, LARc, Nc, bc, Mc, xmaxc, xmc);

    *c++ =   ((GSM_MAGIC & 0xF) << 4)
           | ((LARc[0] >> 2) & 0xF);
    *c++ =   ((LARc[0] & 0x3) << 6)
           | (LARc[1] & 0x3F);
    *c++ =   ((LARc[2] & 0x1F) << 3)
           | ((LARc[3] >> 2) & 0x7);
    *c++ =   ((LARc[3] & 0x3) << 6)
           | ((LARc[4] & 0xF) << 2)
           | ((LARc[5] >> 2) & 0x3);
    *c++ =   ((LARc[5] & 0x3) << 6)
           | ((LARc[6] & 0x7) << 3)
           | (LARc[7] & 0x7);
    *c++ =   ((Nc[0] & 0x7F) << 1)
           | ((bc[0] >> 1) & 0x1);
    *c++ =   ((bc[0] & 0x1) << 7)
           | ((Mc[0] & 0x3) << 5)
           | ((xmaxc[0] >> 1) & 0x1F);
    *c++ =   ((xmaxc[0] & 0x1) << 7)
           | ((xmc[0] & 0x7) << 4)
           | ((xmc[1] & 0x7) << 1)
           | ((xmc[2] >> 2) & 0x1);
    *c++ =   ((xmc[2] & 0x3) << 6)
           | ((xmc[3] & 0x7) << 3)
           | (xmc[4] & 0x7);
    *c++ =   ((xmc[5] & 0x7) << 5)
           | ((xmc[6] & 0x7) << 2)
           | ((xmc[7] >> 1) & 0x3);
    *c++ =   ((xmc[7] & 0x1) << 7)
           | ((xmc[8] & 0x7) << 4)
           | ((xmc[9] & 0x7) << 1)
           | ((xmc[10] >> 2) & 0x1);
    *c++ =   ((xmc[10] & 0x3) << 6)
           | ((xmc[11] & 0x7) << 3)
           | (xmc[12] & 0x7);
    *c++ =   ((Nc[1] & 0x7F) << 1)
           | ((bc[1] >> 1) & 0x1);
    *c++ =   ((bc[1] & 0x1) << 7)
           | ((Mc[1] & 0x3) << 5)
           | ((xmaxc[1] >> 1) & 0x1F);
    *c++ =   ((xmaxc[1] & 0x1) << 7)
           | ((xmc[13] & 0x7) << 4)
           | ((xmc[14] & 0x7) << 1)
           | ((xmc[15] >> 2) & 0x1);
    *c++ =   ((xmc[15] & 0x3) << 6)
           | ((xmc[16] & 0x7) << 3)
           | (xmc[17] & 0x7);
    *c++ =   ((xmc[18] & 0x7) << 5)
           | ((xmc[19] & 0x7) << 2)
           | ((xmc[20] >> 1) & 0x3);
    *c++ =   ((xmc[20] & 0x1) << 7)
           | ((xmc[21] & 0x7) << 4)
           | ((xmc[22] & 0x7) << 1)
           | ((xmc[23] >> 2) & 0x1);
    *c++ =   ((xmc[23] & 0x3) << 6)
           | ((xmc[24] & 0x7) << 3)
           | (xmc[25] & 0x7);
    *c++ =   ((Nc[2] & 0x7F) << 1)
           | ((bc[2] >> 1) & 0x1);
    *c++ =   ((bc[2] & 0x1) << 7)
           | ((Mc[2] & 0x3) << 5)
           | ((xmaxc[2] >> 1) & 0x1F);
    *c++ =   ((xmaxc[2] & 0x1) << 7)
           | ((xmc[26] & 0x7) << 4)
           | ((xmc[27] & 0x7) << 1)
           | ((xmc[28] >> 2) & 0x1);
    *c++ =   ((xmc[28] & 0x3) << 6)
           | ((xmc[29] & 0x7) << 3)
           | (xmc[30] & 0x7);
    *c++ =   ((xmc[31] & 0x7) << 5)
           | ((xmc[32] & 0x7) << 2)
           | ((xmc[33] >> 1) & 0x3);
    *c++ =   ((xmc[33] & 0x1) << 7)
           | ((xmc[34] & 0x7) << 4)
           | ((xmc[35] & 0x7) << 1)
           | ((xmc[36] >> 2) & 0x1);
    *c++ =   ((xmc[36] & 0x3) << 6)
           | ((xmc[37] & 0x7) << 3)
           | (xmc[38] & 0x7);
    *c++ =   ((Nc[3] & 0x7F) << 1)
           | ((bc[3] >> 1) & 0x1);
    *c++ =   ((bc[3] & 0x1) << 7)
           | ((Mc[3] & 0x3) << 5)
           | ((xmaxc[3] >> 1) & 0x1F);
    *c++ =   ((xmaxc[3] & 0x1) << 7)
           | ((xmc[39] & 0x7) << 4)
           | ((xmc[40] & 0x7) << 1)
           | ((xmc[41] >> 2) & 0x1);
    *c++ =   ((xmc[41] & 0x3) << 6)
           | ((xmc[42] & 0x7) << 3)
           | (xmc[43] & 0x7);
    *c++ =   ((xmc[44] & 0x7) << 5)
           | ((xmc[45] & 0x7) << 2)
           | ((xmc[46] >> 1) & 0x3);
    *c++ =   ((xmc[46] & 0x1) << 7)
           | ((xmc[47] & 0x7) << 4)
           | ((xmc[48] & 0x7) << 1)
           | ((xmc[49] >> 2) & 0x1);
    *c++ =   ((xmc[49] & 0x3) << 6)
           | ((xmc[50] & 0x7) << 3)
           | (xmc[51] & 0x7);
}

// FUNCTION: SWBF2 0x0074eea0
// Gsm_RPE_Decoding
void __cdecl Gsm_RPE_Decoding(state, maximum, grid, coded, output)
struct gsm_state* state;
short maximum;
short grid;
const short* coded;
short* output;
{
    short exponent, mantissa, reconstructed[13];
    APCM_quantization_xmaxc_to_exp_mant(maximum, &exponent, &mantissa);
    APCM_inverse_quantization(coded, mantissa, exponent, reconstructed);
    RPE_grid_positioning(grid, reconstructed, output);
}

