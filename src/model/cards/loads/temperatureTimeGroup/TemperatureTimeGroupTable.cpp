#include "oor/model/cards/loads/temperatureTimeGroup/TemperatureTimeGroupTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 2;
// A range goes up and may have an increment (the alternate formats of the format).
constexpr ContactRunRules RULES{true, false};

} // namespace

std::optional<TemperatureTimeGroupCard> TemperatureTimeGroupFormat::read(FieldReader& reader)
{
    TemperatureTimeGroupCard card;
    card.id = reader.requiredInteger(1, "id");
    card.grids = readContactRuns(reader, FIRST_GRID_FIELD, reader.lastUsedField(), "node", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TemperatureTimeGroupFormat::check(const TemperatureTimeGroupCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "node"};
    return checkContactRuns(card.grids, "node", RULES);
}

void TemperatureTimeGroupFormat::write(const TemperatureTimeGroupCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactRuns(writer, FIRST_GRID_FIELD, card.grids);
}

std::uint32_t TemperatureTimeGroupTable::append(const TemperatureTimeGroupCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TemperatureTimeGroupTable::store(std::uint32_t row, const TemperatureTimeGroupCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TemperatureTimeGroupTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TemperatureTimeGroupTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TemperatureTimeGroupTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A single grid point must exist; the two ends of a THRU range may not (the grid points between them need not).
    visitContactRuns(m_cards.edit(row).grids, visitor, "node", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
