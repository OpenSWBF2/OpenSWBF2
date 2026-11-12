#pragma once

#include <stddef.h>

void* PblCrtAlloc(size_t uSize);
char* PblCrtStrdup(const char* pcText);
void PblCrtFree(void* p);
void PblCrtFreeUncounted(void* p);
#ifdef OPENSWBF2_TEST
unsigned int PblCrtLiveBytes();
#endif
