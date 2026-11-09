#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The flags of the format reference: what is randomized. PENTRY: the real values of all the element property entries;
// MENTRY: of all the independent material property entries; CENTRY: of all the connectivity entries; LOADS: of all the
// load entries; SPCD: of all the SPCD entries.
enum class StochasticParamsFlag : std::uint8_t
{
    Property,
    Material,
    Connectivity,
    Loads,
    Spcd
};

// The flag words, in the order of the enum. The last is the neutral name of a card (the enforced motions).
inline constexpr std::array<std::string_view, 5> STOCHASTIC_PARAMS_FLAG_WORDS{
    "PropertyEntries", "MaterialEntries", "ConnectivityEntries", "LoadEntries", "EnforcedMotionConstr"};

inline constexpr double STOCHASTIC_PARAMS_DEFAULT_VARIATION_COEFFICIENT = 0.05;
inline constexpr double STOCHASTIC_PARAMS_DEFAULT_STANDARD_DEVIATIONS = 3.0;

// The statistics of what one flag randomizes.
struct StochasticParamsGroup
{
    StochasticParamsFlag flag = StochasticParamsFlag::Property;
    std::string distribution; // CDF: the name of the distribution, blank (empty) for the default
    double variationCoefficient =
        STOCHASTIC_PARAMS_DEFAULT_VARIATION_COEFFICIENT; // CoV: the coefficient of variance, greater than 0
    double standardDeviations =
        STOCHASTIC_PARAMS_DEFAULT_STANDARD_DEVIATIONS; // m: the number of standard deviations, greater than 0

    [[nodiscard]] bool operator==(const StochasticParamsGroup&) const noexcept = default;
};

// One STOCHAS card: the statistics used to randomize selected model parameters. The Case Control STOCHASTICS selects it
// by SID (that command is kept as text, so the SID is not resolved).
//
// Card layout: STOCHAS  SID  PENTRY  CDF  CoV  m
//                           MENTRY  CDF  CoV  m
//                           CENTRY  CDF  CoV  m
//                           LOADS   CDF  CoV  m
//                           SPCD    CDF  CoV  m
// In this library's numbering (field 1 is SID) each group is a line of eight fields: its flag is the second field of
// the line, CDF, CoV and m the next three; the groups after the first are on continuation lines, whose first field (the
// field before the flag) is blank, as the layout has them. At least one flag must exist and they can be in any order;
// a flag that appears twice is refused. Only Gaussian distributions are supported, and the default of CDF is named
// with nine characters, which no field holds (GAUSS is the usual short form): a blank CDF is the default and is kept
// blank, and any other name of up to 16 characters is kept as written (upper case). CoV
// defaults to 0.05 and m to 3.0, which are static defaults: they are stored resolved and written as blanks. The product
// of m and CoV must be less than 1.0.
struct StochasticParamsCard
{
    base::EntityId id = 0; // SID
    std::vector<StochasticParamsGroup> groups;

    [[nodiscard]] bool operator==(const StochasticParamsCard&) const noexcept = default;
};

} // namespace oor::model
