#include "oor/model/cards/explicit/hybridInflator/HybridInflatorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_GAS_FIELD = 9; // the first field of the first continuation line

} // namespace

std::optional<HybridInflatorCard> HybridInflatorFormat::read(FieldReader& reader)
{
    HybridInflatorCard card;
    card.id = reader.requiredInteger(1, "id");
    card.massFlowTableId = reader.requiredInteger(2, "massFlowTableId");
    card.temperatureTableId = reader.integer(3, "temperatureTableId", 0);
    card.temperature = reader.optionalReal(4, "temperature");
    card.fractionId = reader.requiredInteger(5, "fractionId");
    const int last = reader.lastUsedField();
    requireBlankFields(reader, {6, 7, 8}, last);
    // The gases fill the fields from the first of the continuation line: a blank field between two of them is a gap
    // that the card could not write back.
    bool gap = false;
    for (int field = FIRST_GAS_FIELD; field <= last; ++field)
    {
        if (reader.isBlank(field))
        {
            gap = true;
            continue;
        }
        if (gap)
        {
            reader.error("card.bad-value",
                         "the " + std::string("gasId") + " fields must follow one another: field " +
                             std::to_string(field) + " comes after a blank one",
                         "gasId");
            break;
        }
        card.gasId.push_back(reader.requiredInteger(field, "gasId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HybridInflatorFormat::check(const HybridInflatorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.massFlowTableId, "massFlowTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.temperatureTableId, "temperatureTableId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.temperature, "temperature", RealRule::Positive))
        return problem;
    if (card.temperatureTableId == 0 && !card.temperature)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("temperature") + " is required",
            "temperatureTableId"};
    if (card.temperatureTableId != 0 && card.temperature)
        return CardProblem{"card.bad-value",
                           "give " + CardProblem::fieldNamed("temperatureTableId") + " or " +
                               std::string(CardProblem::FIELD) + ", not both",
                           "temperature"};
    if (const auto problem = checkRequiredId(card.fractionId, "fractionId"))
        return problem;
    if (card.gasId.empty())
        return CardProblem{"card.bad-value",
                           "at least one " + std::string(CardProblem::FIELD) + " (an " +
                               CardProblem::cardNamed("InflatorGas") + ") is required",
                           "gasId"};
    for (const base::EntityId gas : card.gasId)
    {
        if (const auto problem = checkRequiredId(gas, "gasId"))
            return problem;
    }
    return std::nullopt;
}

void HybridInflatorFormat::write(const HybridInflatorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.massFlowTableId);
    writer.integerUnlessDefault(3, card.temperatureTableId, 0);
    if (card.temperature)
        writer.real(4, *card.temperature);
    writer.integer(5, card.fractionId);
    for (std::size_t index = 0; index < card.gasId.size(); ++index)
        writer.integer(FIRST_GAS_FIELD + static_cast<int>(index), card.gasId[index]);
}

std::uint32_t HybridInflatorTable::append(const HybridInflatorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HybridInflatorTable::store(std::uint32_t row, const HybridInflatorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HybridInflatorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HybridInflatorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HybridInflatorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    HybridInflatorCard& card = m_cards.edit(row);
    visitId(visitor, "massFlowTableId", ReferenceTarget::DynamicTable, card.massFlowTableId);
    visitId(visitor, "temperatureTableId", ReferenceTarget::DynamicTable, card.temperatureTableId);
    visitId(visitor, "fractionId", ReferenceTarget::GasFractions, card.fractionId);
    for (base::EntityId& gas : card.gasId)
        visitId(visitor, "gasId", ReferenceTarget::InflatorGas, gas);
}

} // namespace oor::model
