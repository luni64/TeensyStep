// Tests of the test setup itself, and of the error codes (requirements R14, section 3.4).
#include "core/errorCode.h"
#include "doctest.h"

#include <initializer_list>

using namespace TeensyStep;

TEST_CASE("setup: the tests are compiled as C++17, like the Teensy builds")
{
    CHECK(__cplusplus == 201703L);
}

TEST_CASE("setup: the basic integer types have the sizes of the Teensy")
{
    CHECK(sizeof(int) == 4);
    CHECK(sizeof(long long) == 8);
}

TEST_CASE("errorCode: OK is 0")
{
    CHECK(static_cast<int>(errorCode::OK) == 0);
    CHECK_FALSE(isWarning(errorCode::OK));
    CHECK_FALSE(isError(errorCode::OK));
}

TEST_CASE("errorCode: warnings are -1000 .. -1999, errors are 1000 .. 1999")
{
    for (errorCode w : {errorCode::speedLimited, errorCode::accelerationLimited})
    {
        CAPTURE(static_cast<int>(w));
        CHECK(static_cast<int>(w) <= -1000);
        CHECK(static_cast<int>(w) >= -1999);
        CHECK(isWarning(w));
        CHECK_FALSE(isError(w));
    }
    for (errorCode e : {errorCode::invalidArgument, errorCode::motorBusy, errorCode::noTimer, errorCode::notSupported, errorCode::internal})
    {
        CAPTURE(static_cast<int>(e));
        CHECK(static_cast<int>(e) >= 1000);
        CHECK(static_cast<int>(e) <= 1999);
        CHECK(isError(e));
        CHECK_FALSE(isWarning(e));
    }
}

TEST_CASE("errorCode: the values are part of the API and must not change")
{
    CHECK(static_cast<int>(errorCode::speedLimited) == -1000);
    CHECK(static_cast<int>(errorCode::accelerationLimited) == -1001);
    CHECK(static_cast<int>(errorCode::invalidArgument) == 1000);
    CHECK(static_cast<int>(errorCode::motorBusy) == 1001);
    CHECK(static_cast<int>(errorCode::noTimer) == 1002);
    CHECK(static_cast<int>(errorCode::notSupported) == 1003);
    CHECK(static_cast<int>(errorCode::internal) == 1999);
}
