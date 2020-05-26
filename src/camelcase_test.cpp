#include <catch2/catch.hpp>

#include "camelcase.h"

using namespace std;

TEST_CASE( "Test decoding of camel case strings", "[camelcase]" ) {
    string s;

    decodeCamelCase("CAMELCASE", &s);
    REQUIRE(s == "camelcase");

    decodeCamelCase("camelsRUs", &s);
    REQUIRE(s == "camels_r_us");
}

