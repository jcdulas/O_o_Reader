#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// The kind of the value of a model parameter: the integer, the real or the word that the card gave.
enum class ModelParameterKind : std::uint8_t
{
    Integer, // MDLPRM  HDF5  1
    Real,    // MDLPRM  PRCNT  0.5
    Text     // MDLPRM  OFFDEF  LROFF
};

// One name and value of an MDLPRM card.
struct ModelParameter
{
    // word, upper case.
    std::string name;
    ModelParameterKind kind = ModelParameterKind::Integer;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    std::string textValue;

    [[nodiscard]] bool operator==(const ModelParameter&) const noexcept = default;
};

// One MDLPRM card: parameters that affect the solution of the structural model.
//
// Card layout: MDLPRM  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  -etc.-
// The pairs follow each other from field 1 over as many continuation lines as needed (four to a line). The format
// reference lists the allowed names with their types; this card does not check the names (a name it
// does not know is kept) and keeps each value as the kind it was written in. A name appears once on a card.
struct ModelParamsCard
{
    std::vector<ModelParameter> parameters;
};

} // namespace oor::model
