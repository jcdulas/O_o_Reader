#include "oor/model/cards/explicit/gasBagHeatTransfer/GasBagHeatTransferTable.hpp"

namespace oor::model
{

std::optional<GasBagHeatTransferCard> GasBagHeatTransferFormat::read(FieldReader& reader)
{
    GasBagHeatTransferCard card;
    card.id = reader.requiredInteger(1, "id");
    card.setId = reader.requiredInteger(2, "setId");
    card.subsurfaceId = reader.integer(3, "subsurfaceId", 0);
    card.modelKind = readRequiredKeyword<HeatTransferType>(reader, 4, "modelKind", HEAT_TRANSFER_WORDS);
    card.modelId = reader.requiredInteger(5, "modelId");
    card.coefficient = readSourcedValue(reader, GAS_BAG_HEAT_TRANSFER_COEFF_LAYOUT);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagHeatTransferFormat::check(const GasBagHeatTransferCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkOptionalId(card.subsurfaceId, "subsurfaceId"))
        return problem;
    if (const auto problem = checkRequiredId(card.modelId, "modelId"))
        return problem;
    if (const auto problem =
            checkSourcedValue(card.coefficient, GAS_BAG_HEAT_TRANSFER_COEFF_LAYOUT, RealRule::NonNegative))
        return problem;
    // The part of the area of the subsurface that is exposed: a fraction.
    if (card.coefficient.source == ValueSource::Constant)
        return checkRealRange(card.coefficient.constant, "coefficient", 0.0, 1.0);
    return std::nullopt;
}

void GasBagHeatTransferFormat::write(const GasBagHeatTransferCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.setId);
    writer.integerUnlessDefault(3, card.subsurfaceId, 0);
    writeKeyword(writer, 4, HEAT_TRANSFER_WORDS, card.modelKind);
    writer.integer(5, card.modelId);
    writeSourcedValue(writer, GAS_BAG_HEAT_TRANSFER_COEFF_LAYOUT, card.coefficient);
}

std::uint32_t GasBagHeatTransferTable::append(const GasBagHeatTransferCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void GasBagHeatTransferTable::store(std::uint32_t row, const GasBagHeatTransferCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void GasBagHeatTransferTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagHeatTransferTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void GasBagHeatTransferTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GasBagHeatTransferCard& card = m_cards.edit(row);
    visitId(visitor, "subsurfaceId", ReferenceTarget::ContactSurface, card.subsurfaceId);
    // The HTRTYPE word is not matched against the kind of card that holds the number: any heat transfer card satisfies
    // it.
    visitId(visitor, "modelId", ReferenceTarget::HeatTransferModel, card.modelId);
    visitSourcedTable(visitor, "coefficient", card.coefficient);
}

} // namespace oor::model
