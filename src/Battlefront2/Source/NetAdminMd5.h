#pragma once

struct MD5_CTX
{
    unsigned int state[4];
    unsigned int count[2];
    unsigned char buffer[64];
};

static_assert(sizeof(MD5_CTX) == 88, "md5 context size");

void MD5Digest(const char* pcText, unsigned int uLength, char* pcHex);
