#include <catch2/catch_all.hpp>

TEST_CASE("Basic mathematics test", "[math]")
{
  REQUIRE(1 == 1);
  REQUIRE(2 + 2 == 4);
  REQUIRE(5 * 5 == 25);
}

TEST_CASE("Boolean logic test", "[logic]")
{
  REQUIRE(true == true);
  REQUIRE(false == false);
  REQUIRE(true != false);
}

TEST_CASE("String operations test", "[string]")
{
    std::string test = "breakout";
    REQUIRE(test == "breakout");
    REQUIRE(test.length() == 8);
}