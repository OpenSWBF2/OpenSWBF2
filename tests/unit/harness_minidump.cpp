#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <dbghelp.h>
#include <string.h>

#include "harness/minidump.h"

namespace
{

template <typename T>
void Put(std::string& sBytes, size_t uOffset, const T& kValue)
{
    if (sBytes.size() < uOffset + sizeof(T))
    {
        sBytes.resize(uOffset + sizeof(T));
    }
    memcpy(&sBytes[uOffset], &kValue, sizeof(T));
}

std::string MakeDump(bool bException, unsigned int uCode, unsigned long long ullAddress, const wchar_t* pwcModule)
{
    std::string sBytes;
    const size_t uDirectory = sizeof(MINIDUMP_HEADER);
    const ULONG32 uStreams = bException ? 2 : 1;
    const size_t uModules = uDirectory + uStreams * sizeof(MINIDUMP_DIRECTORY);
    const size_t uName = uModules + sizeof(ULONG32) + sizeof(MINIDUMP_MODULE);
    const size_t uException = uName + sizeof(ULONG32) + wcslen(pwcModule) * sizeof(wchar_t);

    MINIDUMP_HEADER kHeader;
    memset(&kHeader, 0, sizeof(kHeader));
    kHeader.Signature = MINIDUMP_SIGNATURE;
    kHeader.NumberOfStreams = uStreams;
    kHeader.StreamDirectoryRva = (RVA)uDirectory;
    Put(sBytes, 0, kHeader);

    MINIDUMP_DIRECTORY kModuleEntry;
    memset(&kModuleEntry, 0, sizeof(kModuleEntry));
    kModuleEntry.StreamType = ModuleListStream;
    kModuleEntry.Location.Rva = (RVA)uModules;
    Put(sBytes, uDirectory, kModuleEntry);

    const ULONG32 uCount = 1;
    Put(sBytes, uModules, uCount);
    MINIDUMP_MODULE kModule;
    memset(&kModule, 0, sizeof(kModule));
    kModule.BaseOfImage = 0x00400000ull;
    kModule.SizeOfImage = 0x00100000u;
    kModule.ModuleNameRva = (RVA)uName;
    Put(sBytes, uModules + sizeof(ULONG32), kModule);
    const ULONG32 uNameBytes = (ULONG32)(wcslen(pwcModule) * sizeof(wchar_t));
    Put(sBytes, uName, uNameBytes);
    sBytes.resize(uName + sizeof(ULONG32) + uNameBytes);
    memcpy(&sBytes[uName + sizeof(ULONG32)], pwcModule, uNameBytes);

    if (bException)
    {
        MINIDUMP_DIRECTORY kExceptionEntry;
        memset(&kExceptionEntry, 0, sizeof(kExceptionEntry));
        kExceptionEntry.StreamType = ExceptionStream;
        kExceptionEntry.Location.Rva = (RVA)uException;
        Put(sBytes, uDirectory + sizeof(MINIDUMP_DIRECTORY), kExceptionEntry);
        MINIDUMP_EXCEPTION_STREAM kStream;
        memset(&kStream, 0, sizeof(kStream));
        kStream.ExceptionRecord.ExceptionCode = uCode;
        kStream.ExceptionRecord.ExceptionAddress = ullAddress;
        Put(sBytes, uException, kStream);
    }
    return sBytes;
}

const char* const kFatal =
    "FATAL: exception 0xC0000005 at 0x00401234 (BattlefrontII.exe+0x1234) build 5F00000012345 dump C:\\x\\a.dmp";

}

TEST_CASE_NAMED(FHarnessMinidumpTest1, "Unit::HarnessMinidump::a dump matches the FATAL exception line by code and address and module and offset", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(true, 0xC0000005u, 0x00401234ull, L"C:\\game\\BattlefrontII.exe");
    MinidumpException kDump;
    std::string sWhy;
    REQUIRE(ReadMinidumpException(sDump, kDump, sWhy));
    CHECK(kDump.bHasException);
    CHECK(kDump.uCode == 0xC0000005u);
    CHECK(kDump.ullAddress == 0x00401234ull);
    CHECK(kDump.bHasModule);
    CHECK(kDump.sModule == "BattlefrontII.exe");
    CHECK(kDump.ullBase == 0x00400000ull);
    CHECK(DumpMatchesFatal(sDump, kFatal, sWhy));
}

TEST_CASE_NAMED(FHarnessMinidumpTest2, "Unit::HarnessMinidump::a dump whose exception code differs from the FATAL line is rejected", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(true, 0xC0000094u, 0x00401234ull, L"C:\\game\\BattlefrontII.exe");
    std::string sWhy;
    CHECK(!DumpMatchesFatal(sDump, kFatal, sWhy));
    CHECK(sWhy == "code dump 0xC0000094 FATAL 0xC0000005");
}

