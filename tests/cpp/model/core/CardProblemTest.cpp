#include "oor/model/core/CardProblem.hpp"

#include <catch2/catch_test_macros.hpp>
using oor::model::CardProblem;
TEST_CASE("card diagnostics expand plain and named neutral fields without translation", "[model][diagnostics]")
{
    const CardProblem problem{
        "card.bad-value", "{field:nodeA} and {field} must differ on {card:Weld}; {field} keeps {literal}", "nodeB"};
    CHECK(problem.text() == "nodeA and nodeB must differ on Weld; nodeB keeps {literal}");
    CHECK(CardProblem{"card.missing-field", "{field} is required", "thickness"}.text() == "thickness is required");
}
