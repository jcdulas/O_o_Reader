#pragma once

#include "oor/analysis/connections/ContactOptions.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/QueryValue.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// The parameters `contact` ("merge", "connection" or "ignore"), `contactTolerance` and `contactRule` ("components" or
// "solver") of the component queries (contact between bodies), and `connections` (the elements a caller declares
// connections, a companion package), in that order.
[[nodiscard]] std::vector<ParameterInfo> contactParameters();
// The parameter `contactTolerance` alone (the contact queries, which have no mode).
[[nodiscard]] ParameterInfo contactToleranceParameter();

// The parameter `rule` of contactNodePairs: "components" (the default) or "solver".
[[nodiscard]] ParameterInfo contactRuleParameter();
// The rule of a call: QueryError naming `query` for another text.
[[nodiscard]] analysis::ContactRule contactRuleOf(const QueryArguments& arguments, std::string_view query);

// The contact options of a call: QueryError naming `query` for a mode that is none of the three, a rule that is none of
// the two, a tolerance that is not a positive number or a connection that is not a positive ID (the declared
// connections sorted and distinct).
[[nodiscard]] analysis::ContactOptions contactOptionsOf(const QueryArguments& arguments, std::string_view query);

// The contact options as text, for the key under which a query keeps its table with the model (ComponentCache::memo):
// two options give the same text exactly when they are equal (the tolerance by its bits).
[[nodiscard]] std::string contactKey(const analysis::ContactOptions& contact);

} // namespace oor::query