TEST_CASE_NAMED(FHarnessMinidumpTest3, "Unit::HarnessMinidump::a dump whose exception address differs from the FATAL line is rejected", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(true, 0xC0000005u, 0x00401238ull, L"C:\\game\\BattlefrontII.exe");
    std::string sWhy;
    CHECK(!DumpMatchesFatal(sDump, kFatal, sWhy));
    CHECK(sWhy == "address dump 0x401238 FATAL 0x401234; offset dump 0x1238 FATAL 0x1234");
}

TEST_CASE_NAMED(FHarnessMinidumpTest4, "Unit::HarnessMinidump::a dump whose module differs from the FATAL line is rejected", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(true, 0xC0000005u, 0x00401234ull, L"C:\\Windows\\KERNELBASE.dll");
    std::string sWhy;
    CHECK(!DumpMatchesFatal(sDump, kFatal, sWhy));
    CHECK(sWhy == "module dump KERNELBASE.dll FATAL BattlefrontII.exe");
    const std::string sOutside = MakeDump(true, 0xC0000005u, 0x00601234ull, L"C:\\game\\BattlefrontII.exe");
    CHECK(!DumpMatchesFatal(sOutside,
                            "FATAL: exception 0xC0000005 at 0x00601234 (BattlefrontII.exe+0x201234) build 1 dump a.dmp",
                            sWhy));
    CHECK(sWhy == "module dump none FATAL BattlefrontII.exe; offset dump none FATAL 0x201234");
}

TEST_CASE_NAMED(FHarnessMinidumpTest5, "Unit::HarnessMinidump::a truncated dump is rejected at every cut", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(true, 0xC0000005u, 0x00401234ull, L"C:\\game\\BattlefrontII.exe");
    std::string sWhy;
    REQUIRE(DumpMatchesFatal(sDump, kFatal, sWhy));
    for (size_t uLen = 0; uLen < sDump.size(); ++uLen)
    {
        CHECK_MESSAGE(!DumpMatchesFatal(sDump.substr(0, uLen), kFatal, sWhy), "cut at " << uLen);
    }
    MinidumpException kDump;
    CHECK(!ReadMinidumpException(sDump.substr(0, sizeof(MINIDUMP_HEADER) + 4), kDump, sWhy));
    CHECK(sWhy.compare(0, 20, "truncated at offset ") == 0);
}

TEST_CASE_NAMED(FHarnessMinidumpTest6, "Unit::HarnessMinidump::a dump with no exception stream is rejected for an exception FATAL line", "[SmokeFilter]")
{
    const std::string sDump = MakeDump(false, 0, 0, L"C:\\game\\BattlefrontII.exe");
    MinidumpException kDump;
    std::string sWhy;
    REQUIRE(ReadMinidumpException(sDump, kDump, sWhy));
    CHECK(!kDump.bHasException);
    CHECK(!DumpMatchesFatal(sDump, kFatal, sWhy));
    CHECK(sWhy == "the dump has no exception stream");
    std::string sBad = sDump;
    sBad[0] = 'X';
    CHECK(!DumpMatchesFatal(sBad, kFatal, sWhy));
    CHECK(sWhy == "dump: no MDMP signature");
    CHECK(!DumpMatchesFatal(sDump, "", sWhy));
    CHECK(sWhy == "no FATAL line");
}

TEST_CASE_NAMED(FHarnessMinidumpTest7, "Unit::HarnessMinidump::a check failure needs exception 0xE0000004 and a FATAL line without an exception needs a dump without one", "[SmokeFilter]")
{
    const char* const pcCheck = "FATAL: check failed: p != 0 at x.cpp:1 build 1 dump a.dmp";
    const char* const pcHang = "FATAL: hang (supervisor saw no progress, last frame 3) build 1 dump a.dmp";
    std::string sWhy;
    CHECK(DumpMatchesFatal(MakeDump(true, 0xE0000004u, 0x00401234ull, L"BattlefrontII.exe"), pcCheck, sWhy));
    CHECK(!DumpMatchesFatal(MakeDump(true, 0xC0000005u, 0x00401234ull, L"BattlefrontII.exe"), pcCheck, sWhy));
    CHECK(sWhy == "a check failure dump needs exception 0xE0000004, the dump has 0xC0000005");
    CHECK(!DumpMatchesFatal(MakeDump(false, 0, 0, L"BattlefrontII.exe"), pcCheck, sWhy));
    CHECK(DumpMatchesFatal(MakeDump(false, 0, 0, L"BattlefrontII.exe"), pcHang, sWhy));
    CHECK(!DumpMatchesFatal(MakeDump(true, 0xC0000005u, 0x00401234ull, L"BattlefrontII.exe"), pcHang, sWhy));
    CHECK(sWhy == "the dump holds exception 0xC0000005, the FATAL line names none");
}
