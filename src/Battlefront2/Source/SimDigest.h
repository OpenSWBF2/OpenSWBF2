#pragma once

namespace SimDigest
{

unsigned int StateHash();
void TraceTurn();
void LogFactories();
void LogFactoryCounts();
void Reset();
#ifdef OPENSWBF2_TEST
struct DetailRecord
{
    unsigned int uTurns;
    unsigned int uFirst;
    unsigned int uLast;
    unsigned int uTimes;
    unsigned int uCharacters;
    unsigned int uObjects;
    unsigned int uTeams;
};
const DetailRecord& GetDetailRecord();
#endif

}
