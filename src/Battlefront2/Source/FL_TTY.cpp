#include "FL_TTY.h"
#include "RedEngineFL/Memory/RedMemory.h"

struct _ttychar
{
    char Char;
    unsigned int Color;
};

// GLOBAL: SWBF2 0x01f9c2a0
static _ttychar* ttymap;

// GLOBAL: SWBF2 0x01f9c2a4
static unsigned int textcolor;

// GLOBAL: SWBF2 0x01f9c2a8
static unsigned int ttywidth;

// GLOBAL: SWBF2 0x01f9c2ac
static unsigned int ttyheight;

// FUNCTION: SWBF2 0x0052bfe0
void TTYInit(unsigned int aWidth, unsigned int aHeight)
{
    ttywidth = aWidth;
    ttyheight = aHeight;
    textcolor = -1;
    ttymap = (_ttychar*)RedAllocFromHeap(__RedCurrHeap, ttywidth * ttyheight * sizeof(_ttychar), 0);

    for (unsigned int i = 0; i < ttyheight; ++i)
    {
        ttymap[i * ttywidth].Char = '\n';
    }
}
