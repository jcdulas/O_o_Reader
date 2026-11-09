#include "oor/model/cards/properties/gapProp/GapPropTable.hpp"

namespace oor::model
{

std::optional<GapPropCard> GapPropFormat::read(FieldReader& reader)
{
    GapPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.initialOpening = reader.real(2, "initialOpening", 0.0);
    card.preload = reader.real(3, "preload", 0.0);
    card.closedStiffness = reader.real(4, "closedStiffness", GapPropCard::DEFAULT_CLOSED_STIFFNESS);
    card.frictionCoeff1 = reader.real(7, "frictionCoeff1", 0.0);
    // The defaults that depend on other fields, stored resolved: KB is 1.0E-14 KA, KT is MU1 KA, MU2 is MU1.
    card.openStiffness = reader.real(5, "openStiffness", 1.0e-14 * card.closedStiffness);
    card.transverseStiffness = reader.real(6, "transverseStiffness", card.frictionCoeff1 * card.closedStiffness);
    card.frictionCoeff2 = reader.real(8, "frictionCoeff2", card.frictionCoeff1);
    card.maxPenetration = reader.real(9, "maxPenetration", 0.0);
    card.maxAdjustmentRatio = reader.real(10, "maxAdjustmentRatio", GapPropCard::DEFAULT_MAX_ADJUSTMENT_RATIO);
    card.minPenetrationRatio = reader.real(11, "minPenetrationRatio", GapPropCard::DEFAULT_MIN_PENETRATION_RATIO);
    if (reader.lastUsedField() > 11)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has eleven fields",
                     "minPenetrationRatio");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GapPropFormat::check(const GapPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.preload < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "preload"};
    if (!(card.closedStiffness > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "closedStiffness"};
    if (card.openStiffness < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "openStiffness"};
    if (card.transverseStiffness < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "transverseStiffness"};
    if (card.frictionCoeff1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "frictionCoeff1"};
    if (card.frictionCoeff2 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "frictionCoeff2"};
    if (card.maxAdjustmentRatio < 1.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1.0",
                           "maxAdjustmentRatio"};
    if (card.minPenetrationRatio < 0.0 || card.minPenetrationRatio > 1.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.0 and 1.0",
                           "minPenetrationRatio"};
    return std::nullopt;
}

void GapPropFormat::write(const GapPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.initialOpening, 0.0);
    writer.realUnlessDefault(3, card.preload, 0.0);
    writer.realUnlessDefault(4, card.closedStiffness, GapPropCard::DEFAULT_CLOSED_STIFFNESS);
    writer.real(5, card.openStiffness); // the defaults of KB, KT and MU2 depend on other fields: always written
    writer.real(6, card.transverseStiffness);
    writer.realUnlessDefault(7, card.frictionCoeff1, 0.0);
    writer.real(8, card.frictionCoeff2);
    writer.realUnlessDefault(9, card.maxPenetration, 0.0);
    writer.realUnlessDefault(10, card.maxAdjustmentRatio, GapPropCard::DEFAULT_MAX_ADJUSTMENT_RATIO);
    writer.realUnlessDefault(11, card.minPenetrationRatio, GapPropCard::DEFAULT_MIN_PENETRATION_RATIO);
}

std::uint32_t GapPropTable::append(const GapPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_initialOpening.push(card.initialOpening);
    m_preload.push(card.preload);
    m_closedStiffness.push(card.closedStiffness);
    m_openStiffness.push(card.openStiffness);
    m_transverseStiffness.push(card.transverseStiffness);
    m_frictionCoeff1.push(card.frictionCoeff1);
    m_frictionCoeff2.push(card.frictionCoeff2);
    m_maxPenetration.push(card.maxPenetration);
    m_maxAdjustmentRatio.push(card.maxAdjustmentRatio);
    m_minPenetrationRatio.push(card.minPenetrationRatio);
    return row;
}

GapPropCard GapPropTable::card(std::uint32_t row) const
{
    GapPropCard card;
    card.id = m_id.get(row);
    card.initialOpening = m_initialOpening.get(row);
    card.preload = m_preload.get(row);
    card.closedStiffness = m_closedStiffness.get(row);
    card.openStiffness = m_openStiffness.get(row);
    card.transverseStiffness = m_transverseStiffness.get(row);
    card.frictionCoeff1 = m_frictionCoeff1.get(row);
    card.frictionCoeff2 = m_frictionCoeff2.get(row);
    card.maxPenetration = m_maxPenetration.get(row);
    card.maxAdjustmentRatio = m_maxAdjustmentRatio.get(row);
    card.minPenetrationRatio = m_minPenetrationRatio.get(row);
    return card;
}

void GapPropTable::store(std::uint32_t row, const GapPropCard& card)
{
    m_id.set(row, card.id);
    m_initialOpening.set(row, card.initialOpening);
    m_preload.set(row, card.preload);
    m_closedStiffness.set(row, card.closedStiffness);
    m_openStiffness.set(row, card.openStiffness);
    m_transverseStiffness.set(row, card.transverseStiffness);
    m_frictionCoeff1.set(row, card.frictionCoeff1);
    m_frictionCoeff2.set(row, card.frictionCoeff2);
    m_maxPenetration.set(row, card.maxPenetration);
    m_maxAdjustmentRatio.set(row, card.maxAdjustmentRatio);
    m_minPenetrationRatio.set(row, card.minPenetrationRatio);
}

void GapPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_initialOpening.keepRows(rows);
    m_preload.keepRows(rows);
    m_closedStiffness.keepRows(rows);
    m_openStiffness.keepRows(rows);
    m_transverseStiffness.keepRows(rows);
    m_frictionCoeff1.keepRows(rows);
    m_frictionCoeff2.keepRows(rows);
    m_maxPenetration.keepRows(rows);
    m_maxAdjustmentRatio.keepRows(rows);
    m_minPenetrationRatio.keepRows(rows);
}

void GapPropTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A PGAP refers to no other card.
}

} // namespace oor::model
