#include "third_party/doctest/doctest.h"

#include "nettest.h"

extern int netLocalIndex[2];

TEST_CASE("local index out of range is minus one" * doctest::test_suite("regression"))
{
    int iSaved0 = netLocalIndex[0];
    int iSaved1 = netLocalIndex[1];
    CHECK(NetComm::GetLocalIndex(-1) == -1);
    CHECK(NetComm::GetLocalIndex(2) == -1);
    netLocalIndex[0] = 5;
    netLocalIndex[1] = 7;
    CHECK(NetComm::GetLocalIndex(0) == 5);
    CHECK(NetComm::GetLocalIndex(1) == 7);
    netLocalIndex[0] = iSaved0;
    netLocalIndex[1] = iSaved1;
}
