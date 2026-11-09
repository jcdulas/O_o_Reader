#include "oor/model/cards/explicit/AirbagGas.hpp"

#include <string>
#include <variant>

namespace oor::model
{

InflatorGasName readInflatorGasName(FieldReader& reader, int field, const char* name)
{
    InflatorGasName value;
    const auto number = reader.integerOrReal(field, name);
    if (!number)
        return value;
    if (const auto* gas = std::get_if<std::int32_t>(&*number))
    {
        if (*gas <= 0)
            reader.error(
                "card.bad-id",
                std::string(name) + " must be a positive integer (an " + std::string("InflatorGas") + ") or a real",
                name);
        else
            value.gas = *gas;
    }
    else
    {
        value.ratio = std::get<double>(*number);
    }
    return value;
}

void writeInflatorGasName(FieldWriter& writer, int field, const InflatorGasName& value)
{
    if (value.ratio)
        writer.real(field, *value.ratio);
    else if (value.gas > 0)
        writer.integer(field, value.gas);
}

std::optional<CardProblem> checkInflatorGasName(const InflatorGasName& value, const char* name)
{
    if (value.ratio && value.gas != 0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is either a ratio or an " +
                               CardProblem::cardNamed("InflatorGas") + ", not both",
                           name};
    if (const auto problem = checkOptionalReal(value.ratio, name, RealRule::Positive))
        return problem;
    return checkOptionalId(value.gas, name);
}

} // namespace oor::model
