#pragma once

#include "oor/api/Deck.hpp"
#include "oor/api/Results.hpp"
#include "oor/base/text/Json.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/named/QueryValue.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::api
{

// A named query of the catalogue, as a caller sees it.
struct QueryDescription
{
    std::string name;
    std::string family; // "T1" ... (topology of the model), "R1" ... (results)
    std::string description;
    bool resultQuery = false; // reads results
    bool needsModel = false;  // cannot run on results alone
    // The parameters, the query's own then the common ones (`columns`, `sort`, `limit`, ...).
    std::vector<query::ParameterInfo> parameters;
};

// Every named query, in the order of the catalogue.
[[nodiscard]] std::vector<QueryDescription> queryCatalogue();

// The kind of value a parameter of a query takes; throws QueryError for an unknown query or parameter.
[[nodiscard]] query::ParameterKind parameterKind(const std::string& queryName, const std::string& parameter);

// Runs a named query on a deck and/or results (either may be null when the query does not need it). Throws QueryError
// for an unknown query, parameter or column.
[[nodiscard]] std::shared_ptr<const query::QueryTable> runQuery(const std::string& name,
                                                                const std::shared_ptr<Deck>& deck,
                                                                const std::shared_ptr<Results>& results,
                                                                const query::QueryArguments& arguments);

// The arguments of a query from a JSON object {"parameter": value, ...} (what the JavaScript side sends): each value is
// converted to the kind its parameter takes (ParameterKind):
//   Bool true/false; Integer, Real a number; Text a text; IdList a number or an array of numbers (null: all);
//   IdListOrText, RealsOrText a text or numbers; TextList a text or an array of texts; Point, Reals an array of
//   numbers; Steps an array of step values, or {"start": 0, "stop": 10, "step": 2} for positions; Pairs an array of
//   [n1, n2]; Thresholds, Values an object {"name": number}; IdSets an object {"name": [IDs]} or an array of SET1 IDs;
//   Groups {"name": {"components": [IDs]}} or {"name": {"elements": [IDs]}}; Combinations {"name": {"1": 1.5}};
//   Cuts {"name": {"plane": [x, y, z, nx, ny, nz]}} or {"name": {"nodes": [IDs], "sideElements": [IDs]}}.
// A parameter that takes another model (ParameterKind::Model) is not available from JSON. Throws UsageError for a value
// of the wrong type, QueryError for an unknown query or parameter.
[[nodiscard]] query::QueryArguments argumentsFromJson(const std::string& queryName, const std::string& json);

// The table as CSV text.
[[nodiscard]] std::string toCsv(const query::QueryTable& table);

} // namespace oor::api
