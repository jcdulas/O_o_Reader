#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqSteps/NonlinearHarmonicFreqStepsTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<NonlinearHarmonicFreqStepsCard> NonlinearHarmonicFreqStepsFormat::read(FieldReader& reader)
{
    NonlinearHarmonicFreqStepsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.startFreq = reader.requiredReal(2, "startFreq");
    card.freqStep = reader.requiredReal(3, "freqStep");
    card.stepCount = reader.integer(4, "stepCount", 1);
    requireUnusedBlank(reader, {5, 6, 7, 8}, 4);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearHarmonicFreqStepsFormat::check(const NonlinearHarmonicFreqStepsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (!(card.startFreq > 0.0) || !std::isfinite(card.startFreq))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "startFreq"};
    if (card.freqStep == 0.0 || !std::isfinite(card.freqStep))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "freqStep"};
    if (card.stepCount <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "stepCount"};
    return std::nullopt;
}

void NonlinearHarmonicFreqStepsFormat::write(const NonlinearHarmonicFreqStepsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.startFreq);
    writer.real(3, card.freqStep);
    writer.integerUnlessDefault(4, card.stepCount, 1);
}

std::uint32_t NonlinearHarmonicFreqStepsTable::append(const NonlinearHarmonicFreqStepsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_startFreq.push(card.startFreq);
    m_freqStep.push(card.freqStep);
    m_stepCount.push(card.stepCount);
    return row;
}

NonlinearHarmonicFreqStepsCard NonlinearHarmonicFreqStepsTable::card(std::uint32_t row) const
{
    NonlinearHarmonicFreqStepsCard card;
    card.id = m_id.get(row);
    card.startFreq = m_startFreq.get(row);
    card.freqStep = m_freqStep.get(row);
    card.stepCount = m_stepCount.get(row);
    return card;
}

void NonlinearHarmonicFreqStepsTable::store(std::uint32_t row, const NonlinearHarmonicFreqStepsCard& card)
{
    m_id.set(row, card.id);
    m_startFreq.set(row, card.startFreq);
    m_freqStep.set(row, card.freqStep);
    m_stepCount.set(row, card.stepCount);
}

void NonlinearHarmonicFreqStepsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_startFreq.keepRows(rows);
    m_freqStep.keepRows(rows);
    m_stepCount.keepRows(rows);
}

} // namespace oor::model
