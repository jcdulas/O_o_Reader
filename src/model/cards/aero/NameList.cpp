#include "oor/model/cards/aero/NameList.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::vector<std::string> readNameList(FieldReader& reader, int first, const char* name)
{
    std::vector<std::string> names;
    const int last = reader.lastUsedField();
    for (int field = first; field <= last; ++field)
    {
        if (reader.isBlank(field))
        {
            reader.error("card.bad-value", "a blank field inside the list of " + std::string(name), name);
            continue;
        }
        names.emplace_back(reader.text(field));
    }
    return names;
}

int writeNameList(FieldWriter& writer, int first, std::span<const std::string> names)
{
    int field = first;
    for (const std::string& name : names)
        writer.text(field++, name);
    return field;
}

std::optional<CardProblem> checkNameList(std::span<const std::string> names, const char* name)
{
    if (names.empty())
        return CardProblem{"card.missing-field", "at least one name is required ({field})", name};
    for (const std::string& item : names)
    {
        if (const auto problem = checkNameWord(item, name))
            return problem;
    }
    return std::nullopt;
}

} // namespace oor::model
