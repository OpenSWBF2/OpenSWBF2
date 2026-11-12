#include "third_party/doctest/doctest.h"

#include <string.h>

#include "GameSpyFL/md5.h"

namespace
{

void CheckDigest(const char* pcText, const char* pcExpected)
{
    char acHex[33];
    memset(acHex, 0x5A, sizeof(acHex));
    MD5Digest(pcText, (unsigned int)strlen(pcText), acHex);
    CAPTURE(pcText);
    CHECK(strcmp(acHex, pcExpected) == 0);
}

}

TEST_CASE("netadmin md5 matches rfc 1321" * doctest::test_suite("unit"))
{
    CheckDigest("abc", "900150983cd24fb0d6963f7d28e17f72");
    CheckDigest("", "d41d8cd98f00b204e9800998ecf8427e");
    CheckDigest("12345678901234567890123456789012345678901234567890123456789012345678901234567890",
                "57edf4a22be3c955ac49da2e2107b67a");
}
