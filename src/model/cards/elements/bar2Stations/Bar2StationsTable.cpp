#include "oor/model/cards/elements/bar2Stations/Bar2StationsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <variant>

namespace oor::model
{

std::optional<Bar2StationsCard> Bar2StationsFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES[Bar2StationsCard::MAX_LISTED] = {"station1", "station2", "station3",
                                                                           "station4", "station5", "station6"};
    Bar2StationsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stationScale = std::string(reader.word(2));
    // Field 3 is NPTS (an integer: equally spaced stations) or X1 (a real: a list of stations).
    const auto third = reader.integerOrReal(3, "station1/stationCount");
    if (third && std::holds_alternative<std::int32_t>(*third))
    {
        card.stationCount = std::get<std::int32_t>(*third);
        card.first = reader.requiredReal(4, "station1");
        card.stationSpacing = reader.requiredReal(5, "stationSpacing");
        if (reader.lastUsedField() > 5)
            reader.error("card.bad-value",
                         "the equally spaced form of a " + std::string(reader.cardName()) + " ends with " +
                             std::string("stationSpacing") + " (field 5)",
                         "stationSpacing");
    }
    else
    {
        if (third)
            card.listedStations.push_back(std::get<double>(*third));
        for (std::size_t index = 1; index < Bar2StationsCard::MAX_LISTED; ++index)
        {
            // A blank field between two stations is skipped.
            if (const auto value = reader.optionalReal(3 + static_cast<int>(index), NAMES[index]))
                card.listedStations.push_back(*value);
        }
        if (reader.lastUsedField() > 8)
            reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " lists at most six stations",
                         "station6");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Bar2StationsFormat::check(const Bar2StationsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.stationScale != "ActualLength" && card.stationScale != "FractionalLength")
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be ActualLength or FractionalLength",
                           "stationScale"};
    if (card.stationCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "stationCount"};
    if (card.isSpaced())
    {
        if (!card.listedStations.empty())
            return CardProblem{"card.exclusive-fields", "the stations are listed or equally spaced, not both",
                               "stationCount"};
        if (card.first <= 0.0)
            return CardProblem{"card.bad-value",
                               "the first station " + std::string(CardProblem::FIELD) + " must be greater than 0",
                               "station1"};
        return std::nullopt;
    }
    if (card.listedStations.empty())
        return CardProblem{"card.missing-field", "the card needs at least one station", "station1"};
    if (card.listedStations.size() > Bar2StationsCard::MAX_LISTED)
        return CardProblem{"card.bad-value", "the card lists at most six stations", "station6"};
    for (const double station : card.listedStations)
    {
        if (!(station > 0.0))
            return CardProblem{"card.bad-value", "the stations must be greater than 0", "station1"};
    }
    return std::nullopt;
}

void Bar2StationsFormat::write(const Bar2StationsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.stationScale);
    if (card.isSpaced())
    {
        writer.integer(3, card.stationCount);
        writer.real(4, card.first);
        writer.real(5, card.stationSpacing);
        return;
    }
    for (std::size_t index = 0; index < card.listedStations.size(); ++index)
        writer.real(3 + static_cast<int>(index), card.listedStations[index]);
}

std::uint32_t Bar2StationsTable::append(const Bar2StationsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stationScale.push(card.stationScale);
    m_listedStations.push(card.listedStations);
    m_stationCount.push(card.stationCount);
    m_first.push(card.first);
    m_stationSpacing.push(card.stationSpacing);
    return row;
}

Bar2StationsCard Bar2StationsTable::card(std::uint32_t row) const
{
    Bar2StationsCard card;
    card.id = m_id.get(row);
    card.stationScale = m_stationScale.get(row);
    const auto x = m_listedStations.get(row);
    card.listedStations.assign(x.begin(), x.end());
    card.stationCount = m_stationCount.get(row);
    card.first = m_first.get(row);
    card.stationSpacing = m_stationSpacing.get(row);
    return card;
}

void Bar2StationsTable::store(std::uint32_t row, const Bar2StationsCard& card)
{
    m_id.set(row, card.id);
    m_stationScale.set(row, card.stationScale);
    m_listedStations.set(row, card.listedStations);
    m_stationCount.set(row, card.stationCount);
    m_first.set(row, card.first);
    m_stationSpacing.set(row, card.stationSpacing);
}

void Bar2StationsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stationScale.keepRows(rows);
    m_listedStations.keepRows(rows);
    m_stationCount.keepRows(rows);
    m_first.keepRows(rows);
    m_stationSpacing.keepRows(rows);
}

void Bar2StationsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the CBAR the stations are for.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Element, m_id.get(row)));
}

} // namespace oor::model
