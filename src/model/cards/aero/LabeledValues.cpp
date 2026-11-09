#include "oor/model/cards/aero/LabeledValues.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

std::vector<LabeledValue> readLabeledValues(FieldReader& reader, int firstField, const char* labelName,
                                            const char* valueName)
{
    std::vector<LabeledValue> values;
    const int last = reader.lastUsedField();
    for (int field = firstField; field <= last; field += 2)
    {
        const bool noLabel = reader.isBlank(field);
        const bool noValue = reader.isBlank(field + 1);
        if (noLabel && noValue)
            continue;
        if (noLabel)
        {
            reader.error("card.missing-field", "a value is given without its " + std::string(labelName), labelName);
            continue;
        }
        values.push_back(LabeledValue{std::string(reader.text(field)), reader.requiredReal(field + 1, valueName)});
    }
    return values;
}

int writeLabeledValues(FieldWriter& writer, int firstField, std::span<const LabeledValue> values)
{
    int field = firstField;
    for (const LabeledValue& item : values)
    {
        writer.text(field, item.label);
        writer.real(field + 1, item.value);
        field += 2;
    }
    return field;
}

std::optional<CardProblem> checkLabeledValues(std::span<const LabeledValue> values, const char* labelName,
                                              const char* valueName)
{
    for (const LabeledValue& item : values)
    {
        if (const auto problem = checkTrimLabel(item.label, labelName))
            return problem;
        if (!std::isfinite(item.value))
            return CardProblem{"card.bad-value", "the value of " + item.label + " must be a finite number", valueName};
    }
    return std::nullopt;
}

} // namespace oor::model
