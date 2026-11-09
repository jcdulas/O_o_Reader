#include "oor/model/cards/properties/compPropFreqDamping/CompPropFreqDampingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

namespace oor::model
{

std::optional<CompPropFreqDampingCard> CompPropFreqDampingFormat::read(FieldReader& reader)
{
    CompPropFreqDampingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.structuralDamping = reader.integer(7, "structuralDamping", 0); // the same column as the GE of a PCOMP
    requireUnusedFields(reader, {2, 3, 4, 5, 6, 8});
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has a " + std::string("id") + " and a " +
                         std::string("structuralDamping") + " only",
                     "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CompPropFreqDampingFormat::check(const CompPropFreqDampingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkOptionalId(card.structuralDamping, "structuralDamping");
}

void CompPropFreqDampingFormat::write(const CompPropFreqDampingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(7, card.structuralDamping, 0);
}

std::uint32_t CompPropFreqDampingTable::append(const CompPropFreqDampingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_structuralDamping.push(card.structuralDamping);
    return row;
}

CompPropFreqDampingCard CompPropFreqDampingTable::card(std::uint32_t row) const
{
    return CompPropFreqDampingCard{m_id.get(row), m_structuralDamping.get(row)};
}

void CompPropFreqDampingTable::store(std::uint32_t row, const CompPropFreqDampingCard& card)
{
    m_id.set(row, card.id);
    m_structuralDamping.set(row, card.structuralDamping);
}

void CompPropFreqDampingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_structuralDamping.keepRows(rows);
}

void CompPropFreqDampingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PCOMP or PCOMPG that the card extends (the two share the property space).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Property, m_id.get(row)));
    if (const base::EntityId structuralDamping = m_structuralDamping.get(row); structuralDamping > 0)
        m_structuralDamping.set(row,
                                visitor.visit("structuralDamping", ReferenceTarget::DynamicTable, structuralDamping));
}

} // namespace oor::model
