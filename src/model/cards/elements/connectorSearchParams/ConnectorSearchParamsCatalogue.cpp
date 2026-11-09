#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCatalogue.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace oor::model
{

namespace
{

constexpr ConnectorSearchParamsKind INT = ConnectorSearchParamsKind::Integer;
constexpr ConnectorSearchParamsKind REAL = ConnectorSearchParamsKind::Real;

// The integers of the parameters that take one of a list.
constexpr std::array<std::int32_t, 3> STOP_AFTER_CHECK_VALUES{0, 1, 2};
constexpr std::array<std::int32_t, 4> GEOMETRY_CHECK_LEVEL_VALUES{0, 1, 2, 3};
constexpr std::array<std::int32_t, 2> MOVE_END_POINTS_VALUES{0, 1};
constexpr std::array<std::int32_t, 9> DIAMETER_HALVING_VALUES{0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr std::array<std::int32_t, 2> WELD_MASS_VALUES{0, 1};
constexpr std::array<std::int32_t, 11> GENERATED_DATA_PRINT_VALUES{0, 1, 2, 11, 12, 100, 101, 111, 200, 201, 211};

constexpr std::span<const std::int32_t> NO_VALUES;

// The parameters of SWLDPRM, typed by hand.
constexpr auto CATALOGUE = std::to_array<ConnectorSearchParamsInfo>({
    {"ProjectionToleranceSwitches", INT, 1111.0, ConnectorSearchParamsRule::ProjectionDigits, 0.0, 0.0, NO_VALUES,
     false,
     "weld connectors: four digits, one for each connection method (PARTPAT, ELPAT, ELEMID, GRIDID), that turn the "
     "projection tolerance off (0), "
     "on (1) or always on (2, ELPAT and PARTPAT only)"},
    {"StopAfterConnectivityCheck", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, STOP_AFTER_CHECK_VALUES, true,
     "after the connectivity is generated: 0 stops at the first error, 1 stops once it is checked, 2 goes on when "
     "there is no error (global only)"},
    {"SeamEndMinEdgeAngle", REAL, 160.0, ConnectorSearchParamsRule::RealRangeOrMinusOne, 90.0, 180.0, NO_VALUES, false,
     "seam connectors: the smallest angle between the free edges of the shell elements at the two ends (-1.0 skips the "
     "check)"},
    {"SeamEndMaxNormalAngle", REAL, 20.0, ConnectorSearchParamsRule::RealRangeOrMinusOne, 0.0, 90.0, NO_VALUES, false,
     "seam connectors: the largest angle between the shell normals at the two ends (-1.0 skips the check)"},
    {"DiagnosticTableUnit", INT, 0.0, ConnectorSearchParamsRule::NonNegativeInteger, 0.0, 0.0, NO_VALUES, false,
     "the unit number of a .csv file for the diagnostics (0: none)"},
    {"PatchDiameterFactor", REAL, 1.0, ConnectorSearchParamsRule::RealRange, 1.0, 10.0, NO_VALUES, false,
     "fastener and weld connectors (PARTPAT and ELPAT): the factor on the diameter of the connector patch"},
    {"GeometryCheckLevel", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, GEOMETRY_CHECK_LEVEL_VALUES, false,
     "seam, fastener and weld connectors (ELPAT, PARTPAT): geometry checks, 0 none, 1 check, 2 check and list the "
     "candidate "
     "elements, 3 check backward projections"},
    {"MaxEndPointMoves", INT, 0.0, ConnectorSearchParamsRule::AnyInteger, 0.0, 0.0, NO_VALUES, false,
     "how many times the start point (and the end point of a seam) may be moved when a complete projection is not "
     "found"},
    {"MoveEndPointsToProjection", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, MOVE_END_POINTS_VALUES, false,
     "Fastener and weld connectors: 1 makes new grid points at the corrected places of the two end points, 0 keeps the "
     "places of the deck"},
    {"MaxShellNormalAngle", REAL, 20.0, ConnectorSearchParamsRule::RealRangeOrMinusOne, 0.0, 90.0, NO_VALUES, false,
     "the largest angle between the normals of shell A and shell B (-1.0 skips the check)"},
    {"MaxProjectionDistance", REAL, 0.0, ConnectorSearchParamsRule::NonNegativeReal, 0.0, 0.0, NO_VALUES, false,
     "the largest distance from the start (or end) point to its projection (0.0: no limit)"},
    {"MaxDiameterHalvings", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, DIAMETER_HALVING_VALUES, false,
     "fastener and weld connectors (PARTPAT and ELPAT): how many times the diameter may be halved"},
    {"ProjectionTolerance", REAL, 0.02, ConnectorSearchParamsRule::AbsoluteRealRange, 0.0, 1.0, NO_VALUES, false,
     "the tolerance for a projection outside the shell element, as a fraction of its size (a negative value starts "
     "the auxiliary projections at its absolute value)"},
    {"WeldMass", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, WELD_MASS_VALUES, false,
     "Weld connectors: 1 lets the weld have mass when its material has a density"},
    {"GeneratedDataPrint", INT, 0.0, ConnectorSearchParamsRule::OneOf, 0.0, 0.0, GENERATED_DATA_PRINT_VALUES, false,
     "print or punch the generated elements and grid points: 0 nothing, 1, 2, 11 and 12 diagnostics, 100, 101 and 111 "
     "the Bulk Data, 200, 201 and 211 the equivalent spring-damper and beam element data"},
    {"PatchStiffnessFactor", REAL, 0.0, ConnectorSearchParamsRule::NonNegativeReal, 0.0, 0.0, NO_VALUES, false,
     "fastener and weld connectors (PARTPAT and ELPAT): the factor that stiffens the connector patch (0.1 is a first "
     "value to try)"},
});

// A number as a short phrase: 90 and 0.02, not 90.000000.
std::string numberText(double value)
{
    std::ostringstream text;
    text << value;
    return text.str();
}

} // namespace

std::span<const ConnectorSearchParamsInfo> connectorSearchParamsCatalogue() noexcept
{
    return CATALOGUE;
}

const ConnectorSearchParamsInfo* findConnectorSearchParam(std::string_view name) noexcept
{
    for (const ConnectorSearchParamsInfo& info : CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

std::string_view connectorSearchParamsKindName(ConnectorSearchParamsKind kind) noexcept
{
    return kind == ConnectorSearchParamsKind::Integer ? "integer" : "real";
}

std::string connectorSearchParamsRuleText(const ConnectorSearchParamsInfo& info)
{
    switch (info.rule)
    {
    case ConnectorSearchParamsRule::AnyInteger:
        return "an integer";
    case ConnectorSearchParamsRule::NonNegativeInteger:
        return "an integer, 0 or more";
    case ConnectorSearchParamsRule::OneOf:
    {
        std::string text = "one of ";
        for (std::size_t index = 0; index < info.allowed.size(); ++index)
        {
            if (index > 0)
                text += index + 1 == info.allowed.size() ? " or " : ", ";
            text += std::to_string(info.allowed[index]);
        }
        return text;
    }
    case ConnectorSearchParamsRule::ProjectionDigits:
        return "an integer of up to four digits, each of them 0, 1 or 2";
    case ConnectorSearchParamsRule::RealRange:
        return "a real from " + numberText(info.low) + " to " + numberText(info.high);
    case ConnectorSearchParamsRule::RealRangeOrMinusOne:
        return "a real from " + numberText(info.low) + " to " + numberText(info.high) + ", or -1";
    case ConnectorSearchParamsRule::AbsoluteRealRange:
        return "a real whose absolute value is from " + numberText(info.low) + " to " + numberText(info.high);
    case ConnectorSearchParamsRule::NonNegativeReal:
        return "a real, 0.0 or more";
    }
    return {};
}

std::string connectorSearchParamsValueProblem(const ConnectorSearchParamsInfo& info,
                                              const ConnectorSearchParamsValue& value)
{
    const std::string what = std::string(info.name) + " must be " + connectorSearchParamsRuleText(info);
    if (info.kind == ConnectorSearchParamsKind::Integer && !value.isInteger())
        return what + " (an integer was expected)";
    const double number = value.asReal();
    if (!std::isfinite(number))
        return what;
    switch (info.rule)
    {
    case ConnectorSearchParamsRule::AnyInteger:
        return {};
    case ConnectorSearchParamsRule::NonNegativeInteger:
        return number >= 0.0 ? std::string() : what;
    case ConnectorSearchParamsRule::OneOf:
        return std::find(info.allowed.begin(), info.allowed.end(), std::get<std::int32_t>(value.number)) !=
                       info.allowed.end()
                   ? std::string()
                   : what;
    case ConnectorSearchParamsRule::ProjectionDigits:
    {
        std::int32_t rest = std::get<std::int32_t>(value.number);
        if (rest < 0 || rest > 2222)
            return what;
        for (; rest > 0; rest /= 10)
        {
            if (rest % 10 > 2)
                return what;
        }
        return {};
    }
    case ConnectorSearchParamsRule::RealRange:
        return number >= info.low && number <= info.high ? std::string() : what;
    case ConnectorSearchParamsRule::RealRangeOrMinusOne:
        return (number >= info.low && number <= info.high) || number == -1.0 ? std::string() : what;
    case ConnectorSearchParamsRule::AbsoluteRealRange:
        return std::fabs(number) >= info.low && std::fabs(number) <= info.high ? std::string() : what;
    case ConnectorSearchParamsRule::NonNegativeReal:
        return number >= 0.0 ? std::string() : what;
    }
    return {};
}

} // namespace oor::model
