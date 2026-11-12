#include <stdio.h>
#include <string.h>
#include "md5.h"


void __cdecl MD5Transform(unsigned int* pState, const unsigned char* pBlock);
void __cdecl MD5EncodeWords(unsigned char* pDest, const unsigned int* pSource, unsigned int uLength);
void __cdecl MD5DecodeWords(unsigned int* pWords, const unsigned char* pBlock, unsigned int uLength);
void* __cdecl MD5CopyBytes(void* pDest, const void* pSource, unsigned int uLength);
void __cdecl MD5ClearBytes(void* pDest, int iValue, unsigned int uLength);
void __cdecl MD5Update(MD5_CTX* pState, const unsigned char* pData, unsigned int uLength);
void __cdecl MD5Print(const unsigned char* pDigest, char* pcHex);

// GLOBAL: SWBF2 0x007e88d0
unsigned char md5Padding[64] = { 0x80 };

// FUNCTION: SWBF2 0x005a2730
void __cdecl MD5Init(MD5_CTX* pState)
{
    pState->count[0] = pState->count[1] = 0;
    pState->state[0] = 0x67452301;
    pState->state[1] = 0xefcdab89;
    pState->state[2] = 0x98badcfe;
    pState->state[3] = 0x10325476;
}

// FUNCTION: SWBF2 0x005a2760
void __cdecl MD5Update(MD5_CTX* pState, const unsigned char* pData, unsigned int uLength)
{
    unsigned int uIndex = (pState->count[0] >> 3) & 63;
    if ((pState->count[0] += (uLength << 3)) < (uLength << 3))
    {
        ++pState->count[1];
    }
    pState->count[1] += uLength >> 29;
    unsigned int uPart = 64 - uIndex;
    unsigned int i;
    if (uLength >= uPart)
    {
        MD5CopyBytes(pState->buffer + uIndex, pData, uPart);
        MD5Transform(pState->state, pState->buffer);
        for (i = uPart; i + 63 < uLength; i += 64)
        {
            MD5Transform(pState->state, pData + i);
        }
        uIndex = 0;
    }
    else
    {
        i = 0;
    }
    MD5CopyBytes(pState->buffer + uIndex, pData + i, uLength - i);
}

// FUNCTION: SWBF2 0x005a2810
void __cdecl MD5Final(unsigned char* pDigest, MD5_CTX* pState)
{
    unsigned char aucBits[8];
    MD5EncodeWords(aucBits, pState->count, 8);
    unsigned int uIndex = (pState->count[0] >> 3) & 63;
    MD5Update(pState, md5Padding, (uIndex < 56 ? 56 : 120) - uIndex);
    MD5Update(pState, aucBits, 8);
    MD5EncodeWords(pDigest, pState->state, 16);
    MD5ClearBytes(pState, 0, sizeof(*pState));
}

#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))
#define MD5_ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define MD5_STEP(f, a, b, c, d, x, s, ac) { (a) += f((b), (c), (d)) + (x) + (unsigned int)(ac); (a) = MD5_ROL((a), (s)); (a) += (b); }

