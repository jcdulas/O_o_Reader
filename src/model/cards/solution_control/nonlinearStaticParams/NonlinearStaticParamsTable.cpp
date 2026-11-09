#include "oor/model/cards/solution_control/nonlinearStaticParams/NonlinearStaticParamsTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

std::optional<StiffnessMethod> parseMethod(std::string_view text)
{
    if (text.empty())
        return StiffnessMethod::NotGiven;
    if ((text == "Automatic"))
        return StiffnessMethod::Auto;
    if ((text == "Iterative"))
        return StiffnessMethod::Iter;
    if ((text == "Semi"))
        return StiffnessMethod::Semi;
    if ((text == "FullNewtonTangent"))
        return StiffnessMethod::Fnt;
    if ((text == "PseudoFullNewtonTangent"))
        return StiffnessMethod::Pfnt;
    return std::nullopt;
}

const char* methodWord(StiffnessMethod method)
{
    switch (method)
    {
    case StiffnessMethod::Auto:
        return "Automatic";
    case StiffnessMethod::Iter:
        return "Iterative";
    case StiffnessMethod::Semi:
        return "Semi";
    case StiffnessMethod::Fnt:
        return "FullNewtonTangent";
    case StiffnessMethod::Pfnt:
        return "PseudoFullNewtonTangent";
    case StiffnessMethod::NotGiven:
        break;
    }
    return "";
}

// The letters of the convergence flags: displacement, load, work, vector component, length and automatic switch.
constexpr std::string_view CONVERGENCE_LETTERS = "UPWVNA";

// INTOUT: YES, NO, ALL or a positive integer (SOL 400).
bool isIntermediateOutput(std::string_view text)
{
    if (text == "Yes" || text == "No" || text == "All")
        return true;
    const auto count = base::NumberParser::parseInteger(text);
    return count && *count > 0;
}

} // namespace

