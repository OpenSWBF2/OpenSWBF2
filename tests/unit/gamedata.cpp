#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdio.h>
#include <string>

#include "testutil.h"

TEST_CASE_NAMED(FGamedataTest1, "Unit::Gamedata::gamedata core lvl header", "[SmokeFilter]")
{
    REQUIRE_GAMEDATA();
    std::string sPath = std::string(FindGameDataRoot()) + "\\data\\_lvl_pc\\core.lvl";
    FILE* pFile = fopen(sPath.c_str(), "rb");
    REQUIRE(pFile);
    unsigned int auiHeader[2] = { 0, 0 };
    size_t uiRead = fread(auiHeader, 1, sizeof(auiHeader), pFile);
    fseek(pFile, 0, SEEK_END);
    long lFileSize = ftell(pFile);
    fclose(pFile);
    printf("core.lvl magic=0x%08X size=%u\n", auiHeader[0], auiHeader[1]);
    REQUIRE(uiRead == sizeof(auiHeader));
    CHECK(auiHeader[0] == 0x62666375);
    CHECK((long)auiHeader[1] == lFileSize - 8);
}
