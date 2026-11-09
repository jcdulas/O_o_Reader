#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsCard.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// The kind of value a parameter takes.
enum class AcousticInfiniteParamsKind : std::uint8_t
{
    Integer,
    Real
};

// One parameter of ACIFPRM: its name, the kind of value, the default the solver
// uses when no ACIFPRM gives it and what the value must be (a phrase for the messages).
struct AcousticInfiniteParamsInfo
{
    std::string_view name; // the neutral name (FieldPointInsideTolerance for TOLER)
    AcousticInfiniteParamsKind kind;
    double defaultValue;
    std::string_view values;
    std::string_view note;
};

// The two parameters of ACIFPRM, typed by hand.
inline constexpr std::array<AcousticInfiniteParamsInfo, 2> ACOUSTIC_INFINITE_PARAMS_CATALOGUE{{
    {"FieldPointInsideTolerance", AcousticInfiniteParamsKind::Real, 0.001, "a real greater than 0",
     "the geometric tolerance that tells whether a field point is inside an infinite element"},
    {"FirstExtraPointId", AcousticInfiniteParamsKind::Integer, 200000000.0, "an integer greater than 0",
     "the first number given to the extra points the solver adds"},
}};

// The entry of a neutral name; null for a name the table does not list (a card may use such a name).
[[nodiscard]] inline const AcousticInfiniteParamsInfo* findAcousticInfiniteParam(std::string_view name) noexcept
{
    for (const AcousticInfiniteParamsInfo& info : ACOUSTIC_INFINITE_PARAMS_CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

// The name of a kind: "integer" or "real".
[[nodiscard]] inline std::string_view acousticInfiniteParamsKindName(AcousticInfiniteParamsKind kind) noexcept
{
    return kind == AcousticInfiniteParamsKind::Integer ? "integer" : "real";
}

[[nodiscard]] inline std::string acousticInfiniteParamsValueProblem(const AcousticInfiniteParamsInfo& info,
                                                                    const AcousticInfiniteParamsValue& value)
{
    const std::string what = std::string(info.name) + " must be " + std::string(info.values);
    if (info.kind == AcousticInfiniteParamsKind::Integer && !value.isInteger())
        return what + " (an integer was expected)";
    const double number = value.asReal();
    return std::isfinite(number) && number > 0.0 ? std::string() : what;
}

} // namespace oor::model
