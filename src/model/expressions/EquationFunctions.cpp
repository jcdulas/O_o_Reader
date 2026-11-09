#include "oor/model/expressions/EquationFunctions.hpp"

namespace oor::model
{

std::optional<EquationFunction> findEquationFunction(std::string_view name) noexcept
{
    for (std::size_t index = 0; index < EQUATION_FUNCTIONS.size(); ++index)
    {
        if (EQUATION_FUNCTIONS[index].name == name)
            return static_cast<EquationFunction>(index);
    }
    return std::nullopt;
}

} // namespace oor::model
