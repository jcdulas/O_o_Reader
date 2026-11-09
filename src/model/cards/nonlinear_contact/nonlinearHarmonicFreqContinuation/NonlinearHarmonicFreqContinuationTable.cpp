#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqContinuation/NonlinearHarmonicFreqContinuationTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The fields of the card: the first line is 1 to 8, the second 9 and 10, the output request line starts at 17.
constexpr int OUTPUT_REQUEST_FIELD = 17;

} // namespace

std::optional<NonlinearHarmonicFreqContinuationCard> NonlinearHarmonicFreqContinuationFormat::read(FieldReader& reader)
{
    NonlinearHarmonicFreqContinuationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.initialFreq = reader.real(2, "initialFreq", 0.0);
    card.initialStep = reader.requiredReal(3, "initialStep");
    card.minimumStepSize = reader.optionalReal(4, "minimumStepSize");
    card.maximumStepSize = reader.optionalReal(5, "maximumStepSize");
    card.maximumSteps = reader.integer(6, "maximumSteps", 3);
    card.minimumFreq = reader.real(7, "minimumFreq", 0.0);
    card.maximumFreq = reader.optionalReal(8, "maximumFreq");
    card.derivativeStep = reader.real(9, "derivativeStep", 1.0E-3);
    card.printOnFailure = reader.isBlank(10) ? "No" : std::string(reader.text(10));
    requireUnusedBlank(reader, {11, 12, 13, 14, 15, 16}, 21);
    if (!reader.isBlank(OUTPUT_REQUEST_FIELD))
    {
        card.outputRequest = (reader.text(OUTPUT_REQUEST_FIELD) == "DefaultOutput");
        if (!card.outputRequest)
            reader.error("card.bad-value",
                         "the third line of an " + std::string(reader.cardName()) + " starts with DFOUT",
                         "outputRequest");
        card.outputFreqStep = reader.requiredReal(OUTPUT_REQUEST_FIELD + 1, "outputFreqStep");
        card.outputFreqTolerance = reader.real(OUTPUT_REQUEST_FIELD + 2, "outputFreqTolerance", 5.0);
        card.outputStartFreq = reader.optionalReal(OUTPUT_REQUEST_FIELD + 3, "outputStartFreq");
        card.outputEndFreq = reader.optionalReal(OUTPUT_REQUEST_FIELD + 4, "outputEndFreq");
    }
    else
    {
        // Without the DFOUT keyword the four fields after it are not read.
        requireUnusedBlank(reader, {18, 19, 20, 21}, 21);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearHarmonicFreqContinuationFormat::check(
    const NonlinearHarmonicFreqContinuationCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (!(card.initialFreq >= 0.0) || !std::isfinite(card.initialFreq))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "initialFreq"};
    if (card.initialStep == 0.0 || !std::isfinite(card.initialStep))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "initialStep"};
    if (card.minimumStepSize && !(*card.minimumStepSize >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "minimumStepSize"};
    if (card.maximumStepSize && !(*card.maximumStepSize >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "maximumStepSize"};
    if (card.maximumSteps < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "maximumSteps"};
    if (!(card.minimumFreq >= 0.0) || !std::isfinite(card.minimumFreq))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "minimumFreq"};
    if (card.maximumFreq && !(*card.maximumFreq >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "maximumFreq"};
    if (!std::isfinite(card.derivativeStep))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "derivativeStep"};
    if (card.printOnFailure != "No" && card.printOnFailure != "Yes")
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be NO or YES", "printOnFailure"};
    if (card.outputRequest)
    {
        if (!(card.outputFreqStep > 0.0) || !std::isfinite(card.outputFreqStep))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "outputFreqStep"};
        if (!(card.outputFreqTolerance >= 0.0) || !std::isfinite(card.outputFreqTolerance))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "outputFreqTolerance"};
        if (card.outputStartFreq && !(*card.outputStartFreq >= 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "outputStartFreq"};
        if (card.outputEndFreq && !(*card.outputEndFreq >= 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "outputEndFreq"};
    }
    else if (card.outputFreqStep != 0.0 || card.outputFreqTolerance != 5.0 || card.outputStartFreq ||
             card.outputEndFreq)
    {
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("outputFreqStep") + ", " +
                               CardProblem::fieldNamed("outputFreqTolerance") + ", " +
                               CardProblem::fieldNamed("outputStartFreq") + " and " +
                               CardProblem::fieldNamed("outputEndFreq") + " need " + std::string(CardProblem::FIELD),
                           "outputRequest"};
    }
    return std::nullopt;
}

void NonlinearHarmonicFreqContinuationFormat::write(const NonlinearHarmonicFreqContinuationCard& card,
                                                    FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.initialFreq, 0.0);
    writer.real(3, card.initialStep);
    if (card.minimumStepSize)
        writer.real(4, *card.minimumStepSize);
    if (card.maximumStepSize)
        writer.real(5, *card.maximumStepSize);
    writer.integerUnlessDefault(6, card.maximumSteps, 3);
    writer.realUnlessDefault(7, card.minimumFreq, 0.0);
    if (card.maximumFreq)
        writer.real(8, *card.maximumFreq);
    writer.realUnlessDefault(9, card.derivativeStep, 1.0E-3);
    writer.textUnlessDefault(10, card.printOnFailure, "No");
    if (!card.outputRequest)
        return;
    writer.text(OUTPUT_REQUEST_FIELD, "DefaultOutput");
    writer.real(OUTPUT_REQUEST_FIELD + 1, card.outputFreqStep);
    writer.realUnlessDefault(OUTPUT_REQUEST_FIELD + 2, card.outputFreqTolerance, 5.0);
    if (card.outputStartFreq)
        writer.real(OUTPUT_REQUEST_FIELD + 3, *card.outputStartFreq);
    if (card.outputEndFreq)
        writer.real(OUTPUT_REQUEST_FIELD + 4, *card.outputEndFreq);
}

std::uint32_t NonlinearHarmonicFreqContinuationTable::append(const NonlinearHarmonicFreqContinuationCard& card,
                                                             base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NonlinearHarmonicFreqContinuationTable::store(std::uint32_t row, const NonlinearHarmonicFreqContinuationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NonlinearHarmonicFreqContinuationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearHarmonicFreqContinuationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
