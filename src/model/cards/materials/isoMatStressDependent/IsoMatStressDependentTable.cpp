#include "oor/model/cards/materials/isoMatStressDependent/IsoMatStressDependentTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> optionalOf(double value)
{
    return std::isnan(value) ? std::nullopt : std::optional<double>(value);
}

} // namespace

const char* IsoMatStressDependentFormat::typeWord(IsoMatStressDependentType type) noexcept
{
    return type == IsoMatStressDependentType::Plastic ? "Plastic" : "NonlinearElastic";
}

std::optional<IsoMatStressDependentType> IsoMatStressDependentFormat::parseType(std::string_view text) noexcept
{
    if ((text == "NonlinearElastic"))
        return IsoMatStressDependentType::NonlinearElastic;
    if ((text == "Plastic"))
        return IsoMatStressDependentType::Plastic;
    return std::nullopt;
}

IsoMatStressDependentTable::IsoMatStressDependentTable()
    : m_h(notGiven()),
      m_yieldFunction(1),
      m_hardeningRule(1),
      m_initialYieldStress(notGiven()),
      m_frictionAngle(notGiven())
{
}

std::optional<IsoMatStressDependentCard> IsoMatStressDependentFormat::read(FieldReader& reader)
{
    IsoMatStressDependentCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tableId = reader.integer(2, "tableId", 0);
    const std::string_view word = reader.text(3);
    if (word.empty())
    {
        reader.error("card.missing-field", std::string("kind") + " is required: NLELAST or PLASTIC", "kind");
    }
    else if (const auto type = parseType(word))
    {
        card.kind = *type;
    }
    else
    {
        reader.error("card.bad-value",
                     std::string("kind") + " must be NLELAST or PLASTIC, found \"" + std::string(word) + "\"", "kind");
    }
    card.hardeningSlope = reader.optionalReal(4, "hardeningSlope");
    card.yieldFunction = reader.integer(5, "yieldFunction", 1);
    card.hardeningRule = reader.integer(6, "hardeningRule", 1);
    card.initialYieldStress = reader.optionalReal(7, "initialYieldStress");
    card.frictionAngle = reader.optionalReal(8, "frictionAngle");
    requireBlankFields(reader, {}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IsoMatStressDependentFormat::check(const IsoMatStressDependentCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.tableId, "tableId"))
        return problem;
    if (card.yieldFunction < 1 || card.yieldFunction > 4)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or 4",
                           "yieldFunction"};
    if (card.hardeningRule < 1 || card.hardeningRule > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3", "hardeningRule"};
    if (card.kind == IsoMatStressDependentType::NonlinearElastic)
        return std::nullopt; // H, YF, HR, LIMIT1 and LIMIT2 are not used
    // A plastic material: the table or the slope, the initial yield point, and for the criteria with a
    // friction angle that angle.
    if (card.hardeningSlope && card.tableId != 0)
        return CardProblem{"card.bad-value",
                           "give " + std::string(CardProblem::FIELD) + " or a table " +
                               CardProblem::fieldNamed("tableId") + ", not both",
                           "hardeningSlope"};
    if (!card.initialYieldStress)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + ", the initial yield point, is required",
                           "initialYieldStress"};
    if (card.yieldFunction == 3 || card.yieldFunction == 4)
    {
        if (!card.frictionAngle)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + ", the friction angle, is required for " +
                                   CardProblem::fieldNamed("yieldFunction") + " 3 and 4",
                               "frictionAngle"};
        if (!(*card.frictionAngle > 0.0 && *card.frictionAngle < 45.0))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be between 0.0 and 45.0 degrees",
                               "frictionAngle"};
    }
    return std::nullopt;
}

void IsoMatStressDependentFormat::write(const IsoMatStressDependentCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.tableId, 0);
    writer.text(3, typeWord(card.kind));
    if (card.hardeningSlope)
        writer.real(4, *card.hardeningSlope);
    writer.integerUnlessDefault(5, card.yieldFunction, 1);
    writer.integerUnlessDefault(6, card.hardeningRule, 1);
    if (card.initialYieldStress)
        writer.real(7, *card.initialYieldStress);
    if (card.frictionAngle)
        writer.real(8, *card.frictionAngle);
}

std::uint32_t IsoMatStressDependentTable::append(const IsoMatStressDependentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_tableId.push(card.tableId);
    m_type.push(static_cast<std::uint8_t>(card.kind));
    m_h.push(card.hardeningSlope.value_or(notGiven()));
    m_yieldFunction.push(card.yieldFunction);
    m_hardeningRule.push(card.hardeningRule);
    m_initialYieldStress.push(card.initialYieldStress.value_or(notGiven()));
    m_frictionAngle.push(card.frictionAngle.value_or(notGiven()));
    return row;
}

IsoMatStressDependentCard IsoMatStressDependentTable::card(std::uint32_t row) const
{
    IsoMatStressDependentCard card;
    card.id = m_id.get(row);
    card.tableId = m_tableId.get(row);
    card.kind = static_cast<IsoMatStressDependentType>(m_type.get(row));
    card.hardeningSlope = optionalOf(m_h.get(row));
    card.yieldFunction = m_yieldFunction.get(row);
    card.hardeningRule = m_hardeningRule.get(row);
    card.initialYieldStress = optionalOf(m_initialYieldStress.get(row));
    card.frictionAngle = optionalOf(m_frictionAngle.get(row));
    return card;
}

void IsoMatStressDependentTable::store(std::uint32_t row, const IsoMatStressDependentCard& card)
{
    m_id.set(row, card.id);
    m_tableId.set(row, card.tableId);
    m_type.set(row, static_cast<std::uint8_t>(card.kind));
    m_h.set(row, card.hardeningSlope.value_or(notGiven()));
    m_yieldFunction.set(row, card.yieldFunction);
    m_hardeningRule.set(row, card.hardeningRule);
    m_initialYieldStress.set(row, card.initialYieldStress.value_or(notGiven()));
    m_frictionAngle.set(row, card.frictionAngle.value_or(notGiven()));
}

void IsoMatStressDependentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_tableId.keepRows(rows);
    m_type.keepRows(rows);
    m_h.keepRows(rows);
    m_yieldFunction.keepRows(rows);
    m_hardeningRule.keepRows(rows);
    m_initialYieldStress.keepRows(rows);
    m_frictionAngle.keepRows(rows);
}

void IsoMatStressDependentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1 that the card extends; the table is a TABLES1 (or a TABLEST).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId table = m_tableId.get(row); table > 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::StressTable, table));
}

} // namespace oor::model
