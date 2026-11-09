#include "oor/model/cards/dynamics/frfConnection/FrfConnectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>

namespace oor::model
{

namespace
{

constexpr std::array<FrfPointNames, 2> GROUP_NAMES{{{"frfComponent1", "point1"}, {"frfComponent2", "point2"}}};

constexpr int FIRST_GROUP_FIELD = 2; // the groups are fields 2-3 and 4-5

} // namespace

std::optional<FrfConnectionCard> FrfConnectionFormat::read(FieldReader& reader)
{
    FrfConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t group = 0; group < card.points.size(); ++group)
        card.points[group] =
            readFrfComponentPoint(reader, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), GROUP_NAMES[group]);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("point2"), "point2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfConnectionFormat::check(const FrfConnectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (const auto problem = checkFrfComponentPoint(card.points[group], GROUP_NAMES[group], true))
            return problem;
    }
    return std::nullopt;
}

void FrfConnectionFormat::write(const FrfConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t group = 0; group < card.points.size(); ++group)
        writeFrfComponentPoint(writer, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), card.points[group]);
}

std::uint32_t FrfConnectionTable::append(const FrfConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FrfConnectionTable::store(std::uint32_t row, const FrfConnectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FrfConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FrfConnectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FrfConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FrfConnectionCard& card = m_cards.edit(row);
    visitFrfComponent(visitor, "frfComponent1", card.points[0].component);
    visitFrfComponent(visitor, "frfComponent2", card.points[1].component);
}

} // namespace oor::model
