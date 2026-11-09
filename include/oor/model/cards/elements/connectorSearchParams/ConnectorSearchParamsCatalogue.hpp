#pragma once

#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCard.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace oor::model
{

// The kind of value a connector parameter takes (its type in the format).
enum class ConnectorSearchParamsKind : std::uint8_t
{
    Integer,
    Real
};

// What the Type column of the table allows a value to be.
enum class ConnectorSearchParamsRule : std::uint8_t
{
    AnyInteger,          // an integer (GSMOVE: a negative one has a meaning)
    NonNegativeInteger,  // an integer, 0 or more (CSVOUT)
    OneOf,               // an integer of a list (CHKRUN, GMCHK, MOVGAB, NREDIA, WMASS, PRTSW)
    ProjectionDigits,    // ACTVTOL: an integer of up to four digits, each of them 0, 1 or 2
    RealRange,           // a real from `low` to `high` (DRATIO)
    RealRangeOrMinusOne, // a real from `low` to `high`, or -1.0 which skips the check (CNRAGLI, CNRAGLO, GSPROJ)
    AbsoluteRealRange,   // a real whose absolute value is from `low` to `high` (PROJTOL: a negative one changes the
                         // start)
    NonNegativeReal      // a real, 0.0 or more (GSTOL, SCLSKIN)
};

// One parameter that a SWLDPRM card may set. `defaultValue` is the value the solver uses when no SWLDPRM card
// gives the parameter. `low`, `high` are the ends of the range of the real rules, `allowed` the integers of OneOf.
struct ConnectorSearchParamsInfo
{
    std::string_view name; // the neutral name (ProjectionTolerance for PROJTOL)
    ConnectorSearchParamsKind kind;
    double defaultValue;
    ConnectorSearchParamsRule rule;
    double low;
    double high;
    std::span<const std::int32_t> allowed;
    bool onlyGlobal; // true when the parameter is invalid after a CFAST, CWELD or CSEAM keyword
    std::string_view note;
};

// The parameters of the format reference in the order of its table (by their neutral names).
[[nodiscard]] std::span<const ConnectorSearchParamsInfo> connectorSearchParamsCatalogue() noexcept;

// The entry of a neutral name; null for a name the table does not list (a card may use such a name).
[[nodiscard]] const ConnectorSearchParamsInfo* findConnectorSearchParam(std::string_view name) noexcept;

// The name of a kind: "integer" or "real".
[[nodiscard]] std::string_view connectorSearchParamsKindName(ConnectorSearchParamsKind kind) noexcept;

// What the values of a parameter may be, as a phrase ("a real from 0 to 90, or -1").
[[nodiscard]] std::string connectorSearchParamsRuleText(const ConnectorSearchParamsInfo& info);

[[nodiscard]] std::string connectorSearchParamsValueProblem(const ConnectorSearchParamsInfo& info,
                                                            const ConnectorSearchParamsValue& value);

} // namespace oor::model
