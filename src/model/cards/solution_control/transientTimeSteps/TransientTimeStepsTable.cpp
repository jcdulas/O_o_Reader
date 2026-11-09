#include "oor/model/cards/solution_control/transientTimeSteps/TransientTimeStepsTable.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_INTERVAL_FIELD = 2;
constexpr int INTERVAL_STRIDE = 8; // one interval on each line

} // namespace

std::optional<TransientTimeStepsCard> TransientTimeStepsFormat::read(FieldReader& reader)
{
    TransientTimeStepsCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (int first = FIRST_INTERVAL_FIELD; first <= last && !reader.failed(); first += INTERVAL_STRIDE)
    {
        // The fields between the intervals (the first of each continuation line) are blank.
        if (first > FIRST_INTERVAL_FIELD && !reader.isBlank(first - 1))
            reader.error(
                "card.bad-value",
                "the first field of a continuation line of a " + std::string(reader.cardName()) + " is not used",
                "stepCounts");
        TransientTimeStepsInterval interval;
        interval.stepCount = reader.requiredInteger(first, "stepCounts");
        interval.timeStep = reader.requiredReal(first + 1, "timeSteps");
        interval.outputInterval = reader.integer(first + 2, "outputIntervals", 1);
        for (int field = first + 3; field < first + INTERVAL_STRIDE - 1; ++field)
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) + " has Ni, " + std::string("timeSteps") +
                                 " and " + std::string("outputIntervals") + " only",
                             "outputIntervals");
        }
        card.intervals.push_back(interval);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransientTimeStepsFormat::check(const TransientTimeStepsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.intervals.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one interval (N1, " + CardProblem::fieldNamed("timeStep1") + ")",
                           "stepCounts"};
    for (const TransientTimeStepsInterval& interval : card.intervals)
    {
        if (interval.stepCount < 1)
            return CardProblem{"card.bad-value", "Ni must be a positive integer", "stepCounts"};
        if (!(interval.timeStep > 0.0) || !std::isfinite(interval.timeStep))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "timeSteps"};
        if (interval.outputInterval < 1)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "outputIntervals"};
    }
    return std::nullopt;
}

void TransientTimeStepsFormat::write(const TransientTimeStepsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int first = FIRST_INTERVAL_FIELD;
    for (const TransientTimeStepsInterval& interval : card.intervals)
    {
        writer.integer(first, interval.stepCount);
        writer.real(first + 1, interval.timeStep);
        writer.integerUnlessDefault(first + 2, interval.outputInterval, 1);
        first += INTERVAL_STRIDE;
    }
}

std::uint32_t TransientTimeStepsTable::append(const TransientTimeStepsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<std::int32_t> stepCounts;
    std::vector<double> timeSteps;
    std::vector<std::int32_t> outputIntervals;
    for (const TransientTimeStepsInterval& interval : card.intervals)
    {
        stepCounts.push_back(interval.stepCount);
        timeSteps.push_back(interval.timeStep);
        outputIntervals.push_back(interval.outputInterval);
    }
    m_stepCounts.push(stepCounts);
    m_timeSteps.push(timeSteps);
    m_outputIntervals.push(outputIntervals);
    return row;
}

TransientTimeStepsCard TransientTimeStepsTable::card(std::uint32_t row) const
{
    TransientTimeStepsCard card;
    card.id = m_id.get(row);
    const auto stepCounts = m_stepCounts.get(row);
    const auto timeSteps = m_timeSteps.get(row);
    const auto outputIntervals = m_outputIntervals.get(row);
    for (std::size_t index = 0; index < stepCounts.size(); ++index)
        card.intervals.push_back(
            TransientTimeStepsInterval{stepCounts[index], timeSteps[index], outputIntervals[index]});
    return card;
}

void TransientTimeStepsTable::store(std::uint32_t row, const TransientTimeStepsCard& card)
{
    m_id.set(row, card.id);
    std::vector<std::int32_t> stepCounts;
    std::vector<double> timeSteps;
    std::vector<std::int32_t> outputIntervals;
    for (const TransientTimeStepsInterval& interval : card.intervals)
    {
        stepCounts.push_back(interval.stepCount);
        timeSteps.push_back(interval.timeStep);
        outputIntervals.push_back(interval.outputInterval);
    }
    m_stepCounts.set(row, stepCounts);
    m_timeSteps.set(row, timeSteps);
    m_outputIntervals.set(row, outputIntervals);
}

void TransientTimeStepsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stepCounts.keepRows(rows);
    m_timeSteps.keepRows(rows);
    m_outputIntervals.keepRows(rows);
}

} // namespace oor::model
