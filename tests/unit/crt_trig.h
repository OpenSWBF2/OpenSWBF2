#pragma once

#include <string>
#include <vector>

enum CrtTrigFn
{
    CRT_TRIG_SIN,
    CRT_TRIG_COS,
    CRT_TRIG_TAN,
    CRT_TRIG_FNS
};

enum CrtTrigRoute
{
    CRT_TRIG_PRECISE,
    CRT_TRIG_PLAIN,
    CRT_TRIG_ROUTES
};

enum CrtTrigPath
{
    CRT_TRIG_PATH_SSE,
    CRT_TRIG_PATH_TINY,
    CRT_TRIG_PATH_X87
};

enum CrtTrigSet
{
    CRT_SET_TRIG,
    CRT_SET_SIM,
    CRT_SETS
};

enum CrtSimFn
{
    CRT_SIM_SIN,
    CRT_SIM_COS,
    CRT_SIM_TAN,
    CRT_SIM_SIN_STRICT,
    CRT_SIM_COS_STRICT,
    CRT_SIM_ASIN,
    CRT_SIM_ACOS,
    CRT_SIM_ATAN,
    CRT_SIM_ATAN2,
    CRT_SIM_EXP,
    CRT_SIM_LOG,
    CRT_SIM_LOG10,
    CRT_SIM_POW,
    CRT_SIM_FMOD,
    CRT_SIM_FNS
};

static const unsigned int CRT_SIM_BLOCK = 65536;

struct CrtSimIn
{
    unsigned int uX;
    unsigned int uY;
};

struct CrtSimBlock
{
    std::string sLine;
    std::string sRange;
};

typedef float (*CrtSimEval)(unsigned int uFn, float fX, float fY);

struct CrtTrigEntry
{
    unsigned int uSet;
    unsigned int uFn;
    unsigned int uRoute;
    std::vector<unsigned long long> vOut;
};

struct CrtTrigReference
{
    unsigned int auVersion[4];
    std::vector<CrtTrigEntry> vEntries;
};

static const char CRT_TRIG_MAGIC[8] = {'C', 'R', 'T', 'T', 'R', 'I', 'G', '2'};

const char* CrtSimName(unsigned int uFn);
bool CrtSimIsPair(unsigned int uFn);
bool CrtSimHasX87(unsigned int uFn);
bool CrtSimIsX87(unsigned int uFn, const CrtSimIn& kIn);
void CrtSimInputs(unsigned int uFn, std::vector<CrtSimIn>& vIn);
void CrtSimSplit(unsigned int uFn, std::vector<CrtSimIn>& vPinned, std::vector<CrtSimIn>& vX87);
std::string CrtSimRange(const std::vector<CrtSimIn>& vIn, size_t uFirst, size_t uCount);
void CrtSimPinBlocks(CrtSimEval pfnEval, std::vector<CrtSimBlock>& vBlocks);
float CrtSimRaw(unsigned int uFn, float fX, float fY);
int CrtSimWritePins(const std::string& sOutDir);

const char* CrtTrigName(unsigned int uFn);
bool CrtTrigHasRoute(unsigned int uFn, unsigned int uRoute);
void CrtTrigInputs(unsigned int uFn, std::vector<unsigned long long>& vIn);
unsigned int CrtTrigPathOf(unsigned int uFn, unsigned long long uIn);
bool CrtTrigReadFile(const std::string& sPath, CrtTrigReference& kRef, std::string& sWhy);
bool CrtTrigWriteFile(const char* pcPath, const CrtTrigReference& kRef);

bool CrtTrigInProcess(CrtTrigReference& kRef, std::string& sWhy);
double CrtTrigPlain(unsigned int uFn, double d);
int CrtTrigRefWriteFile(const char* pcPath);
