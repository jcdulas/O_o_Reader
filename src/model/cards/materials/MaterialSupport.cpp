#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

std::optional<CardProblem> checkMaterialId(base::EntityId id)
{
    if (id <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "id"};
    return std::nullopt;
}

std::optional<CardProblem> checkTableReference(base::EntityId id, const char* field, bool allowNegative)
{
    if (id < 0 && !allowNegative)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be a table ID (a positive integer) or blank",
                           field};
    return std::nullopt;
}

void requireBlankFields(FieldReader& reader, std::initializer_list<int> fields, int lastField)
{
    for (const int field : fields)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return;
        }
    }
    if (reader.lastUsedField() > lastField)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has " + std::to_string(lastField) + " fields", "");
}

void requireOnlyFields(FieldReader& reader, const std::vector<int>& usedFields)
{
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last; ++field)
    {
        if (reader.isBlank(field) || std::find(usedFields.begin(), usedFields.end(), field) != usedFields.end())
            continue;
        reader.error("card.bad-value",
                     "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                         " is not used by this form of the card and must be blank",
                     "");
        return;
    }
}

std::string fieldNameList(std::initializer_list<const char*> names)
{
    std::string list;
    std::size_t index = 0;
    for (const char* name : names)
    {
        if (index > 0)
            list += index + 1 == names.size() ? " or " : ", ";
        list += std::string(name);
        ++index;
    }
    return list;
}

} // namespace oor::model
