#include "oor/model/cards/explicit/couplingHoleToCoupling/CouplingHoleToCouplingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<CouplingHoleToCouplingCard> CouplingHoleToCouplingFormat::read(FieldReader& reader)
{
    CouplingHoleToCouplingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.size = readKeyword(reader, 2, "holeSize", HOLE_SIZE_WORDS, HoleSize::Small);
    card.flow = readKeyword(reader, 4, "flowDirection", FLOW_DIRECTION_WORDS, FlowDirection::Both);
    card.couplingId = reader.requiredInteger(5, "couplingId");
    card.matId = reader.integer(6, "matId", 0);
    // Field 3 is an empty cell of the grid, and nothing follows field 6.
    requireOnlyFields(reader, {1, 2, 4, 5, 6});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CouplingHoleToCouplingFormat::check(const CouplingHoleToCouplingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.couplingId, "couplingId"))
        return problem;
    return checkOptionalId(card.matId, "matId");
}

void CouplingHoleToCouplingFormat::write(const CouplingHoleToCouplingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, HOLE_SIZE_WORDS, card.size, HoleSize::Small);
    writeKeywordUnlessDefault(writer, 4, FLOW_DIRECTION_WORDS, card.flow, FlowDirection::Both);
    writer.integer(5, card.couplingId);
    writer.integerUnlessDefault(6, card.matId, 0);
}

std::uint32_t CouplingHoleToCouplingTable::append(const CouplingHoleToCouplingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CouplingHoleToCouplingTable::store(std::uint32_t row, const CouplingHoleToCouplingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CouplingHoleToCouplingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CouplingHoleToCouplingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CouplingHoleToCouplingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CouplingHoleToCouplingCard& card = m_cards.edit(row);
    visitId(visitor, "couplingId", ReferenceTarget::Coupling, card.couplingId);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
