#include "oor/model/cards/explicit/InitialVariables.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cmath>
#include <utility>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 4> RADIAL_CONSTANTS{"CenterX", "CenterY", "CenterZ", "Decay"};

// A value that holds the ID of a table: a whole number from 1 to the largest ID.
[[nodiscard]] bool isTableId(double value) noexcept
{
    return std::isfinite(value) && value >= 1.0 && value <= static_cast<double>(base::MAX_ENTITY_ID) &&
           value == std::floor(value);
}

} // namespace

bool isRadialConstant(std::string_view name) noexcept
{
    for (const std::string_view constant : RADIAL_CONSTANTS)
    {
        if ((constant == name))
            return true;
    }
    return false;
}

bool holdsTable(InitialValueMode mode, std::string_view name) noexcept
{
    return mode == InitialValueMode::RadialProfiles && !isRadialConstant(name);
}

std::vector<InitialVariable> readInitialVariables(FieldReader& reader, int firstField, InitialValueMode mode)
{
    std::vector<InitialVariable> variables;
    bool ended = false; // a pair with no name and no value was seen
    for (int field = firstField; field <= reader.lastUsedField() && !reader.failed(); field += 2)
    {
        const bool blankName = reader.isBlank(field);
        const bool blankValue = reader.isBlank(field + 1);
        if (blankName && blankValue)
        {
            ended = true;
            continue;
        }
        if (ended)
        {
            reader.error("card.bad-value", "a blank field ends the list of variables: nothing may follow it",
                         "variableName");
            break;
        }
        if (blankName)
        {
            reader.error("card.missing-field", "a value has no variable name", "variableName");
            break;
        }
        // The format numbers the pairs NAMEi and VALUEi; the diagnostics say the field of the pair that failed.
        InitialVariable variable;
        variable.name = std::string(reader.text(field));
        if (holdsTable(mode, variable.name))
            variable.value = static_cast<double>(reader.requiredInteger(field + 1, "variableValue"));
        else
            variable.value = reader.requiredReal(field + 1, "variableValue");
        variables.push_back(std::move(variable));
    }
    return variables;
}

void writeInitialVariables(FieldWriter& writer, int firstField, InitialValueMode mode,
                           const std::vector<InitialVariable>& variables)
{
    int field = firstField;
    for (const InitialVariable& variable : variables)
    {
        writer.text(field, variable.name);
        if (holdsTable(mode, variable.name))
            writer.integer(field + 1, static_cast<std::int64_t>(variable.value));
        else
            writer.real(field + 1, variable.value);
        field += 2;
    }
}

std::optional<CardProblem> checkInitialVariables(const std::vector<InitialVariable>& variables, InitialValueMode mode)
{
    if (variables.empty())
        return CardProblem{"card.missing-field", "at least one variable and its value are required", "variableName"};
    for (const InitialVariable& variable : variables)
    {
        if (const auto problem = checkWordOrName(variable.name, "variableName"))
            return problem;
        if (holdsTable(mode, variable.name))
        {
            if (!isTableId(variable.value))
                return CardProblem{"card.bad-value",
                                   "the value of " + variable.name + " must be the ID of a " +
                                       CardProblem::cardNamed("TableDynamic1") + " in a radial list",
                                   "variableValue"};
        }
        else if (const auto problem = checkReal(variable.value, "variableValue", RealRule::Finite))
        {
            return problem;
        }
    }
    return std::nullopt;
}

void visitInitialTables(std::vector<InitialVariable>& variables, InitialValueMode mode, ReferenceVisitor& visitor)
{
    for (InitialVariable& variable : variables)
    {
        if (!holdsTable(mode, variable.name) || !isTableId(variable.value))
            continue;
        const base::EntityId table =
            visitor.visit("variableValue", ReferenceTarget::DynamicTable, static_cast<base::EntityId>(variable.value));
        variable.value = static_cast<double>(table);
    }
}

} // namespace oor::model
