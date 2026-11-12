#ifndef GAMESPYFL_MD5_H
#define GAMESPYFL_MD5_H

typedef struct MD5_CTX
{
    unsigned int state[4];
    unsigned int count[2];
    unsigned char buffer[64];
} MD5_CTX;

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl MD5Init(MD5_CTX* pState);
void __cdecl MD5Update(MD5_CTX* pState, const unsigned char* pData, unsigned int uLength);
void __cdecl MD5Final(unsigned char* pDigest, MD5_CTX* pState);
void __cdecl MD5Print(const unsigned char* pDigest, char* pcHex);
void MD5Digest(const char* pcText, unsigned int uLength, char* pcHex);

#ifdef __cplusplus
}
#endif

#endif
