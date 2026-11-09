#pragma once

#include "support/ExplicitTestSupport.hpp"

namespace oor::testing
{

// Arbitrary text remains exact and reads back through the neutral model format.
inline void requireLiteralRoundTrip(const std::string& record, std::string_view literal)
{
    INFO(record);
    const model::Model model = readBulk(record);
    REQUIRE(writeModelText(model).find(literal) != std::string::npos);
    requireModelFileRoundTrip(model);
}

} // namespace oor::testing
