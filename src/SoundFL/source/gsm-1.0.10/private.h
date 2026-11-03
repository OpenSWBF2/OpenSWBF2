#pragma once

typedef short word;
typedef long longword;
typedef unsigned long ulongword;

#define MIN_WORD (-32767 - 1)
#define MAX_WORD 32767
#define SASR(x, by) ((x) >= 0 ? (x) >> (by) : (~(-((x) + 1) >> (by))))
#define GSM_MULT_R(a, b) (SASR(((longword)(a) * (longword)(b) + 16384), 15))
#define GSM_MULT(a, b) (SASR(((longword)(a) * (longword)(b)), 15))
#define GSM_ADD(a, b) \
    ((ulongword)((ltmp = (longword)(a) + (longword)(b)) - MIN_WORD) > \
        MAX_WORD - MIN_WORD ? (ltmp > 0 ? MAX_WORD : MIN_WORD) : ltmp)
#define GSM_SUB(a, b) \
    ((ltmp = (longword)(a) - (longword)(b)) >= MAX_WORD \
    ? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)
#define GSM_ABS(a) ((a) < 0 ? ((a) == MIN_WORD ? MAX_WORD : -(a)) : (a))
#define MIN_LONGWORD (-2147483647 - 1)
#define MAX_LONGWORD 2147483647
#define GSM_L_ADD(a, b) \
    ((a) < 0 ? ((b) >= 0 ? (a) + (b) \
        : (utmp = (ulongword)-((a) + 1) + (ulongword)-((b) + 1)) \
            >= MAX_LONGWORD ? MIN_LONGWORD : -(longword)utmp - 2) \
    : ((b) <= 0 ? (a) + (b) \
        : (utmp = (ulongword)(a) + (ulongword)(b)) >= MAX_LONGWORD \
            ? MAX_LONGWORD : utmp))
#define GSM_MAGIC 0xD

#define GSM_ASSERT(e, file, line) \
    (void)((!!(e)) || (_wassert(_CRT_WIDE(#e), file, line), 0))
#define GSM_ENCODE_C L"source\\gsm-1.0.10\\gsm_encode.c"
#define LONG_TERM_C L"source\\gsm-1.0.10\\long_term.c"


struct gsm_state
{
    short dp0[280];
    short z1;
    int L_z2;
    int mp;
    short u[8];
    short LARpp[2][8];
    short j;
    short ltp_cut;
    short nrp;
    short v[9];
    short msr;
    char verbose, fast, wav_fmt;
    unsigned char frame_index, frame_chain;
};

extern short __cdecl gsm_mult();
extern short __cdecl gsm_norm();
extern short gsm_DLB[4];
extern short gsm_QLB[4];
extern void __cdecl Gsm_Long_Term_Predictor();

extern void __cdecl Gsm_Short_Term_Analysis_Filter();
extern void __cdecl Gsm_Short_Term_Synthesis_Filter();

extern void __cdecl Gsm_RPE_Decoding();
extern void __cdecl Gsm_Long_Term_Synthesis_Filtering();
