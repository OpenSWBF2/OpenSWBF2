#pragma once

enum PblSimOrderMode
{
    PBL_SIMORDER_ADDRESS,
    PBL_SIMORDER_SERIAL
};

PblSimOrderMode PblGetSimOrder();
void PblSetSimOrder(PblSimOrderMode eMode);
bool PblParseSimOrder(const char* pcValue, PblSimOrderMode* peOut);
const char* PblSimOrderName(PblSimOrderMode eMode);
