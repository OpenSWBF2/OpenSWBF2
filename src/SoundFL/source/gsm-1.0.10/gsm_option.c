#include "private.h"

// FUNCTION: SWBF2 0x0074e040
// gsm_option
int __cdecl gsm_option(pGsm, iOption, pValue)
struct gsm_state* pGsm;
int iOption;
int* pValue;
{
    int result = -1;
    switch (iOption)
    {
    case 1:
        result = pGsm->verbose;
        if (pValue != 0)
        {
            pGsm->verbose = (char)*pValue;
        }
        break;
    }
    return result;
}
