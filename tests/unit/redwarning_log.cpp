#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <stdio.h>
#include <string>

#include "RedEngineFL/Util/RedWarning.h"

namespace
{

std::string ReadWholeFile(const std::string& sPath)
{
    std::string sData;
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (pFile)
    {
        char acBuf[512];
        size_t uiRead;
        while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
        {
            sData.append(acBuf, uiRead);
        }
        fclose(pFile);
    }
    return sData;
}

std::string TempDir()
{
    char acTemp[MAX_PATH];
    GetTempPathA(MAX_PATH, acTemp);
    return std::string(acTemp);
}

void LogOne(const std::string& sPath, const char* pcMessage)
{
    RedWarning::SetLogFileName(sPath.c_str());
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_FILE, RedWarning::RED_SEVERITY_INFORM);
    RedWarning::SetLogData(RedWarning::RED_SEVERITY_INFORM);
    RedWarning::LogMessage("%s\n", pcMessage);
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_FILE, RedWarning::RED_SEVERITY_NONE);
}

}

TEST_CASE("file log writes percent text literally" * doctest::test_suite("regression"))
{
    std::string sPath = TempDir() + "owb2_pct.log";
    DeleteFileA(sPath.c_str());
    LogOne(sPath, "100%d%x marker");
    std::string sData = ReadWholeFile(sPath);
    CHECK(sData.find("100%d%x marker") != std::string::npos);
    DeleteFileA(sPath.c_str());
}

TEST_CASE("log file name accepts a long path" * doctest::test_suite("regression"))
{
    std::string sDir = TempDir() + "owb2_long_directory_name_one";
    std::string sDir2 = sDir + "\\owb2_long_directory_name_two_padding_padding";
    std::string sDir3 = sDir2 + "\\owb2_long_directory_name_three_padding_padding";
    CreateDirectoryA(sDir.c_str(), 0);
    CreateDirectoryA(sDir2.c_str(), 0);
    CreateDirectoryA(sDir3.c_str(), 0);
    std::string sPath = sDir3 + "\\owb2_long_file_name_padding_padding_padding.log";
    REQUIRE(sPath.size() > 150);
    REQUIRE(sPath.size() < MAX_PATH);
    LogOne(sPath, "long path marker");
    std::string sData = ReadWholeFile(sPath);
    CHECK(sData.find("long path marker") != std::string::npos);
    DeleteFileA(sPath.c_str());
    RemoveDirectoryA(sDir3.c_str());
    RemoveDirectoryA(sDir2.c_str());
    RemoveDirectoryA(sDir.c_str());
}

TEST_CASE("log file is truncated at open" * doctest::test_suite("regression"))
{
    std::string sPath = TempDir() + "owb2_trunc.log";
    FILE* pFile = fopen(sPath.c_str(), "wb");
    REQUIRE(pFile);
    fputs("OLDCONTENT", pFile);
    fclose(pFile);
    LogOne(sPath, "fresh marker");
    std::string sData = ReadWholeFile(sPath);
    CHECK(sData.find("OLDCONTENT") == std::string::npos);
    CHECK(sData.find("fresh marker") != std::string::npos);
    DeleteFileA(sPath.c_str());
}

TEST_CASE("ten existing log files fall back to BFront2.log" * doctest::test_suite("regression"))
{
    char acOldDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, acOldDir);
    std::string sDir = TempDir() + "owb2_fallback";
    CreateDirectoryA(sDir.c_str(), 0);
    SetCurrentDirectoryA(sDir.c_str());
    for (int i = 0; i < 10; ++i)
    {
        char acName[64];
        sprintf(acName, "BFront2_%d.log", i);
        FILE* pFile = fopen(acName, "wb");
        REQUIRE(pFile);
        fclose(pFile);
    }
    DeleteFileA("BFront2.log");

    RedWarning::SetLogFileOverwrite(false);
    RedWarning::Init();
    RedWarning::SetLogFileOverwrite(true);
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_FILE, RedWarning::RED_SEVERITY_INFORM);
    RedWarning::SetLogData(RedWarning::RED_SEVERITY_INFORM);
    RedWarning::LogMessage("%s\n", "fallback marker");
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_FILE, RedWarning::RED_SEVERITY_NONE);

    std::string sData = ReadWholeFile("BFront2.log");
    CHECK(sData.find("fallback marker") != std::string::npos);

    DeleteFileA("BFront2.log");
    for (int i = 0; i < 10; ++i)
    {
        char acName[64];
        sprintf(acName, "BFront2_%d.log", i);
        DeleteFileA(acName);
    }
    SetCurrentDirectoryA(acOldDir);
    RemoveDirectoryA(sDir.c_str());
    RedWarning::Init();
}
