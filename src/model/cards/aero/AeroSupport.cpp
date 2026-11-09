#include "oor/model/cards/aero/AeroSupport.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace oor::model
{

std::optional<CardProblem> checkTrimLabel(std::string_view label, const char* field)
{
    if (std::find(TRIM_RESERVED_LABELS.begin(), TRIM_RESERVED_LABELS.end(), label) != TRIM_RESERVED_LABELS.end())
        return std::nullopt;
    return checkNameWord(label, field);
}

std::vector<double> readRealList(FieldReader& reader, int first, int last, const char* name)
{
    std::vector<double> values;
    const int end = std::min(last, reader.slotCount());
    // The list runs to the last used field of the range; a blank field before it is the embedded blank the format
    // reference forbids (and a value that follows a blank that closes the list would be lost by a program that stops
    // there).
    int lastUsed = 0;
    for (int field = first; field <= end; ++field)
    {
        if (!reader.isBlank(field))
            lastUsed = field;
    }
    for (int field = first; field <= lastUsed; ++field)
    {
        if (reader.isBlank(field))
        {
            reader.error("card.bad-value", "a blank field inside the list of " + std::string(name), name);
            continue;
        }
        values.push_back(reader.requiredReal(field, name));
    }
    return values;
}

int writeRealList(FieldWriter& writer, int first, std::span<const double> values)
{
    int field = first;
    for (const double value : values)
        writer.real(field++, value);
    return field;
}

std::optional<CardProblem> checkRealListFinite(std::span<const double> values, const char* name)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", name};
    }
    return std::nullopt;
}

} // namespace oor::model
