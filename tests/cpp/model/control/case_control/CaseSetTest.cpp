#include "oor/model/control/case_control/CaseSet.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor::model;

namespace
{

CaseSet parsed(const std::string& text)
{
    std::string problem;
    const auto set = CaseSet::parse(text, problem);
    INFO(problem);
    REQUIRE(set.has_value());
    return *set;
}

std::string refused(const std::string& text)
{
    std::string problem;
    REQUIRE_FALSE(CaseSet::parse(text, problem).has_value());
    return problem;
}

using Ids = std::vector<std::int64_t>;

} // namespace

TEST_CASE("a set of IDs is read in order", "[control][set]")
{
    const CaseSet set = parsed("101,116 ,117,108,109,132");
    REQUIRE(set.kind == CaseSetKind::Integers);
    REQUIRE(set.integers == Ids{101, 116, 117, 108, 109, 132});
    REQUIRE(set.contains(108));
    REQUIRE_FALSE(set.contains(1));
}

TEST_CASE("Through and By expand to the IDs they stand for", "[control][set]")
{
    REQUIRE(parsed("1 Through 5").integers == Ids{1, 2, 3, 4, 5});
    REQUIRE(parsed("5, 6, 7 Through 10, 20").integers == Ids{5, 6, 7, 8, 9, 10, 20});
    REQUIRE(parsed("1 Through 10 By 3").integers == Ids{1, 4, 7, 10});
    REQUIRE(parsed("4 Through 4").integers == Ids{4});
}

TEST_CASE("a set is not expanded twice and keeps the order it was given", "[control][set]")
{
    REQUIRE(parsed("3, 1, 3, 2").integers == Ids{3, 1, 2});
}

TEST_CASE("reals, All and names make the other kinds of set", "[control][set]")
{
    const CaseSet reals = parsed("1.0, 2.5, 3");
    REQUIRE(reals.kind == CaseSetKind::Reals);
    REQUIRE(reals.reals == std::vector<double>{1.0, 2.5, 3.0});

    REQUIRE(parsed("All").kind == CaseSetKind::All);
    REQUIRE(parsed("all").words == std::vector<std::string>{"all"});

    const CaseSet pairs = parsed("101/T1, 501/t3, 12/All");
    REQUIRE(pairs.kind == CaseSetKind::Words);
    REQUIRE(pairs.words == std::vector<std::string>{"101/T1", "501/t3", "12/All"});
    REQUIRE(parsed("M1,M2").words == std::vector<std::string>{"M1", "M2"});
}

TEST_CASE("runs of three or more consecutive IDs are written with Through", "[control][set]")
{
    using Items = std::vector<std::string>;
    REQUIRE(CaseSet::ofIntegers({1, 2, 3, 4, 5, 9}).items() == Items{"1 Through 5", "9"});
    REQUIRE(CaseSet::ofIntegers({1, 2, 5}).items() == Items{"1", "2", "5"});
    REQUIRE(CaseSet::ofIntegers({5, 3, 1}).items() == Items{"5", "3", "1"});
    REQUIRE(CaseSet::ofAll().items() == Items{"All"});
    REQUIRE(CaseSet::ofReals({1.0, 2.5}).items() == Items{"1.", "2.5"});
}

TEST_CASE("a set that cannot be read says why", "[control][set]")
{
    REQUIRE(refused("").find("no members") != std::string::npos);
    REQUIRE(refused("Through 5").find("Through") != std::string::npos);
    REQUIRE(refused("10 Through 5").find("ascending") != std::string::npos);
    REQUIRE(refused("1 Through").find("missing") != std::string::npos);
    REQUIRE(refused("Except 3, 4").find("Except") != std::string::npos);
    REQUIRE(refused("1 Through 2000000000").find("members") != std::string::npos);
}

TEST_CASE("a long Except list costs one pass over the set, and no end of a range overflows",
          "[control][set][robustness]")
{
    // 20 000 IDs after Except, each removed from a range of a million: one pass, not 20 000.
    std::string text = "1 Through 1000000 Except";
    for (int id = 2; id <= 20'001; ++id)
        text += " " + std::to_string(id);
    text += " 2000000";
    const CaseSet set = parsed(text);
    REQUIRE(set.integers.size() == 1'000'000 - 20'000 + 1);
    REQUIRE(set.integers[0] == 1);
    REQUIRE(set.integers[1] == 20'002);
    REQUIRE(set.integers.back() == 2'000'000);
    // the IDs after the Except that share an end with the one before are removed too
    REQUIRE(parsed("1 Through 20 Except 2 Through 4 By 2, 4 Through 6").integers ==
            Ids{1, 3, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20});
    // the ends of the 64-bit range: counted and stepped without overflow
    REQUIRE(parsed("9223372036854775806 Through 9223372036854775807").integers ==
            Ids{9223372036854775806, 9223372036854775807});
    REQUIRE(refused("-9223372036854775807 Through 9223372036854775807").find("members") != std::string::npos);
    REQUIRE(parsed("-9223372036854775807 Through 9223372036854775807 By 9223372036854775807").integers ==
            Ids{-9223372036854775807, 0, 9223372036854775807});
}
