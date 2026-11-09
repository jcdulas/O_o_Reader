#include "oor/model/cards/dynamics/hybridDamping/HybridDampingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<HybridDampingCard> HybridDampingFormat::read(FieldReader& reader)
{
    HybridDampingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.methodId = reader.requiredInteger(2, "methodId");
    card.modalDampingTableId = reader.requiredInteger(3, "modalDampingTableId");
    card.dampingInStiffness =
        readKeyword(reader, 4, "dampingInStiffness", HYBRID_DAMPING_OPTION_WORDS, HybridDampingOption::No);
    card.printEigenvalues =
        readKeyword(reader, 5, "printEigenvalues", HYBRID_DAMPING_OPTION_WORDS, HybridDampingOption::No);
    requireUnusedBlank(reader, {}, 5);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HybridDampingFormat::check(const HybridDampingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.methodId, "methodId"))
        return problem;
    return checkRequiredId(card.modalDampingTableId, "modalDampingTableId");
}

void HybridDampingFormat::write(const HybridDampingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.methodId);
    writer.integer(3, card.modalDampingTableId);
    writeKeywordUnlessDefault(writer, 4, HYBRID_DAMPING_OPTION_WORDS, card.dampingInStiffness, HybridDampingOption::No);
    writeKeywordUnlessDefault(writer, 5, HYBRID_DAMPING_OPTION_WORDS, card.printEigenvalues, HybridDampingOption::No);
}

std::uint32_t HybridDampingTable::append(const HybridDampingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HybridDampingTable::store(std::uint32_t row, const HybridDampingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HybridDampingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HybridDampingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HybridDampingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    HybridDampingCard& card = m_cards.edit(row);
    visitId(visitor, "methodId", ReferenceTarget::Method, card.methodId);
    // The SDAMP of the format reference is a TABDMP1 (the Case Control SDAMPING also takes a TABLEDi, this field does
    // not).
    visitId(visitor, "modalDampingTableId", ReferenceTarget::HybridDampingRotorOptionsTable, card.modalDampingTableId);
}

} // namespace oor::model
