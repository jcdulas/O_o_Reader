#include "oor/model/cards/explicit/vonMisesYield/VonMisesYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_TABLE = 9;
constexpr int FIELD_TYPE = 10;
constexpr int FIELD_RATE_SCALE_TABLE = 11;
constexpr int FIELD_D = 12;
constexpr int FIELD_P = 13;

} // namespace

std::optional<VonMisesYieldCard> VonMisesYieldFormat::read(FieldReader& reader)
{
    VonMisesYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.yieldStress = reader.optionalReal(2, "yieldStress");
    card.hardeningModulus = reader.real(3, "hardeningModulus", 0.0);
    card.tableId = reader.integer(FIELD_TABLE, "tableId", 0);
    card.type =
        readKeyword(reader, FIELD_TYPE, "curveType", VON_MISES_CURVE_TYPE_WORDS, VonMisesYieldCurveType::TrueStrain);
    card.rateScaleTableId = reader.integer(FIELD_RATE_SCALE_TABLE, "rateScaleTableId", 0);
    card.rateCoefficientD = reader.real(FIELD_D, "rateCoefficientD", 0.0);
    card.rateExponentP = reader.real(FIELD_P, "rateExponentP", 0.0);
    requireBlankFields(reader, {4, 5, 6, 7, 8}, FIELD_P);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> VonMisesYieldFormat::check(const VonMisesYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.yieldStress, "yieldStress", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.hardeningModulus, "hardeningModulus", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.tableId, "tableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.rateScaleTableId, "rateScaleTableId"))
        return problem;
    if (const auto problem = checkReal(card.rateCoefficientD, "rateCoefficientD", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.rateExponentP, "rateExponentP", RealRule::NonNegative))
        return problem;
    // The curve comes from the table or from YIELD and EH, not from both.
    if (card.tableId > 0 && card.yieldStress)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank when " +
                               CardProblem::fieldNamed("tableId") + " gives the stress-strain curve",
                           "yieldStress"};
    if (card.tableId == 0 && !card.yieldStress)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required when " + CardProblem::fieldNamed("tableId") + " is blank",
            "yieldStress"};
    // A table of rate scale factors replaces the Cowper-Symonds law.
    if (card.rateScaleTableId > 0 && card.rateCoefficientD != 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank or zero when " +
                               CardProblem::fieldNamed("rateScaleTableId") + " is given",
                           "rateCoefficientD"};
    if (card.rateScaleTableId > 0 && card.rateExponentP != 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be blank or zero when " +
                               CardProblem::fieldNamed("rateScaleTableId") + " is given",
                           "rateExponentP"};
    return std::nullopt;
}

void VonMisesYieldFormat::write(const VonMisesYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.yieldStress)
        writer.real(2, *card.yieldStress);
    writer.realUnlessDefault(3, card.hardeningModulus, 0.0);
    writer.integerUnlessDefault(FIELD_TABLE, card.tableId, 0);
    writeKeywordUnlessDefault(writer, FIELD_TYPE, VON_MISES_CURVE_TYPE_WORDS, card.type,
                              VonMisesYieldCurveType::TrueStrain);
    writer.integerUnlessDefault(FIELD_RATE_SCALE_TABLE, card.rateScaleTableId, 0);
    writer.realUnlessDefault(FIELD_D, card.rateCoefficientD, 0.0);
    writer.realUnlessDefault(FIELD_P, card.rateExponentP, 0.0);
}

std::uint32_t VonMisesYieldTable::append(const VonMisesYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void VonMisesYieldTable::store(std::uint32_t row, const VonMisesYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void VonMisesYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void VonMisesYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void VonMisesYieldTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    VonMisesYieldCard& card = m_cards.edit(row);
    visitId(visitor, "tableId", ReferenceTarget::DynamicTable, card.tableId);
    visitId(visitor, "rateScaleTableId", ReferenceTarget::DynamicTable, card.rateScaleTableId);
}

} // namespace oor::model
