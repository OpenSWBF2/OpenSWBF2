#include <string.h>

#include "PblJournal.h"

// GLOBAL: SWBF2 0x009cefd0
PblJournal sJournalInstance;

// FUNCTION: SWBF2 0x0072b340
PblJournal* PblJournal::Get(PblJournalType  )
{
    return &sJournalInstance;
}


// FUNCTION: SWBF2 0x0072b360
void JournalString(char* pcString, int iMax)
{
    int iLength = (int)strlen(pcString);
    if (iLength > iMax)
    {
        iLength = iMax;
    }
    pcString[iLength] = '\0';
}

// FUNCTION: SWBF2 0x0072b350
void __cdecl JournalBytes(char* bytes, int len)
{
}
