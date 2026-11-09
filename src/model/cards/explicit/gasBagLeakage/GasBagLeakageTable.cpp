#include "oor/model/cards/explicit/gasBagLeakage/GasBagLeakageTable.hpp"

namespace oor::model
{

std::optional<GasBagLeakageCard> GasBagLeakageFormat::read(FieldReader& reader)
{
    GasBagLeakageCard card;
    card.id = reader.requiredInteger(1, "id");
    card.setId = reader.requiredInteger(2, "setId");
    card.subsurfaceId = reader.integer(3, "subsurfaceId", 0);
    card.porosityKind = readRequiredKeyword<PorosityType>(reader, 4, "porosityKind", POROSITY_TYPE_WORDS);
    card.porosityModelId = reader.requiredInteger(5, "porosityModelId");
    card.coefficient = readSourcedValue(reader, GAS_BAG_LEAKAGE_COEFF_LAYOUT);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagLeakageFormat::check(const GasBagLeakageCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkOptionalId(card.subsurfaceId, "subsurfaceId"))
        return problem;
    if (const auto problem = checkRequiredId(card.porosityModelId, "porosityModelId"))
        return problem;
    if (const auto problem = checkSourcedValue(card.coefficient, GAS_BAG_LEAKAGE_COEFF_LAYOUT, RealRule::NonNegative))
        return problem;
    // The part of the area that is open: a fraction.
    if (card.coefficient.source == ValueSource::Constant)
        return checkRealRange(card.coefficient.constant, "coefficient", 0.0, 1.0);
    return std::nullopt;
}

void GasBagLeakageFormat::write(const GasBagLeakageCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.setId);
    writer.integerUnlessDefault(3, card.subsurfaceId, 0);
    writeKeyword(writer, 4, POROSITY_TYPE_WORDS, card.porosityKind);
    writer.integer(5, card.porosityModelId);
    writeSourcedValue(writer, GAS_BAG_LEAKAGE_COEFF_LAYOUT, card.coefficient);
}

std::uint32_t GasBagLeakageTable::append(const GasBagLeakageCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void GasBagLeakageTable::store(std::uint32_t row, const GasBagLeakageCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void GasBagLeakageTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagLeakageTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void GasBagLeakageTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GasBagLeakageCard& card = m_cards.edit(row);
    visitId(visitor, "subsurfaceId", ReferenceTarget::ContactSurface, card.subsurfaceId);
    // The PORTYPE word is not matched against the kind of card that holds the number: any porosity card satisfies it.
    visitId(visitor, "porosityModelId", ReferenceTarget::Porosity, card.porosityModelId);
    visitSourcedTable(visitor, "coefficient", card.coefficient);
}

} // namespace oor::model
