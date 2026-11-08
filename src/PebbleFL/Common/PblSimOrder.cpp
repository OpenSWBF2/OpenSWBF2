#include "PblSimOrder.h"

#include <string.h>

static PblSimOrderMode sSimOrder = PBL_SIMORDER_SERIAL;

PblSimOrderMode PblGetSimOrder()
{
    return sSimOrder;
}

void PblSetSimOrder(PblSimOrderMode eMode)
{
    sSimOrder = eMode;
}

bool PblParseSimOrder(const char* pcValue, PblSimOrderMode* peOut)
{
    if (!pcValue || !pcValue[0] || pcValue[0] == '-')
    {
        return false;
    }
    if (_stricmp(pcValue, "address") == 0)
    {
        *peOut = PBL_SIMORDER_ADDRESS;
        return true;
    }
    if (_stricmp(pcValue, "serial") == 0)
    {
        *peOut = PBL_SIMORDER_SERIAL;
        return true;
    }
    return false;
}

const char* PblSimOrderName(PblSimOrderMode eMode)
{
    return eMode == PBL_SIMORDER_SERIAL ? "serial" : "address";
}
