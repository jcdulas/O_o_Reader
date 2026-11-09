#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace oor::model
{

// The value of a parameter, an integer or a real as the deck wrote it. A parameter that the catalogue says is real
// takes 1 as well as 1.0 (an integer is a real that is whole) and the card keeps what was written.
struct AcousticInfiniteParamsValue
{
    std::variant<std::int32_t, double> number = std::int32_t{0};

    [[nodiscard]] bool isInteger() const noexcept { return std::holds_alternative<std::int32_t>(number); }
    // The value as a real, whichever it was written as.
    [[nodiscard]] double asReal() const noexcept
    {
        return isInteger() ? static_cast<double>(std::get<std::int32_t>(number)) : std::get<double>(number);
    }

    [[nodiscard]] bool operator==(const AcousticInfiniteParamsValue&) const noexcept = default;
};

// A parameter and its value.
struct AcousticInfiniteParamsParameter
{
    std::string name;
    AcousticInfiniteParamsValue value;

    [[nodiscard]] bool operator==(const AcousticInfiniteParamsParameter&) const noexcept = default;
};

// One ACIFPRM card: values for the parameters of the acoustic conjugate infinite elements (the two that the format
// reference lists are in `AcousticInfiniteParamsCatalogue.hpp`: the geometric tolerance TOLER and the first
// identification number of the extra points EPTID1).
//
// Card layout: ACIFPRM  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  PARAM4  VAL4     fields 1 to 8
//                      PARAM5  VAL5  -etc.-                                        fields 9 to 16, and so on
// A pair is a name and a value in the next field. The format draws two pairs and no continuation, but it lists only two
// names, so the card keeps any number of pairs and reads a pair whose two fields are blank as padding. A name the
// catalogue does not list is kept with its value as written (a later release may add names); a listed name takes the
// values the catalogue gives it. Only one ACIFPRM entry is allowed in the Bulk Data (the table reports a
// second one, as for SWLDPRM).
struct AcousticInfiniteParamsCard
{
    std::vector<AcousticInfiniteParamsParameter> parameters;
};

} // namespace oor::model