std::optional<NonlinearStaticParamsCard> NonlinearStaticParamsFormat::read(FieldReader& reader)
{
    NonlinearStaticParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.incrementCount = reader.optionalInteger(2, "incrementCount");
    card.timeStep = reader.real(3, "timeStep", 0.0);
    const std::string_view methodText = reader.text(4);
    if (const auto method = parseMethod(methodText))
        card.stiffnessMethod = *method;
    else
        reader.error("card.bad-value",
                     std::string("stiffnessMethod") + " must be AUTO, ITER, SEMI, FNT or PFNT, found \"" +
                         std::string(methodText) + "\"",
                     "stiffnessMethod");
    card.stiffnessUpdateStep = reader.optionalInteger(5, "stiffnessUpdateStep");
    card.maxIterations = reader.integer(6, "maxIterations", 25);
    card.convergenceFlags = base::toUpperAscii(reader.text(7));
    const std::string_view intoutText = reader.text(8);
    card.intermediateOutput = intoutText.empty() ? "No" : std::string(intoutText);
    card.epsilonU = reader.optionalReal(9, "epsilonU");
    card.epsilonP = reader.optionalReal(10, "epsilonP");
    card.epsilonW = reader.optionalReal(11, "epsilonW");
    card.maxDivergences = reader.integer(12, "maxDivergences", 3);
    card.maxQuasiNewtonVectors =
        reader.integer(13, "maxQuasiNewtonVectors",
                       nonlinearStaticParamsDefaultMaxQuasiNewtonVectors(card.stiffnessMethod, card.maxIterations));
    card.maxLineSearches =
        reader.integer(14, "maxLineSearches", nonlinearStaticParamsDefaultMaxLineSearches(card.stiffnessMethod));
    card.stressSubincrementFraction = reader.real(15, "stressSubincrementFraction", 0.2);
    card.lineSearchTolerance = reader.real(16, "lineSearchTolerance", 0.5);
    card.maxBisections =
        reader.integer(17, "maxBisections", nonlinearStaticParamsDefaultMaxBisections(card.maxIterations));
    for (const int field : {18, 19, 20, 22})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of an " + std::string(reader.cardName()) + " is not used",
                         "maxBisections");
    }
    card.maxRotationRatio = reader.real(21, "maxRotationRatio", 20.0);
    card.rotationTolerance = reader.real(23, "rotationTolerance", 20.0);
    card.minIterations = reader.optionalInteger(24, "minIterations");
    if (reader.lastUsedField() > 24)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has no field beyond " + std::string("minIterations"),
                     "minIterations");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearStaticParamsFormat::check(const NonlinearStaticParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "id"};
    if (card.incrementCount && *card.incrementCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "incrementCount"};
    if (!(card.timeStep >= 0.0) || !std::isfinite(card.timeStep))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "timeStep"};
    if (card.stiffnessUpdateStep && *card.stiffnessUpdateStep < -1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be -1 or more",
                           "stiffnessUpdateStep"};
    if (card.maxIterations == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "maxIterations"};
    if (!onlyLettersOf(card.convergenceFlags, CONVERGENCE_LETTERS))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is made of the letters U, P, W, V, N and A",
                           "convergenceFlags"};
    if (!isIntermediateOutput(card.intermediateOutput))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be YES, NO, ALL or a positive integer",
                           "intermediateOutput"};
    if (card.epsilonU && (!(*card.epsilonU != 0.0) || !std::isfinite(*card.epsilonU)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "epsilonU"};
    if (card.epsilonP && !(*card.epsilonP > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "epsilonP"};
    if (card.epsilonW && (!(*card.epsilonW != 0.0) || !std::isfinite(*card.epsilonW)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "epsilonW"};
    if (card.maxDivergences == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "maxDivergences"};
    if (card.maxQuasiNewtonVectors < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "maxQuasiNewtonVectors"};
    if (card.maxLineSearches < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "maxLineSearches"};
    if (!(card.stressSubincrementFraction > 0.0 && card.stressSubincrementFraction < 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.0 and 1.0",
                           "stressSubincrementFraction"};
    if (!(card.lineSearchTolerance > 0.01 && card.lineSearchTolerance < 0.9))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.01 and 0.9",
                           "lineSearchTolerance"};
    if (card.maxBisections <= -10 || card.maxBisections >= 10)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between -10 and 10",
                           "maxBisections"};
    if (!(card.maxRotationRatio > 1.0 && card.maxRotationRatio < 40.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 1.0 and 40.0",
                           "maxRotationRatio"};
    if (!(card.rotationTolerance > 2.0) || !std::isfinite(card.rotationTolerance))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 2.0",
                           "rotationTolerance"};
    if (card.minIterations && *card.minIterations < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "minIterations"};
    return std::nullopt;
}

void NonlinearStaticParamsFormat::write(const NonlinearStaticParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.incrementCount)
        writer.integer(2, *card.incrementCount);
    writer.realUnlessDefault(3, card.timeStep, 0.0);
    if (card.stiffnessMethod != StiffnessMethod::NotGiven)
        writer.text(4, methodWord(card.stiffnessMethod));
    if (card.stiffnessUpdateStep)
        writer.integer(5, *card.stiffnessUpdateStep);
    writer.integerUnlessDefault(6, card.maxIterations, 25);
    if (!card.convergenceFlags.empty())
        writer.text(7, card.convergenceFlags);
    writer.textUnlessDefault(8, card.intermediateOutput, "No");
    // A tolerance is written when it was given (a blank one leaves PARAM,NLTOL in force); MAXQN, MAXLS and MAXBIS
    // follow from KMETHOD or MAXITER when blank: written whenever they are.
    if (card.epsilonU)
        writer.real(9, *card.epsilonU);
    if (card.epsilonP)
        writer.real(10, *card.epsilonP);
    if (card.epsilonW)
        writer.real(11, *card.epsilonW);
    writer.integerUnlessDefault(12, card.maxDivergences, 3);
    writer.integer(13, card.maxQuasiNewtonVectors);
    writer.integer(14, card.maxLineSearches);
    writer.realUnlessDefault(15, card.stressSubincrementFraction, 0.2);
    writer.realUnlessDefault(16, card.lineSearchTolerance, 0.5);
    writer.integer(17, card.maxBisections);
    writer.realUnlessDefault(21, card.maxRotationRatio, 20.0);
    writer.realUnlessDefault(23, card.rotationTolerance, 20.0);
    if (card.minIterations)
        writer.integer(24, *card.minIterations);
}

std::uint32_t NonlinearStaticParamsTable::append(const NonlinearStaticParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_incrementCount.push(blankInteger(card.incrementCount));
    m_timeStep.push(card.timeStep);
    m_stiffnessMethod.push(static_cast<std::uint8_t>(card.stiffnessMethod));
    m_stiffnessUpdateStep.push(blankInteger(card.stiffnessUpdateStep));
    m_maxIterations.push(card.maxIterations);
    m_convergenceFlags.push(card.convergenceFlags);
    m_intermediateOutput.push(card.intermediateOutput);
    m_epsilonU.push(blankReal(card.epsilonU));
    m_epsilonP.push(blankReal(card.epsilonP));
    m_epsilonW.push(blankReal(card.epsilonW));
    m_maxDivergences.push(card.maxDivergences);
    m_maxQuasiNewtonVectors.push(card.maxQuasiNewtonVectors);
    m_maxLineSearches.push(card.maxLineSearches);
    m_stressSubincrementFraction.push(card.stressSubincrementFraction);
    m_lineSearchTolerance.push(card.lineSearchTolerance);
    m_maxBisections.push(card.maxBisections);
    m_maxRotationRatio.push(card.maxRotationRatio);
    m_rotationTolerance.push(card.rotationTolerance);
    m_minIterations.push(blankInteger(card.minIterations));
    return row;
}

