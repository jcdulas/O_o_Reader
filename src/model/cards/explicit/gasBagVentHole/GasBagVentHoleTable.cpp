#include "oor/model/cards/explicit/gasBagVentHole/GasBagVentHoleTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<GasBagVentHoleCard> GasBagVentHoleFormat::read(FieldReader& reader)
{
    GasBagVentHoleCard card;
    card.id = reader.requiredInteger(1, "id");
    card.size = readKeyword(reader, 2, "holeSize", HOLE_SIZE_WORDS, HoleSize::Small);
    card.flow = readKeyword(reader, 4, "flowDirection", FLOW_DIRECTION_WORDS, FlowDirection::Both);
    card.environmentPressure = reader.requiredReal(5, "environmentPressure");
    card.environmentDensity = reader.requiredReal(6, "environmentDensity");
    card.environmentEnergy = reader.requiredReal(7, "environmentEnergy");
    card.environmentSpecificHeat = reader.optionalReal(8, "environmentSpecificHeat");
    card.matId = reader.integer(9, "matId", 0);
    // Field 3 is an empty cell of the grid, and nothing follows field 9.
    requireOnlyFields(reader, {1, 2, 4, 5, 6, 7, 8, 9});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagVentHoleFormat::check(const GasBagVentHoleCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.environmentPressure, "environmentPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.environmentDensity, "environmentDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.environmentEnergy, "environmentEnergy", RealRule::Positive))
        return problem;
    if (const auto problem =
            checkOptionalReal(card.environmentSpecificHeat, "environmentSpecificHeat", RealRule::Positive))
        return problem;
    return checkOptionalId(card.matId, "matId");
}

void GasBagVentHoleFormat::write(const GasBagVentHoleCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, HOLE_SIZE_WORDS, card.size, HoleSize::Small);
    writeKeywordUnlessDefault(writer, 4, FLOW_DIRECTION_WORDS, card.flow, FlowDirection::Both);
    writer.real(5, card.environmentPressure);
    writer.real(6, card.environmentDensity);
    writer.real(7, card.environmentEnergy);
    if (card.environmentSpecificHeat)
        writer.real(8, *card.environmentSpecificHeat);
    writer.integerUnlessDefault(9, card.matId, 0);
}

std::uint32_t GasBagVentHoleTable::append(const GasBagVentHoleCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagVentHoleTable::store(std::uint32_t row, const GasBagVentHoleCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagVentHoleTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagVentHoleTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagVentHoleTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
