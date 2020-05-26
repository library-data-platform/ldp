#include <catch2/catch.hpp>

#include "camelcase.h"

using namespace std;

TEST_CASE( "Test decoding of camel case strings", "[camelcase]" ) {
    vector<pair<string, string>> tests = {
        {"", ""},
        {"c", "c"},
        {"C", "c"},
        {"cc", "cc"},
        {"Cc", "cc"},
        {"CC", "cc"},
        {"cC", "c_c"},
        {"ccc", "ccc"},
        {"Ccc", "ccc"},
        {"CCc", "c_cc"},
        {"CCC", "ccc"},
        {"Ccase", "ccase"},
        {"CCase", "c_case"},
        {"cCase", "c_case"},
        {"CamelC", "camel_c"},
        {"CamelCase", "camel_case"},
        {"camelCase", "camel_case"},
        {"Camelcase", "camelcase"},
        {"CAMELCase", "camel_case"},
        {"CamelCASE", "camel_case"},
        {"CAMELCASE", "camelcase"},
        {"camelsRUs", "camels_r_us"}
    };
    for (auto& t : tests) {
        string s;
        decodeCamelCase(t.first.c_str(), &s);
        REQUIRE(s == t.second);
    }
}

