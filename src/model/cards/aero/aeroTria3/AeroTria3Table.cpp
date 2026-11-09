#include "oor/model/cards/aero/aeroTria3/AeroTria3Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;
constexpr std::size_t GRID_COUNT = 3;
constexpr std::array<const char*, GRID_COUNT> GRID_NAMES{"node1", "node2", "node3"};

} // namespace

std::optional<AeroTria3Card> AeroTria3Format::read(FieldReader& reader)
{
    AeroTria3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.componentId = reader.requiredInteger(2, "componentId");
    card.node1 = reader.requiredInteger(3, "node1");
    card.node2 = reader.requiredInteger(4, "node2");
    card.node3 = reader.requiredInteger(5, "node3");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has five fields: {field:id}, {field:componentId}, {field:node1}, {field:node2} "
                                      "and {field:node3}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroTria3Format::check(const AeroTria3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.componentId, "componentId"))
        return problem;
    const std::array<base::EntityId, GRID_COUNT> grids{card.node1, card.node2, card.node3};
    for (std::size_t index = 0; index < GRID_COUNT; ++index)
    {
        if (const auto problem = checkRequiredId(grids[index], GRID_NAMES[index]))
            return problem;
    }
    // Not in the format reference (it says it for the AEQUAD4 only): a triangle with a repeated point has no area.
    for (std::size_t later = 1; later < GRID_COUNT; ++later)
    {
        for (std::size_t earlier = 0; earlier < later; ++earlier)
        {
            if (grids[earlier] == grids[later])
                return CardProblem{"card.bad-value",
                                   CardProblem::fieldNamed(GRID_NAMES[earlier]) + " and {field}" +
                                       " name the same grid point: the three points of the card are different",
                                   GRID_NAMES[later]};
        }
    }
    return std::nullopt;
}

void AeroTria3Format::write(const AeroTria3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.componentId);
    writer.integer(3, card.node1);
    writer.integer(4, card.node2);
    writer.integer(5, card.node3);
}

std::uint32_t AeroTria3Table::append(const AeroTria3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroTria3Table::store(std::uint32_t row, const AeroTria3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroTria3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroTria3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroTria3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // CMPID is a component number that no card owns, so it is not visited.
    AeroTria3Card& card = m_cards.edit(row);
    visitId(visitor, "node1", ReferenceTarget::AeroNode, card.node1);
    visitId(visitor, "node2", ReferenceTarget::AeroNode, card.node2);
    visitId(visitor, "node3", ReferenceTarget::AeroNode, card.node3);
}

} // namespace oor::model
