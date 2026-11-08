#include "third_party/doctest/doctest.h"

#include <string>

#include "PebbleFL/Common/PblSimOrder.h"

TEST_CASE("sim order defaults to serial and parses address and serial" * doctest::test_suite("regression"))
{
    const PblSimOrderMode ePrev = PblGetSimOrder();
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    CHECK(PblSimOrderName(PBL_SIMORDER_ADDRESS) == std::string("address"));
    CHECK(PblSimOrderName(PBL_SIMORDER_SERIAL) == std::string("serial"));

    PblSimOrderMode eMode = PBL_SIMORDER_ADDRESS;
    CHECK(PblParseSimOrder("serial", &eMode));
    CHECK(eMode == PBL_SIMORDER_SERIAL);
    CHECK(PblParseSimOrder("address", &eMode));
    CHECK(eMode == PBL_SIMORDER_ADDRESS);
    CHECK(PblParseSimOrder("SERIAL", &eMode));
    CHECK(eMode == PBL_SIMORDER_SERIAL);

    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    PblSetSimOrder(PBL_SIMORDER_ADDRESS);
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    PblSetSimOrder(ePrev);
}

TEST_CASE("sim order rejects a bad or missing value and keeps the mode" * doctest::test_suite("regression"))
{
    const PblSimOrderMode ePrev = PblGetSimOrder();
    static const char* const apcBad[] = { 0, "", "-x", "-serial", "bogus", "serialx", "addr" };
    for (unsigned int i = 0; i < sizeof(apcBad) / sizeof(apcBad[0]); ++i)
    {
        INFO(i);
        PblSimOrderMode eMode = PBL_SIMORDER_SERIAL;
        CHECK_FALSE(PblParseSimOrder(apcBad[i], &eMode));
        CHECK(eMode == PBL_SIMORDER_SERIAL);
        eMode = PBL_SIMORDER_ADDRESS;
        CHECK_FALSE(PblParseSimOrder(apcBad[i], &eMode));
        CHECK(eMode == PBL_SIMORDER_ADDRESS);
    }
    CHECK(PblGetSimOrder() == ePrev);
}
