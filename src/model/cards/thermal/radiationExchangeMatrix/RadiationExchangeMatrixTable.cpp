#include "oor/model/cards/thermal/radiationExchangeMatrix/RadiationExchangeMatrixTable.hpp"

#include <string>

namespace oor::model
{

std::optional<RadiationExchangeMatrixCard> RadiationExchangeMatrixFormat::read(FieldReader& reader)
{
    RadiationExchangeMatrixCard card;
    card.cavityId = reader.requiredInteger(1, "cavityId");
    card.columnIndex = reader.requiredInteger(2, "columnIndex");
    // The coefficients of the column follow, one per field, over as many lines as the card has.
    const int last = reader.lastUsedField();
    for (int field = 3; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        card.factors.push_back(reader.requiredReal(field, "exchangeFactor"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationExchangeMatrixFormat::check(const RadiationExchangeMatrixCard& card)
{
    if (card.cavityId <= 0)
        return CardProblem{"card.bad-id", "the cavity ID must be a positive integer", "cavityId"};
    if (card.columnIndex <= 0)
        return CardProblem{
            "card.bad-id",
            std::string("the column number ") + std::string(CardProblem::FIELD) + " must be a positive integer",
            "columnIndex"};
    if (card.factors.empty())
        return CardProblem{"card.missing-field", "a column of the matrix needs at least one coefficient",
                           "exchangeFactor"};
    for (const double factor : card.factors)
    {
        if (!(factor >= 0.0))
            return CardProblem{"card.bad-value", "a radiation exchange coefficient cannot be negative",
                               "exchangeFactor"};
    }
    return std::nullopt;
}

void RadiationExchangeMatrixFormat::write(const RadiationExchangeMatrixCard& card, FieldWriter& writer)
{
    writer.integer(1, card.cavityId);
    writer.integer(2, card.columnIndex);
    for (std::size_t index = 0; index < card.factors.size(); ++index)
        writer.real(3 + static_cast<int>(index), card.factors[index]);
}

std::uint32_t RadiationExchangeMatrixTable::append(const RadiationExchangeMatrixCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cavityId.push(card.cavityId);
    m_columnIndex.push(card.columnIndex);
    m_factors.push(card.factors);
    return row;
}

RadiationExchangeMatrixCard RadiationExchangeMatrixTable::card(std::uint32_t row) const
{
    RadiationExchangeMatrixCard card;
    card.cavityId = m_cavityId.get(row);
    card.columnIndex = m_columnIndex.get(row);
    const auto factors = m_factors.get(row);
    card.factors.assign(factors.begin(), factors.end());
    return card;
}

void RadiationExchangeMatrixTable::store(std::uint32_t row, const RadiationExchangeMatrixCard& card)
{
    m_cavityId.set(row, card.cavityId);
    m_columnIndex.set(row, card.columnIndex);
    m_factors.set(row, card.factors);
}

void RadiationExchangeMatrixTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_cavityId.keepRows(rows);
    m_columnIndex.keepRows(rows);
    m_factors.keepRows(rows);
}

} // namespace oor::model
