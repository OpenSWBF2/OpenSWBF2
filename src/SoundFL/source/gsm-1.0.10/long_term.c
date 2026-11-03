#include <assert.h>
#include "private.h"

// FUNCTION: SWBF2 0x007511b0
void __cdecl Calculation_of_the_LTP_parameters(const short* d, const short* dp, short* bc_out, short* Nc_out)
{
    int k, lambda;
    word Nc, bc;
    word wt[40];

    longword L_max, L_power;
    word R, S, dmax, scal;
    word temp;

    dmax = 0;
    for (k = 0; k <= 39; k++)
    {
        temp = d[k];
        temp = GSM_ABS(temp);
        if (temp > dmax)
        {
            dmax = temp;
        }
    }

    temp = 0;
    if (dmax == 0)
    {
        scal = 0;
    }
    else
    {
// STRING: SWBF2 0x787250
// L"dmax > 0"
        GSM_ASSERT(dmax > 0, LONG_TERM_C, 0xb8);
        temp = (word)gsm_norm((longword)dmax << 16);
    }

    if (temp > 6)
    {
        scal = 0;
    }
    else
    {
        scal = (word)(6 - temp);
    }

// STRING: SWBF2 0x787264
// L"scal >= 0"
    GSM_ASSERT(scal >= 0, LONG_TERM_C, 0xbf);

    for (k = 0; k <= 39; k++)
    {
        wt[k] = (word)SASR(d[k], scal);
    }

    L_max = 0;
    Nc = 40;

    for (lambda = 40; lambda <= 120; lambda++)
    {
#define STEP(k) (longword)wt[k] * dp[k - lambda]
        longword L_result;

        L_result  = STEP(0);  L_result += STEP(1);
        L_result += STEP(2);  L_result += STEP(3);
        L_result += STEP(4);  L_result += STEP(5);
        L_result += STEP(6);  L_result += STEP(7);
        L_result += STEP(8);  L_result += STEP(9);
        L_result += STEP(10); L_result += STEP(11);
        L_result += STEP(12); L_result += STEP(13);
        L_result += STEP(14); L_result += STEP(15);
        L_result += STEP(16); L_result += STEP(17);
        L_result += STEP(18); L_result += STEP(19);
        L_result += STEP(20); L_result += STEP(21);
        L_result += STEP(22); L_result += STEP(23);
        L_result += STEP(24); L_result += STEP(25);
        L_result += STEP(26); L_result += STEP(27);
        L_result += STEP(28); L_result += STEP(29);
        L_result += STEP(30); L_result += STEP(31);
        L_result += STEP(32); L_result += STEP(33);
        L_result += STEP(34); L_result += STEP(35);
        L_result += STEP(36); L_result += STEP(37);
        L_result += STEP(38); L_result += STEP(39);
#undef STEP

        if (L_result > L_max)
        {
            Nc = (word)lambda;
            L_max = L_result;
        }
    }

    *Nc_out = Nc;

    L_max <<= 1;

// STRING: SWBF2 0x787278
// L"scal <= 100 && scal >= -100"
    GSM_ASSERT(scal <= 100 && scal >= -100, LONG_TERM_C, 0xf4);
    L_max = L_max >> (6 - scal);

// STRING: SWBF2 0x7872b0
// L"Nc <= 120 && Nc >= 40"
    GSM_ASSERT(Nc <= 120 && Nc >= 40, LONG_TERM_C, 0xf7);

    L_power = 0;
    for (k = 0; k <= 39; k++)
    {
        longword L_temp;

        L_temp = SASR(dp[k - Nc], 3);
        L_power += L_temp * L_temp;
    }
    L_power <<= 1;

    if (L_max <= 0)
    {
        *bc_out = 0;
        return;
    }
    if (L_max >= L_power)
    {
        *bc_out = 3;
        return;
    }

    temp = (word)gsm_norm(L_power);

    R = (word)SASR(L_max << temp, 16);
    S = (word)SASR(L_power << temp, 16);

    for (bc = 0; bc <= 2; bc++)
    {
        if (R <= gsm_mult(S, gsm_DLB[bc]))
        {
            break;
        }
    }
    *bc_out = bc;
}

// FUNCTION: SWBF2 0x007519d0
// Long_term_analysis_filtering
static void __cdecl Long_term_analysis_filtering(gain, lag, history, input, predicted, residual)
short gain;
short lag;
const short* history;
const short* input;
short* predicted;
short* residual;
{
    int k;
    longword ltmp;
#define STEP(BP) \
    for (k = 0; k <= 39; ++k) { \
        predicted[k] = (word)GSM_MULT_R(BP, history[k - lag]); \
        residual[k] = (word)GSM_SUB(input[k], predicted[k]); \
    }
    switch (gain)
    {
    case 0: STEP(3277); break;
    case 1: STEP(11469); break;
    case 2: STEP(21299); break;
    case 3: STEP(32767); break;
    }
#undef STEP
}

// FUNCTION: SWBF2 0x007517c0
// Gsm_Long_Term_Predictor
void __cdecl Gsm_Long_Term_Predictor(state, input, history, residual, predicted, lag, gain)
struct gsm_state* state;
const short* input;
const short* history;
short* residual;
short* predicted;
short* lag;
short* gain;
{
    const wchar_t* file = 
// STRING: SWBF2 0x787214
L"source\\gsm-1.0.10\\long_term.c";
    if (!input)
    {
        _wassert(
// STRING: SWBF2 0x7872dc
L"d", file, 0x370);
    }
    if (!history)
    {
        _wassert(
// STRING: SWBF2 0x7872e0
L"dp", file, 0x370);
    }
    if (!residual)
    {
        _wassert(
// STRING: SWBF2 0x7872e8
L"e", file, 0x370);
    }
    if (!predicted)
    {
        _wassert(
// STRING: SWBF2 0x7872ec
L"dpp", file, 0x371);
    }
    if (!lag)
    {
        _wassert(
// STRING: SWBF2 0x7872f4
L"Nc", file, 0x371);
    }
    if (!gain)
    {
        _wassert(
// STRING: SWBF2 0x7872fc
L"bc", file, 0x371);
    }
    Calculation_of_the_LTP_parameters(input, history, gain, lag);
    Long_term_analysis_filtering(*gain, *lag, history, input, predicted, residual);
}
// FUNCTION: SWBF2 0x007518b0
// Gsm_Long_Term_Synthesis_Filtering
void __cdecl Gsm_Long_Term_Synthesis_Filtering(state, lag, gain, residual, history)
struct gsm_state* state;
short lag;
short gain;
const short* residual;
short* history;
{
    longword ltmp;
    int k;
    short factor, predicted, effectiveLag;
    effectiveLag = lag < 40 || lag > 120 ? state->nrp : lag;
    state->nrp = effectiveLag;
    if (!(effectiveLag >= 40 && effectiveLag <= 120))
        _wassert(
// STRING: SWBF2 0x787304
L"Nr >= 40 && Nr <= 120", LONG_TERM_C, 0x3a0);
    factor = gsm_QLB[gain];
    if (factor == MIN_WORD)
        _wassert(
// STRING: SWBF2 0x787330
L"brp != MIN_WORD", LONG_TERM_C, 0x3a9);
    for (k = 0; k <= 39; ++k)
    {
        predicted = (word)GSM_MULT_R(factor, history[k - effectiveLag]);
        history[k] = (word)GSM_ADD(residual[k], predicted);
    }
    for (k = 0; k <= 119; ++k) history[-120 + k] = history[-80 + k];
}
