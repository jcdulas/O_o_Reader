#include "oor/model/cards/explicit/initialGasComposition/InitialGasCompositionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 3; // GASNAM1; its FRAC1 is the next field, and so on two fields to a pair

} // namespace

std::optional<InitialGasCompositionCard> InitialGasCompositionFormat::read(FieldReader& reader)
{
    InitialGasCompositionCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    requireBlankFields(reader, {2}, last);
    // The pairs follow one another: a pair of blank fields in the middle is a gap that the card could not write back.
    bool gap = false;
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
        {
            gap = true;
            continue;
        }
        if (gap)
        {
            reader.error("card.bad-value",
                         "the " + std::string("gasId") + " and " + std::string("fraction") +
                             " pairs must follow one another: field " + std::to_string(field) +
                             " comes after a blank pair",
                         "gasId");
            break;
        }
        InitialGasFraction pair;
        pair.gasId = reader.requiredInteger(field, "gasId");
        pair.fraction = reader.requiredReal(field + 1, "fraction");
        card.gases.push_back(pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialGasCompositionFormat::check(const InitialGasCompositionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.gases.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("fraction") + " pair is required",
                           "gasId"};
    for (const InitialGasFraction& pair : card.gases)
    {
        if (const auto problem = checkRequiredId(pair.gasId, "gasId"))
            return problem;
        if (const auto problem = checkReal(pair.fraction, "fraction", RealRule::NonNegative))
            return problem;
    }
    return std::nullopt;
}

void InitialGasCompositionFormat::write(const InitialGasCompositionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.gases.size(); ++index)
    {
        const int field = FIRST_PAIR_FIELD + 2 * static_cast<int>(index);
        writer.integer(field, card.gases[index].gasId);
        writer.real(field + 1, card.gases[index].fraction);
    }
}

std::uint32_t InitialGasCompositionTable::append(const InitialGasCompositionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialGasCompositionTable::store(std::uint32_t row, const InitialGasCompositionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialGasCompositionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialGasCompositionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialGasCompositionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (InitialGasFraction& pair : m_cards.edit(row).gases)
        visitId(visitor, "gasId", ReferenceTarget::InflatorGas, pair.gasId);
}

} // namespace oor::model
