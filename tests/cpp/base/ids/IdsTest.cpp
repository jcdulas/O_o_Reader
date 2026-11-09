#include "oor/base/ids/CardKey.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/ids/IdSpace.hpp"

#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

using namespace oor::base;

TEST_CASE("an ID must fit in 32 bits", "[base][ids]")
{
    REQUIRE(toEntityId(0) == 0);
    REQUIRE(toEntityId(2147483647) == 2147483647);
    REQUIRE(toEntityId(-2147483648LL) == -2147483647 - 1);
    REQUIRE_FALSE(toEntityId(2147483648LL).has_value());
    REQUIRE_FALSE(toEntityId(-2147483649LL).has_value());
    static_assert(sizeof(EntityId) == 4);
}

TEST_CASE("card keys compare, hash and start invalid", "[base][ids]")
{
    REQUIRE_FALSE(CardKey{}.isValid());
    REQUIRE(CardKey{0}.isValid());
    REQUIRE(CardKey{3} < CardKey{4});
    REQUIRE(CardKey{3} == CardKey{3});
    std::unordered_set<CardKey> keys{CardKey{1}, CardKey{2}, CardKey{1}};
    REQUIRE(keys.size() == 2);
}

TEST_CASE("ID spaces have names", "[base][ids]")
{
    REQUIRE(idSpaceName(IdSpace::Element) == "element");
    REQUIRE(idSpaceName(IdSpace::DynamicTable) == "dynamic table");
    REQUIRE(idSpaceName(IdSpace::MaterialTable) == "material table");
    REQUIRE(idSpaceName(IdSpace::StressTable) == "stress table");
    REQUIRE(idSpaceName(IdSpace::HeatTable) == "heat table");
    REQUIRE(idSpaceName(IdSpace::HybridDampingRotorOptionsTable) == "damping table");
    REQUIRE(idSpaceName(IdSpace::RandomTable) == "random table");
}
