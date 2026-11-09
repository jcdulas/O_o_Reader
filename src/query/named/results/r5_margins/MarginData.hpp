#pragma once

#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

// One row of the margins: an element at a subcase and step, judged by one criterion at the place where it is most
// critical.
struct MarginRow
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    double step = std::numeric_limits<double>::quiet_NaN();
    double eigenvalue = std::numeric_limits<double>::quiet_NaN();
    std::int32_t eid = 0;
    std::string card;
    std::int32_t pid = 0;
    std::int32_t mid = 0;
    std::int32_t layer = 0; // 0: none
    std::string location;
    std::string criterion;
    double value = std::numeric_limits<double>::quiet_NaN();
    double allowable = std::numeric_limits<double>::quiet_NaN();
    std::string allowableSource;
    double factor = 1.0;
    double rf = std::numeric_limits<double>::quiet_NaN();
    double ms = std::numeric_limits<double>::quiet_NaN();
    double tension = std::numeric_limits<double>::quiet_NaN();
    double shear = std::numeric_limits<double>::quiet_NaN();
    double tensionAllowable = std::numeric_limits<double>::quiet_NaN();
    double shearAllowable = std::numeric_limits<double>::quiet_NaN();
    Precision precision = Precision::Single;
    std::int16_t provider = -1; // in MarginSet::providers
    std::string note;           // how the value was obtained, besides the provider
    std::string status;         // the status of the table the value was read from
};

struct MarginSet
{
    std::vector<MarginRow> rows;
    std::vector<ProviderInfo> providers;
    std::string criterion;
};

// The margins of the elements for the criterion and the parameters of the call: `criterion`, `allowables`, `factor`,
// `theory`, `exponents` and the selection of the elements.
[[nodiscard]] MarginSet computeMargins(const ResultQueryContext& context);

// The shared columns of the margin table.
[[nodiscard]] RowBlock marginBlock(const ResultQueryContext& context, const std::vector<const MarginRow*>& rows,
                                   const std::vector<std::int16_t>& providers);

// The parameters `margins` and the queries built on it (the summary, the sizing table) share.
[[nodiscard]] std::vector<ParameterInfo> marginParameters();

} // namespace oor::query
