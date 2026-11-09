#pragma once

#include <cstdint>
#include <string>

namespace oor::model
{

enum class ParameterKind : std::uint8_t
{
    Blank,   // PARAM,NAME with no value
    Integer, // PARAM,POST,-1
    Real,    // PARAM,WTMASS,0.00259
    Text,    // PARAM,AUTOSPC,YES
    Complex  // PARAM,name,real,imaginary (V1 and V2)
};

// One PARAM card: a named parameter of the analysis.
//
// Card layout: PARAM  N  V1  V2
// The value is an integer, a real, a word, or (for the few complex parameters) a pair of reals.
struct ParameterCard
{
    std::string name;
    ParameterKind kind = ParameterKind::Blank;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    double imaginaryValue = 0.0;
    std::string textValue;
};

} // namespace oor::model
