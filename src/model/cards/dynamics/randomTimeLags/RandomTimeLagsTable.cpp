#include "oor/model/cards/dynamics/randomTimeLags/RandomTimeLagsTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<RandomTimeLagsCard> RandomTimeLagsFormat::read(FieldReader& reader)
{
    RandomTimeLagsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.count = reader.requiredInteger(2, "count");
    card.startTime = reader.requiredReal(3, "startTime");
    card.maximumTime = reader.requiredReal(4, "maximumTime");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("maximumTime"), "maximumTime");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RandomTimeLagsFormat::check(const RandomTimeLagsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the random analysis set ID must be a positive integer", "id"};
    if (card.count < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer", "count"};
    if (!(card.startTime >= 0.0) || !std::isfinite(card.startTime))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "startTime"};
    if (!(card.maximumTime > card.startTime) || !std::isfinite(card.maximumTime))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be greater than " + CardProblem::fieldNamed("startTime"),
            "maximumTime"};
    return std::nullopt;
}

void RandomTimeLagsFormat::write(const RandomTimeLagsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.count);
    writer.real(3, card.startTime);
    writer.real(4, card.maximumTime);
}

std::uint32_t RandomTimeLagsTable::append(const RandomTimeLagsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_count.push(card.count);
    m_startTime.push(card.startTime);
    m_maximumTime.push(card.maximumTime);
    return row;
}

RandomTimeLagsCard RandomTimeLagsTable::card(std::uint32_t row) const
{
    return RandomTimeLagsCard{m_id.get(row), m_count.get(row), m_startTime.get(row), m_maximumTime.get(row)};
}

void RandomTimeLagsTable::store(std::uint32_t row, const RandomTimeLagsCard& card)
{
    m_id.set(row, card.id);
    m_count.set(row, card.count);
    m_startTime.set(row, card.startTime);
    m_maximumTime.set(row, card.maximumTime);
}

void RandomTimeLagsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_count.keepRows(rows);
    m_startTime.keepRows(rows);
    m_maximumTime.keepRows(rows);
}

} // namespace oor::model
