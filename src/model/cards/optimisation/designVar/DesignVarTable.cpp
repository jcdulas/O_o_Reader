#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 7;

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : DesignVarTable::BLANK;
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

std::optional<DesignVarCard> DesignVarFormat::read(FieldReader& reader)
{
    DesignVarCard card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    card.initialValue = reader.requiredReal(3, "initialValue");
    card.lowerBound = reader.real(4, "lowerBound", DesignVarCard::NO_LOWER_BOUND);
    card.upperBound = reader.real(5, "upperBound", DesignVarCard::NO_UPPER_BOUND);
    card.designVarMoveLimit = reader.optionalReal(6, "designVarMoveLimit");
    card.discreteValuesId = reader.integer(7, "discreteValuesId", 0);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignVarFormat::check(const DesignVarCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (const auto problem = checkReal(card.initialValue, "initialValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.lowerBound, "lowerBound", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.upperBound, "upperBound", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.designVarMoveLimit, "designVarMoveLimit", RealRule::Positive))
        return problem;
    return checkOptionalId(card.discreteValuesId, "discreteValuesId");
}

void DesignVarFormat::write(const DesignVarCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    writer.real(3, card.initialValue);
    writer.realUnlessDefault(4, card.lowerBound, DesignVarCard::NO_LOWER_BOUND);
    writer.realUnlessDefault(5, card.upperBound, DesignVarCard::NO_UPPER_BOUND);
    if (card.designVarMoveLimit)
        writer.real(6, *card.designVarMoveLimit);
    writer.integerUnlessDefault(7, card.discreteValuesId, 0);
}

std::uint32_t DesignVarTable::append(const DesignVarCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_entryLabel.push(card.entryLabel);
    m_initialValue.push(card.initialValue);
    m_lowerBound.push(card.lowerBound);
    m_upperBound.push(card.upperBound);
    m_designVarMoveLimit.push(blankIfNone(card.designVarMoveLimit));
    m_discreteValuesId.push(card.discreteValuesId);
    return row;
}

DesignVarCard DesignVarTable::card(std::uint32_t row) const
{
    DesignVarCard card;
    card.id = m_id.get(row);
    card.entryLabel = m_entryLabel.get(row);
    card.initialValue = m_initialValue.get(row);
    card.lowerBound = m_lowerBound.get(row);
    card.upperBound = m_upperBound.get(row);
    card.designVarMoveLimit = noneIfBlank(m_designVarMoveLimit.get(row));
    card.discreteValuesId = m_discreteValuesId.get(row);
    return card;
}

void DesignVarTable::store(std::uint32_t row, const DesignVarCard& card)
{
    m_id.set(row, card.id);
    m_entryLabel.set(row, card.entryLabel);
    m_initialValue.set(row, card.initialValue);
    m_lowerBound.set(row, card.lowerBound);
    m_upperBound.set(row, card.upperBound);
    m_designVarMoveLimit.set(row, blankIfNone(card.designVarMoveLimit));
    m_discreteValuesId.set(row, card.discreteValuesId);
}

void DesignVarTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId discreteValuesId = m_discreteValuesId.get(row);
    visitId(visitor, "discreteValuesId", ReferenceTarget::DiscreteValueSet, discreteValuesId);
    m_discreteValuesId.set(row, discreteValuesId);
}

void DesignVarTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_entryLabel.keepRows(rows);
    m_initialValue.keepRows(rows);
    m_lowerBound.keepRows(rows);
    m_upperBound.keepRows(rows);
    m_designVarMoveLimit.keepRows(rows);
    m_discreteValuesId.keepRows(rows);
}

} // namespace oor::model
