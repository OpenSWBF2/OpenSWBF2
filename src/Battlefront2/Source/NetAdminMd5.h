#pragma once

struct MD5Context
{
    unsigned int state[4];
    unsigned int count[2];
    unsigned char buffer[64];
};

static_assert(sizeof(MD5Context) == 88, "md5 context size");

void MD5Digest(const char* pcText, unsigned int uLength, char* pcHex);
