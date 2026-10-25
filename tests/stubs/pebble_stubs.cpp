#include <windows.h>
#include "PebbleFL/Common/PblFile.h"

bool __cdecl PblFile::Exists(const char* pcFileName)
{
    return GetFileAttributesA(pcFileName) != INVALID_FILE_ATTRIBUTES;
}
