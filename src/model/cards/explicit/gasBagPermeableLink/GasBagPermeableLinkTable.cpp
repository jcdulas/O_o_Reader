#include "oor/model/cards/explicit/gasBagPermeableLink/GasBagPermeableLinkTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<GasBagPermeableLinkCard> GasBagPermeableLinkFormat::read(FieldReader& reader)
{
    GasBagPermeableLinkCard card;
    card.id = reader.requiredInteger(1, "id");
    card.permeability = reader.optionalReal(2, "permeability");
    card.permeabilityTableId = reader.integer(3, "permeabilityTableId", 0);
    card.flow = readKeyword(reader, 4, "flowDirection", FLOW_DIRECTION_WORDS, FlowDirection::Both);
    card.gasBagId = reader.requiredInteger(5, "gasBagId");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagPermeableLinkFormat::check(const GasBagPermeableLinkCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.permeability, "permeability", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.permeabilityTableId, "permeabilityTableId"))
        return problem;
    // The permeability is a linear law or a table, so one of the two must be there.
    if (!card.permeability && card.permeabilityTableId == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("permeabilityTableId") + " is required",
            "permeability"};
    return checkRequiredId(card.gasBagId, "gasBagId");
}

void GasBagPermeableLinkFormat::write(const GasBagPermeableLinkCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.permeability)
        writer.real(2, *card.permeability);
    writer.integerUnlessDefault(3, card.permeabilityTableId, 0);
    writeKeywordUnlessDefault(writer, 4, FLOW_DIRECTION_WORDS, card.flow, FlowDirection::Both);
    writer.integer(5, card.gasBagId);
}

std::uint32_t GasBagPermeableLinkTable::append(const GasBagPermeableLinkCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagPermeableLinkTable::store(std::uint32_t row, const GasBagPermeableLinkCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagPermeableLinkTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagPermeableLinkTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagPermeableLinkTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GasBagPermeableLinkCard& card = m_cards.edit(row);
    visitId(visitor, "permeabilityTableId", ReferenceTarget::DynamicTable, card.permeabilityTableId);
    visitId(visitor, "gasBagId", ReferenceTarget::GasBag, card.gasBagId);
}

} // namespace oor::model