// FUNCTION: SWBF2 0x005a2880
void __cdecl MD5Transform(unsigned int* pState, const unsigned char* pBlock)
{
    unsigned int a = pState[0], b = pState[1], c = pState[2], d = pState[3];
    unsigned int x[16];
    MD5DecodeWords(x, pBlock, 64);

    MD5_STEP(MD5_F, a, b, c, d, x[ 0],  7, 0xd76aa478); MD5_STEP(MD5_F, d, a, b, c, x[ 1], 12, 0xe8c7b756);
    MD5_STEP(MD5_F, c, d, a, b, x[ 2], 17, 0x242070db); MD5_STEP(MD5_F, b, c, d, a, x[ 3], 22, 0xc1bdceee);
    MD5_STEP(MD5_F, a, b, c, d, x[ 4],  7, 0xf57c0faf); MD5_STEP(MD5_F, d, a, b, c, x[ 5], 12, 0x4787c62a);
    MD5_STEP(MD5_F, c, d, a, b, x[ 6], 17, 0xa8304613); MD5_STEP(MD5_F, b, c, d, a, x[ 7], 22, 0xfd469501);
    MD5_STEP(MD5_F, a, b, c, d, x[ 8],  7, 0x698098d8); MD5_STEP(MD5_F, d, a, b, c, x[ 9], 12, 0x8b44f7af);
    MD5_STEP(MD5_F, c, d, a, b, x[10], 17, 0xffff5bb1); MD5_STEP(MD5_F, b, c, d, a, x[11], 22, 0x895cd7be);
    MD5_STEP(MD5_F, a, b, c, d, x[12],  7, 0x6b901122); MD5_STEP(MD5_F, d, a, b, c, x[13], 12, 0xfd987193);
    MD5_STEP(MD5_F, c, d, a, b, x[14], 17, 0xa679438e); MD5_STEP(MD5_F, b, c, d, a, x[15], 22, 0x49b40821);

    MD5_STEP(MD5_G, a, b, c, d, x[ 1],  5, 0xf61e2562); MD5_STEP(MD5_G, d, a, b, c, x[ 6],  9, 0xc040b340);
    MD5_STEP(MD5_G, c, d, a, b, x[11], 14, 0x265e5a51); MD5_STEP(MD5_G, b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    MD5_STEP(MD5_G, a, b, c, d, x[ 5],  5, 0xd62f105d); MD5_STEP(MD5_G, d, a, b, c, x[10],  9, 0x02441453);
    MD5_STEP(MD5_G, c, d, a, b, x[15], 14, 0xd8a1e681); MD5_STEP(MD5_G, b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    MD5_STEP(MD5_G, a, b, c, d, x[ 9],  5, 0x21e1cde6); MD5_STEP(MD5_G, d, a, b, c, x[14],  9, 0xc33707d6);
    MD5_STEP(MD5_G, c, d, a, b, x[ 3], 14, 0xf4d50d87); MD5_STEP(MD5_G, b, c, d, a, x[ 8], 20, 0x455a14ed);
    MD5_STEP(MD5_G, a, b, c, d, x[13],  5, 0xa9e3e905); MD5_STEP(MD5_G, d, a, b, c, x[ 2],  9, 0xfcefa3f8);
    MD5_STEP(MD5_G, c, d, a, b, x[ 7], 14, 0x676f02d9); MD5_STEP(MD5_G, b, c, d, a, x[12], 20, 0x8d2a4c8a);

    MD5_STEP(MD5_H, a, b, c, d, x[ 5],  4, 0xfffa3942); MD5_STEP(MD5_H, d, a, b, c, x[ 8], 11, 0x8771f681);
    MD5_STEP(MD5_H, c, d, a, b, x[11], 16, 0x6d9d6122); MD5_STEP(MD5_H, b, c, d, a, x[14], 23, 0xfde5380c);
    MD5_STEP(MD5_H, a, b, c, d, x[ 1],  4, 0xa4beea44); MD5_STEP(MD5_H, d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    MD5_STEP(MD5_H, c, d, a, b, x[ 7], 16, 0xf6bb4b60); MD5_STEP(MD5_H, b, c, d, a, x[10], 23, 0xbebfbc70);
    MD5_STEP(MD5_H, a, b, c, d, x[13],  4, 0x289b7ec6); MD5_STEP(MD5_H, d, a, b, c, x[ 0], 11, 0xeaa127fa);
    MD5_STEP(MD5_H, c, d, a, b, x[ 3], 16, 0xd4ef3085); MD5_STEP(MD5_H, b, c, d, a, x[ 6], 23, 0x04881d05);
    MD5_STEP(MD5_H, a, b, c, d, x[ 9],  4, 0xd9d4d039); MD5_STEP(MD5_H, d, a, b, c, x[12], 11, 0xe6db99e5);
    MD5_STEP(MD5_H, c, d, a, b, x[15], 16, 0x1fa27cf8); MD5_STEP(MD5_H, b, c, d, a, x[ 2], 23, 0xc4ac5665);

    MD5_STEP(MD5_I, a, b, c, d, x[ 0],  6, 0xf4292244); MD5_STEP(MD5_I, d, a, b, c, x[ 7], 10, 0x432aff97);
    MD5_STEP(MD5_I, c, d, a, b, x[14], 15, 0xab9423a7); MD5_STEP(MD5_I, b, c, d, a, x[ 5], 21, 0xfc93a039);
    MD5_STEP(MD5_I, a, b, c, d, x[12],  6, 0x655b59c3); MD5_STEP(MD5_I, d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    MD5_STEP(MD5_I, c, d, a, b, x[10], 15, 0xffeff47d); MD5_STEP(MD5_I, b, c, d, a, x[ 1], 21, 0x85845dd1);
    MD5_STEP(MD5_I, a, b, c, d, x[ 8],  6, 0x6fa87e4f); MD5_STEP(MD5_I, d, a, b, c, x[15], 10, 0xfe2ce6e0);
    MD5_STEP(MD5_I, c, d, a, b, x[ 6], 15, 0xa3014314); MD5_STEP(MD5_I, b, c, d, a, x[13], 21, 0x4e0811a1);
    MD5_STEP(MD5_I, a, b, c, d, x[ 4],  6, 0xf7537e82); MD5_STEP(MD5_I, d, a, b, c, x[11], 10, 0xbd3af235);
    MD5_STEP(MD5_I, c, d, a, b, x[ 2], 15, 0x2ad7d2bb); MD5_STEP(MD5_I, b, c, d, a, x[ 9], 21, 0xeb86d391);

    pState[0] += a;
    pState[1] += b;
    pState[2] += c;
    pState[3] += d;
}

// FUNCTION: SWBF2 0x005a2f40
void __cdecl MD5EncodeWords(unsigned char* pDest, const unsigned int* pSource, unsigned int uLength)
{
    for (unsigned int i = 0, j = 0; j < uLength; ++i, j += 4)
    {
        pDest[j] = (unsigned char)(pSource[i] & 0xff);
        pDest[j + 1] = (unsigned char)((pSource[i] >> 8) & 0xff);
        pDest[j + 2] = (unsigned char)((pSource[i] >> 16) & 0xff);
        pDest[j + 3] = (unsigned char)((pSource[i] >> 24) & 0xff);
    }
}

// FUNCTION: SWBF2 0x005a2f90
void __cdecl MD5DecodeWords(unsigned int* pWords, const unsigned char* pBlock, unsigned int uLength)
{
    for (unsigned int i = 0, j = 0; j < uLength; ++i, j += 4)
    {
        pWords[i] = (unsigned int)pBlock[j] |
                    ((unsigned int)pBlock[j + 1] << 8) |
                    ((unsigned int)pBlock[j + 2] << 16) |
                    ((unsigned int)pBlock[j + 3] << 24);
    }
}

// FUNCTION: SWBF2 0x005a2fd0
void* __cdecl MD5CopyBytes(void* pDest, const void* pSource, unsigned int uLength)
{
    return memcpy(pDest, pSource, uLength);
}

// FUNCTION: SWBF2 0x005a2ff0
void __cdecl MD5ClearBytes(void* pDest, int iValue, unsigned int uLength)
{
    memset(pDest, iValue, uLength);
}

// FUNCTION: SWBF2 0x005a3010
void __cdecl MD5Print(const unsigned char* pDigest, char* pcHex)
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        sprintf(pcHex, "%02x", pDigest[i]);
        pcHex += 2;
    }
}

// FUNCTION: SWBF2 0x005a3050
void MD5Digest(const char* pcText, unsigned int uLength, char* pcHex)
{
    MD5_CTX context;
    unsigned char aucDigest[16];
    MD5Init(&context);
    MD5Update(&context, (const unsigned char*)pcText, uLength);
    MD5Final(aucDigest, &context);
    MD5Print(aucDigest, pcHex);
}

