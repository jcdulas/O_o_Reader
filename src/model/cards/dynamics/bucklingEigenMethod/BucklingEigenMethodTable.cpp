#include "oor/model/cards/dynamics/bucklingEigenMethod/BucklingEigenMethodTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int NORM_FIELD = 9;

BucklingMethod readMethod(FieldReader& reader)
{
    const std::string_view text = reader.text(2);
    if (text.empty())
        return BucklingMethod::NotGiven;
    if ((text == "InverseIteration"))
        return BucklingMethod::Inv;
    if ((text == "SturmInverse"))
        return BucklingMethod::Sinv;
    reader.error("card.bad-value", std::string("method") + " must be INV or SINV, found \"" + std::string(text) + "\"",
                 "method");
    return BucklingMethod::NotGiven;
}

const char* methodWord(BucklingMethod method)
{
    switch (method)
    {
    case BucklingMethod::Inv:
        return "InverseIteration";
    case BucklingMethod::Sinv:
        return "SturmInverse";
    case BucklingMethod::NotGiven:
        break;
    }
    return "";
}

BucklingNorm readNorm(FieldReader& reader)
{
    const std::string_view text = reader.text(NORM_FIELD);
    if (text.empty() || (text == "Maximum"))
        return BucklingNorm::Max;
    if ((text == "Point"))
        return BucklingNorm::Point;
    reader.error("card.bad-value", std::string("norm") + " must be MAX or POINT, found \"" + std::string(text) + "\"",
                 "norm");
    return BucklingNorm::Max;
}

} // namespace

std::optional<BucklingEigenMethodCard> BucklingEigenMethodFormat::read(FieldReader& reader)
{
    BucklingEigenMethodCard card;
    card.id = reader.requiredInteger(1, "id");
    card.method = readMethod(reader);
    card.lowerEigenvalue = reader.requiredReal(3, "lowerEigenvalue");
    card.upperEigenvalue = reader.requiredReal(4, "upperEigenvalue");
    card.estimatedModeCount = reader.integer(5, "estimatedModeCount", 0);
    // A blank NDP or NDN is 3 * NEP (format reference): stored resolved.
    card.desiredPositiveModeCount = reader.integer(6, "desiredPositiveModeCount", 3 * card.estimatedModeCount);
    card.desiredNegativeModeCount = reader.integer(7, "desiredNegativeModeCount", 3 * card.estimatedModeCount);
    if (!reader.isBlank(8))
        reader.error("card.bad-value", "field 8 of the card is not used", "desiredNegativeModeCount");
    card.norm = readNorm(reader);
    card.normNode = reader.integer(10, "normNode", 0);
    card.normComponent = reader.integer(11, "normComponent", 0);
    if (reader.lastUsedField() > 11)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("normComponent"), "normComponent");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BucklingEigenMethodFormat::check(const BucklingEigenMethodCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the method ID must be a positive integer", "id"};
    if (!std::isfinite(card.lowerEigenvalue) || !std::isfinite(card.upperEigenvalue) ||
        !(card.lowerEigenvalue < card.upperEigenvalue))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("upperEigenvalue"),
            "lowerEigenvalue"};
    if (card.estimatedModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "estimatedModeCount"};
    if (card.desiredPositiveModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "desiredPositiveModeCount"};
    if (card.desiredNegativeModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "desiredNegativeModeCount"};
    if (card.normNode < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "normNode"};
    if (card.norm == BucklingNorm::Point && card.normNode == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required when " + CardProblem::fieldNamed("norm") + " is POINT",
            "normNode"};
    if (card.normComponent < 0 || card.normComponent > 6)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a component from 1 to 6, or blank",
                           "normComponent"};
    return std::nullopt;
}

void BucklingEigenMethodFormat::write(const BucklingEigenMethodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.method != BucklingMethod::NotGiven)
        writer.text(2, methodWord(card.method));
    writer.real(3, card.lowerEigenvalue);
    writer.real(4, card.upperEigenvalue);
    // NDP and NDN depend on NEP when blank: they are written whenever they have a value.
    if (card.estimatedModeCount != 0)
        writer.integer(5, card.estimatedModeCount);
    if (card.desiredPositiveModeCount != 0)
        writer.integer(6, card.desiredPositiveModeCount);
    if (card.desiredNegativeModeCount != 0)
        writer.integer(7, card.desiredNegativeModeCount);
    writer.textUnlessDefault(NORM_FIELD, card.norm == BucklingNorm::Point ? "Point" : "Maximum", "Maximum");
    if (card.normNode != 0)
        writer.integer(10, card.normNode);
    if (card.normComponent != 0)
        writer.integer(11, card.normComponent);
}

void BucklingEigenMethodTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // G is a grid or a scalar point, which may be named without an SPOINT.
    if (m_normNode.get(row) != 0)
        m_normNode.set(row, visitor.visit("normNode", ReferenceTarget::ScalarOrGridPoint, m_normNode.get(row)));
}

std::uint32_t BucklingEigenMethodTable::append(const BucklingEigenMethodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_lowerEigenvalue.push(card.lowerEigenvalue);
    m_upperEigenvalue.push(card.upperEigenvalue);
    m_estimatedModeCount.push(card.estimatedModeCount);
    m_desiredPositiveModeCount.push(card.desiredPositiveModeCount);
    m_desiredNegativeModeCount.push(card.desiredNegativeModeCount);
    m_norm.push(static_cast<std::uint8_t>(card.norm));
    m_normNode.push(card.normNode);
    m_normComponent.push(card.normComponent);
    return row;
}

BucklingEigenMethodCard BucklingEigenMethodTable::card(std::uint32_t row) const
{
    BucklingEigenMethodCard card;
    card.id = m_id.get(row);
    card.method = static_cast<BucklingMethod>(m_method.get(row));
    card.lowerEigenvalue = m_lowerEigenvalue.get(row);
    card.upperEigenvalue = m_upperEigenvalue.get(row);
    card.estimatedModeCount = m_estimatedModeCount.get(row);
    card.desiredPositiveModeCount = m_desiredPositiveModeCount.get(row);
    card.desiredNegativeModeCount = m_desiredNegativeModeCount.get(row);
    card.norm = static_cast<BucklingNorm>(m_norm.get(row));
    card.normNode = m_normNode.get(row);
    card.normComponent = m_normComponent.get(row);
    return card;
}

void BucklingEigenMethodTable::store(std::uint32_t row, const BucklingEigenMethodCard& card)
{
    m_id.set(row, card.id);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_lowerEigenvalue.set(row, card.lowerEigenvalue);
    m_upperEigenvalue.set(row, card.upperEigenvalue);
    m_estimatedModeCount.set(row, card.estimatedModeCount);
    m_desiredPositiveModeCount.set(row, card.desiredPositiveModeCount);
    m_desiredNegativeModeCount.set(row, card.desiredNegativeModeCount);
    m_norm.set(row, static_cast<std::uint8_t>(card.norm));
    m_normNode.set(row, card.normNode);
    m_normComponent.set(row, card.normComponent);
}

void BucklingEigenMethodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_method.keepRows(rows);
    m_lowerEigenvalue.keepRows(rows);
    m_upperEigenvalue.keepRows(rows);
    m_estimatedModeCount.keepRows(rows);
    m_desiredPositiveModeCount.keepRows(rows);
    m_desiredNegativeModeCount.keepRows(rows);
    m_norm.keepRows(rows);
    m_normNode.keepRows(rows);
    m_normComponent.keepRows(rows);
}

} // namespace oor::model
