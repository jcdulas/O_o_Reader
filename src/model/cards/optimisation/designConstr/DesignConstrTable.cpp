#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cstdint>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 6;

// A bound that is a real, or an integer that numbers a table. The bound keeps its default when it is a table.
void readBound(FieldReader& reader, int field, const char* name, double fallback, double& bound, base::EntityId& table)
{
    bound = fallback;
    table = 0;
    const auto value = reader.integerOrReal(field, name);
    if (!value)
        return;
    if (std::holds_alternative<std::int32_t>(*value))
        table = std::get<std::int32_t>(*value);
    else
        bound = std::get<double>(*value);
}

void writeBound(FieldWriter& writer, int field, double bound, base::EntityId table, double fallback)
{
    if (table > 0)
        writer.integer(field, table);
    else
        writer.realUnlessDefault(field, bound, fallback);
}

} // namespace

std::optional<DesignConstrCard> DesignConstrFormat::read(FieldReader& reader)
{
    DesignConstrCard card;
    card.constraintSetId = reader.requiredInteger(1, "constraintSetId");
    card.responseId = reader.requiredInteger(2, "responseId");
    readBound(reader, 3, "lowerAllowable", DesignConstrCard::NO_LOWER_BOUND, card.lowerAllowable,
              card.lowerAllowableTableId);
    readBound(reader, 4, "upperAllowable", DesignConstrCard::NO_UPPER_BOUND, card.upperAllowable,
              card.upperAllowableTableId);
    card.lowFrequency = reader.real(5, "lowFrequency", 0.0);
    card.highFrequency = reader.real(6, "highFrequency", DesignConstrCard::NO_HIGH_FREQUENCY);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignConstrFormat::check(const DesignConstrCard& card)
{
    if (const auto problem = checkRequiredId(card.constraintSetId, "constraintSetId"))
        return problem;
    if (const auto problem = checkRequiredId(card.responseId, "responseId"))
        return problem;
    if (const auto problem = checkReal(card.lowerAllowable, "lowerAllowable", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.lowerAllowableTableId, "lowerAllowableTableId"))
        return problem;
    if (const auto problem = checkReal(card.upperAllowable, "upperAllowable", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.upperAllowableTableId, "upperAllowableTableId"))
        return problem;
    if (const auto problem = checkReal(card.lowFrequency, "lowFrequency", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.highFrequency, "highFrequency", RealRule::Finite))
        return problem;
    if (card.highFrequency < card.lowFrequency)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be below " + CardProblem::fieldNamed("lowFrequency"),
            "highFrequency"};
    return std::nullopt;
}

void DesignConstrFormat::write(const DesignConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.constraintSetId);
    writer.integer(2, card.responseId);
    writeBound(writer, 3, card.lowerAllowable, card.lowerAllowableTableId, DesignConstrCard::NO_LOWER_BOUND);
    writeBound(writer, 4, card.upperAllowable, card.upperAllowableTableId, DesignConstrCard::NO_UPPER_BOUND);
    writer.realUnlessDefault(5, card.lowFrequency, 0.0);
    writer.realUnlessDefault(6, card.highFrequency, DesignConstrCard::NO_HIGH_FREQUENCY);
}

std::uint32_t DesignConstrTable::append(const DesignConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_constraintSetId.push(card.constraintSetId);
    m_responseId.push(card.responseId);
    m_lowerAllowable.push(card.lowerAllowable);
    m_lowerAllowableTableId.push(card.lowerAllowableTableId);
    m_upperAllowable.push(card.upperAllowable);
    m_upperAllowableTableId.push(card.upperAllowableTableId);
    m_lowFrequency.push(card.lowFrequency);
    m_highFrequency.push(card.highFrequency);
    return row;
}

DesignConstrCard DesignConstrTable::card(std::uint32_t row) const
{
    DesignConstrCard card;
    card.constraintSetId = m_constraintSetId.get(row);
    card.responseId = m_responseId.get(row);
    card.lowerAllowable = m_lowerAllowable.get(row);
    card.lowerAllowableTableId = m_lowerAllowableTableId.get(row);
    card.upperAllowable = m_upperAllowable.get(row);
    card.upperAllowableTableId = m_upperAllowableTableId.get(row);
    card.lowFrequency = m_lowFrequency.get(row);
    card.highFrequency = m_highFrequency.get(row);
    return card;
}

void DesignConstrTable::store(std::uint32_t row, const DesignConstrCard& card)
{
    m_constraintSetId.set(row, card.constraintSetId);
    m_responseId.set(row, card.responseId);
    m_lowerAllowable.set(row, card.lowerAllowable);
    m_lowerAllowableTableId.set(row, card.lowerAllowableTableId);
    m_upperAllowable.set(row, card.upperAllowable);
    m_upperAllowableTableId.set(row, card.upperAllowableTableId);
    m_lowFrequency.set(row, card.lowFrequency);
    m_highFrequency.set(row, card.highFrequency);
}

void DesignConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_constraintSetId.keepRows(rows);
    m_responseId.keepRows(rows);
    m_lowerAllowable.keepRows(rows);
    m_lowerAllowableTableId.keepRows(rows);
    m_upperAllowable.keepRows(rows);
    m_upperAllowableTableId.keepRows(rows);
    m_lowFrequency.keepRows(rows);
    m_highFrequency.keepRows(rows);
}

void DesignConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId responseId = m_responseId.get(row);
    visitId(visitor, "responseId", ReferenceTarget::DesignResponse, responseId);
    m_responseId.set(row, responseId);
    base::EntityId lowerTableId = m_lowerAllowableTableId.get(row);
    visitId(visitor, "lowerAllowableTableId", ReferenceTarget::DynamicTable, lowerTableId);
    m_lowerAllowableTableId.set(row, lowerTableId);
    base::EntityId upperTableId = m_upperAllowableTableId.get(row);
    visitId(visitor, "upperAllowableTableId", ReferenceTarget::DynamicTable, upperTableId);
    m_upperAllowableTableId.set(row, upperTableId);
}

} // namespace oor::model
