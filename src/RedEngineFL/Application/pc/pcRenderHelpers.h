#ifndef SWBF2_GRAPHICS_PC_PCRENDERHELPERS_H
#define SWBF2_GRAPHICS_PC_PCRENDERHELPERS_H

#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblMatrix.h"





union RenderSortKey
{
    unsigned int id;
    struct
    {
        unsigned int depth : 13;
        unsigned int material : 16;
        unsigned int reserved : 3;
    } opaque;
    struct
    {
        unsigned int material : 16;
        unsigned int reserved : 2;
        unsigned int depth : 11;
        unsigned int pass : 2;
        unsigned int translucent : 1;
    } translucent;
};

extern PblMatrix gMatrixIdentity;

#endif
