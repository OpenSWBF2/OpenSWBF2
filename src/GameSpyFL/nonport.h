#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct hostent;
struct hostent* __cdecl getlocalhost(void);

#ifdef __cplusplus
}
#endif