NonlinearStaticParamsCard NonlinearStaticParamsTable::card(std::uint32_t row) const
{
    NonlinearStaticParamsCard card;
    card.id = m_id.get(row);
    card.incrementCount = integerOrNone(m_incrementCount.get(row));
    card.timeStep = m_timeStep.get(row);
    card.stiffnessMethod = static_cast<StiffnessMethod>(m_stiffnessMethod.get(row));
    card.stiffnessUpdateStep = integerOrNone(m_stiffnessUpdateStep.get(row));
    card.maxIterations = m_maxIterations.get(row);
    card.convergenceFlags = m_convergenceFlags.get(row);
    card.intermediateOutput = m_intermediateOutput.get(row);
    card.epsilonU = realOrNone(m_epsilonU.get(row));
    card.epsilonP = realOrNone(m_epsilonP.get(row));
    card.epsilonW = realOrNone(m_epsilonW.get(row));
    card.maxDivergences = m_maxDivergences.get(row);
    card.maxQuasiNewtonVectors = m_maxQuasiNewtonVectors.get(row);
    card.maxLineSearches = m_maxLineSearches.get(row);
    card.stressSubincrementFraction = m_stressSubincrementFraction.get(row);
    card.lineSearchTolerance = m_lineSearchTolerance.get(row);
    card.maxBisections = m_maxBisections.get(row);
    card.maxRotationRatio = m_maxRotationRatio.get(row);
    card.rotationTolerance = m_rotationTolerance.get(row);
    card.minIterations = integerOrNone(m_minIterations.get(row));
    return card;
}

void NonlinearStaticParamsTable::store(std::uint32_t row, const NonlinearStaticParamsCard& card)
{
    m_id.set(row, card.id);
    m_incrementCount.set(row, blankInteger(card.incrementCount));
    m_timeStep.set(row, card.timeStep);
    m_stiffnessMethod.set(row, static_cast<std::uint8_t>(card.stiffnessMethod));
    m_stiffnessUpdateStep.set(row, blankInteger(card.stiffnessUpdateStep));
    m_maxIterations.set(row, card.maxIterations);
    m_convergenceFlags.set(row, card.convergenceFlags);
    m_intermediateOutput.set(row, card.intermediateOutput);
    m_epsilonU.set(row, blankReal(card.epsilonU));
    m_epsilonP.set(row, blankReal(card.epsilonP));
    m_epsilonW.set(row, blankReal(card.epsilonW));
    m_maxDivergences.set(row, card.maxDivergences);
    m_maxQuasiNewtonVectors.set(row, card.maxQuasiNewtonVectors);
    m_maxLineSearches.set(row, card.maxLineSearches);
    m_stressSubincrementFraction.set(row, card.stressSubincrementFraction);
    m_lineSearchTolerance.set(row, card.lineSearchTolerance);
    m_maxBisections.set(row, card.maxBisections);
    m_maxRotationRatio.set(row, card.maxRotationRatio);
    m_rotationTolerance.set(row, card.rotationTolerance);
    m_minIterations.set(row, blankInteger(card.minIterations));
}

void NonlinearStaticParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_incrementCount.keepRows(rows);
    m_timeStep.keepRows(rows);
    m_stiffnessMethod.keepRows(rows);
    m_stiffnessUpdateStep.keepRows(rows);
    m_maxIterations.keepRows(rows);
    m_convergenceFlags.keepRows(rows);
    m_intermediateOutput.keepRows(rows);
    m_epsilonU.keepRows(rows);
    m_epsilonP.keepRows(rows);
    m_epsilonW.keepRows(rows);
    m_maxDivergences.keepRows(rows);
    m_maxQuasiNewtonVectors.keepRows(rows);
    m_maxLineSearches.keepRows(rows);
    m_stressSubincrementFraction.keepRows(rows);
    m_lineSearchTolerance.keepRows(rows);
    m_maxBisections.keepRows(rows);
    m_maxRotationRatio.keepRows(rows);
    m_rotationTolerance.keepRows(rows);
    m_minIterations.keepRows(rows);
}

} // namespace oor::model
