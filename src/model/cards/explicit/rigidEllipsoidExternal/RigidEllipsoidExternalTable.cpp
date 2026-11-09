#include "oor/model/cards/explicit/rigidEllipsoidExternal/RigidEllipsoidExternalTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/ExternalProgramNames.hpp"
#include "oor/model/core/Model.hpp"

#include <set>

namespace oor::model
{

std::optional<RigidEllipsoidExternalCard> RigidEllipsoidExternalFormat::read(FieldReader& reader)
{
    RigidEllipsoidExternalCard card;
    card.name = readExternalName(reader, 1, "ellipsoidName");
    card.programName = readExternalName(reader, 2, "programName");
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has two fields: " + std::string("ellipsoidName") +
                         " and " + std::string("programName"),
                     "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidEllipsoidExternalFormat::check(const RigidEllipsoidExternalCard& card)
{
    if (const auto problem = checkNameWord(card.name, "ellipsoidName"))
        return problem;
    return checkNameWord(card.programName, "programName");
}

void RigidEllipsoidExternalFormat::write(const RigidEllipsoidExternalCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.text(2, card.programName);
}

std::uint32_t RigidEllipsoidExternalTable::append(const RigidEllipsoidExternalCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void RigidEllipsoidExternalTable::store(std::uint32_t row, const RigidEllipsoidExternalCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void RigidEllipsoidExternalTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidEllipsoidExternalTable::finishRead(Model&, const ReadContext& context)
{
    std::set<std::string> names;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        const std::string name = m_name.get(row);
        if (!names.insert(name).second && reported.insert(name).second)
            context.sink.error("deck.duplicate-rigid-ellipsoid-external",
                               "the " + std::string(NAME) + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
