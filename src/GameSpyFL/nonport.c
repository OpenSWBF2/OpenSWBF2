#include <winsock2.h>
#include <string.h>

#include "nonport.h"

// FUNCTION: SWBF2 0x005d4ab0
struct hostent* __cdecl getlocalhost(void)
{
    char name[256] = "";
    gethostname(name, sizeof(name));
    return gethostbyname(name);
}
