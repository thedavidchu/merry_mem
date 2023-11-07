#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("Catch2 should work") {
    const double a = 1.0;
    const double b = 2.0;

    const double result = a / b;
    const double expected = 0.5;

    using Catch::Matchers::WithinRel;
    REQUIRE_THAT(result, WithinRel(expected));
}
