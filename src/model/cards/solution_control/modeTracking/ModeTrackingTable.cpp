#include "oor/model/cards/solution_control/modeTracking/ModeTrackingTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<ModeTrackingCard> ModeTrackingFormat::read(FieldReader& reader)
{
    ModeTrackingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.lowRange = reader.integer(2, "lowRange", 0);
    card.highRange = reader.integer(3, "highRange", 0);
    card.trackingFilter = reader.real(4, "trackingFilter", 0.9);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("trackingFilter"),
                     "trackingFilter");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModeTrackingFormat::check(const ModeTrackingCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.lowRange < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "lowRange"};
    if (card.highRange < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "highRange"};
    if (card.lowRange != 0 && card.highRange != 0 && card.lowRange >= card.highRange)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("highRange"),
                           "lowRange"};
    if (!std::isfinite(card.trackingFilter))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "trackingFilter"};
    return std::nullopt;
}

void ModeTrackingFormat::write(const ModeTrackingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.lowRange, 0);
    writer.integerUnlessDefault(3, card.highRange, 0);
    writer.realUnlessDefault(4, card.trackingFilter, 0.9);
}

std::uint32_t ModeTrackingTable::append(const ModeTrackingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_lowRange.push(card.lowRange);
    m_highRange.push(card.highRange);
    m_trackingFilter.push(card.trackingFilter);
    return row;
}

ModeTrackingCard ModeTrackingTable::card(std::uint32_t row) const
{
    return ModeTrackingCard{m_id.get(row), m_lowRange.get(row), m_highRange.get(row), m_trackingFilter.get(row)};
}

void ModeTrackingTable::store(std::uint32_t row, const ModeTrackingCard& card)
{
    m_id.set(row, card.id);
    m_lowRange.set(row, card.lowRange);
    m_highRange.set(row, card.highRange);
    m_trackingFilter.set(row, card.trackingFilter);
}

void ModeTrackingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_lowRange.keepRows(rows);
    m_highRange.keepRows(rows);
    m_trackingFilter.keepRows(rows);
}

} // namespace oor::model
