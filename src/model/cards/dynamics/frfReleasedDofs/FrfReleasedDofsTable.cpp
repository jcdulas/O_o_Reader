#include "oor/model/cards/dynamics/frfReleasedDofs/FrfReleasedDofsTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>

namespace oor::model
{

namespace
{

constexpr std::array<FrfPointNames, 3> GROUP_NAMES{
    {{"frfComponent1", "node1"}, {"frfComponent2", "node2"}, {"frfComponent3", "node3"}}};

constexpr int FIRST_GROUP_FIELD = 3; // the groups are fields 3-4, 5-6 and 7-8
constexpr int LAST_FIELD = 8;

} // namespace

std::optional<FrfReleasedDofsCard> FrfReleasedDofsFormat::read(FieldReader& reader)
{
    FrfReleasedDofsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.components = base::sortedDigits(reader.requiredInteger(2, "components"));
    for (std::size_t group = 0; group < card.points.size(); ++group)
        card.points[group] =
            readFrfComponentPoint(reader, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), GROUP_NAMES[group]);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has no field beyond " + std::string("node3"), "node3");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfReleasedDofsFormat::check(const FrfReleasedDofsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // The components that are not connected: the digits 1 to 6, each once (a scalar point has no component to release).
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "components"};
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (const auto problem = checkFrfComponentPoint(card.points[group], GROUP_NAMES[group], group == 0))
            return problem;
    }
    return std::nullopt;
}

void FrfReleasedDofsFormat::write(const FrfReleasedDofsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.components);
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (!card.points[group].isBlank())
            writeFrfComponentPoint(writer, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), card.points[group]);
    }
}

std::uint32_t FrfReleasedDofsTable::append(const FrfReleasedDofsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_components.push(card.components);
    m_cards.push(card);
    return row;
}

void FrfReleasedDofsTable::store(std::uint32_t row, const FrfReleasedDofsCard& card)
{
    m_id.set(row, card.id);
    m_components.set(row, card.components);
    m_cards.set(row, card);
}

void FrfReleasedDofsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_components.keepRows(rows);
    m_cards.keepRows(rows);
}

void FrfReleasedDofsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FrfReleasedDofsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FrfReleasedDofsCard& card = m_cards.edit(row);
    visitFrfComponent(visitor, "frfComponent1", card.points[0].component);
    visitFrfComponent(visitor, "frfComponent2", card.points[1].component);
    visitFrfComponent(visitor, "frfComponent3", card.points[2].component);
}

} // namespace oor::model
