#include "oor/model/cards/solution_control/nonlinearTimeSteps/NonlinearTimeStepsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

std::optional<NonlinearTransientMethod> parseMethod(std::string_view text)
{
    if (text.empty())
        return NonlinearTransientMethod::NotGiven;
    if ((text == "Automatic"))
        return NonlinearTransientMethod::Auto;
    if ((text == "Iterative"))
        return NonlinearTransientMethod::Iter;
    if ((text == "Adaptive"))
        return NonlinearTransientMethod::Adapt;
    if ((text == "Semi"))
        return NonlinearTransientMethod::Semi;
    if ((text == "FullNewtonTangent"))
        return NonlinearTransientMethod::Fnt;
    if ((text == "PseudoFullNewtonTangent"))
        return NonlinearTransientMethod::Pfnt;
    return std::nullopt;
}

const char* methodWord(NonlinearTransientMethod method)
{
    switch (method)
    {
    case NonlinearTransientMethod::Auto:
        return "Automatic";
    case NonlinearTransientMethod::Iter:
        return "Iterative";
    case NonlinearTransientMethod::Adapt:
        return "Adaptive";
    case NonlinearTransientMethod::Semi:
        return "Semi";
    case NonlinearTransientMethod::Fnt:
        return "FullNewtonTangent";
    case NonlinearTransientMethod::Pfnt:
        return "PseudoFullNewtonTangent";
    case NonlinearTransientMethod::NotGiven:
        break;
    }
    return "";
}

// The letters of the convergence flags: displacement, load, work, vector component, length, and D
// (CONV = D is UPD).
constexpr std::string_view CONVERGENCE_LETTERS = "UPWVND";

} // namespace

std::optional<NonlinearTimeStepsCard> NonlinearTimeStepsFormat::read(FieldReader& reader)
{
    NonlinearTimeStepsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stepCount = reader.requiredInteger(2, "stepCount");
    card.timeStep = reader.requiredReal(3, "timeStep");
    card.outputInterval = reader.integer(4, "outputInterval", 1);
    const std::string_view methodText = reader.text(5);
    if (const auto method = parseMethod(methodText))
        card.method = *method;
    else
        reader.error("card.bad-value",
                     std::string("method") + " must be AUTO, ITER, ADAPT, SEMI, FNT or PFNT, found \"" +
                         std::string(methodText) + "\"",
                     "method");
    card.stiffnessUpdateStep = reader.optionalInteger(6, "stiffnessUpdateStep");
    if (!card.stiffnessUpdateStep)
        card.stiffnessUpdateStep = nonlinearTimeStepsDefaultStiffnessUpdateStep(card.method);
    card.maxIterations = reader.optionalInteger(7, "maxIterations");
    if (!card.maxIterations)
        card.maxIterations = nonlinearTimeStepsDefaultMaxIterations(card.method);
    card.convergenceFlags = base::toUpperAscii(reader.text(8));
    card.epsilonU = reader.real(9, "epsilonU", nonlinearTimeStepsDefaultEpsilonU(card.method));
    card.epsilonP = reader.optionalReal(10, "epsilonP");
    card.epsilonW = reader.optionalReal(11, "epsilonW");
    card.maxDivergences = reader.integer(12, "maxDivergences", 2);
    card.maxQuasiNewtonVectors =
        reader.integer(13, "maxQuasiNewtonVectors", nonlinearTimeStepsDefaultMaxQuasiNewtonVectors(card.method));
    card.maxLineSearches = reader.integer(14, "maxLineSearches", nonlinearTimeStepsDefaultMaxLineSearches(card.method));
    card.stressSubincrementFraction = reader.real(15, "stressSubincrementFraction", 0.2);
    if (!reader.isBlank(16))
        reader.error("card.bad-value", "field 16 of a " + std::string(reader.cardName()) + " is not used",
                     "stressSubincrementFraction");
    card.maxBisections = reader.optionalInteger(17, "maxBisections");
    if (!card.maxBisections)
        card.maxBisections = nonlinearTimeStepsDefaultMaxBisections(card.maxIterations);
    card.adjustInterval = reader.integer(18, "adjustInterval", 5);
    card.minStepsPerCycle = reader.optionalInteger(19, "minStepsPerCycle");
    card.stepBounds = reader.real(20, "stepBounds", 0.6);
    card.maxRotationRatio = reader.real(21, "maxRotationRatio", 32.0);
    card.displacementTolerance = reader.real(22, "displacementTolerance", 0.1);
    card.rotationTolerance = reader.real(23, "rotationTolerance", 20.0);
    card.minIterations = reader.optionalInteger(24, "minIterations");
    if (reader.lastUsedField() > 24)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("minIterations"),
                     "minIterations");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearTimeStepsFormat::check(const NonlinearTimeStepsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "id"};
    if (card.stepCount < 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an integer of 3 or more",
                           "stepCount"};
    if (!(card.timeStep > 0.0) || !std::isfinite(card.timeStep))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "timeStep"};
    if (card.outputInterval == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "outputInterval"};
    if (card.stiffnessUpdateStep)
    {
        // FNT and PFNT take -1 or 1 (a blank is the third choice); the other methods a positive count.
        const bool newton =
            card.method == NonlinearTransientMethod::Fnt || card.method == NonlinearTransientMethod::Pfnt;
        if (newton ? (*card.stiffnessUpdateStep != -1 && *card.stiffnessUpdateStep != 1)
                   : *card.stiffnessUpdateStep < 1)
            return CardProblem{
                "card.bad-value",
                newton ? std::string(CardProblem::FIELD) + " must be -1, 1 or blank for the FNT and PFNT methods"
                       : std::string(CardProblem::FIELD) + " must be a positive integer",
                "stiffnessUpdateStep"};
    }
    if (card.maxIterations && *card.maxIterations == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "maxIterations"};
    if (!onlyLettersOf(card.convergenceFlags, CONVERGENCE_LETTERS))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is made of the letters U, P, W, V, N and D",
                           "convergenceFlags"};
    if (!(card.epsilonU != 0.0) || !std::isfinite(card.epsilonU))
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
    if (card.maxBisections && (*card.maxBisections <= -9 || *card.maxBisections >= 9))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between -9 and 9",
                           "maxBisections"};
    if (card.adjustInterval < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "adjustInterval"};
    if (card.minStepsPerCycle && (*card.minStepsPerCycle <= 10 || *card.minStepsPerCycle >= 200))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 10 and 200",
                           "minStepsPerCycle"};
    if (!(card.stepBounds > 0.1 && card.stepBounds < 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.1 and 1.0",
                           "stepBounds"};
    if (!(card.maxRotationRatio > 1.0) || !std::isfinite(card.maxRotationRatio))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 1.0",
                           "maxRotationRatio"};
    if (!(card.displacementTolerance > 0.001 && card.displacementTolerance < 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.001 and 1.0",
                           "displacementTolerance"};
    if (!(card.rotationTolerance > 2.0) || !std::isfinite(card.rotationTolerance))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 2.0",
                           "rotationTolerance"};
    if (card.minIterations && *card.minIterations < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "minIterations"};
    return std::nullopt;
}

void NonlinearTimeStepsFormat::write(const NonlinearTimeStepsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.stepCount);
    writer.real(3, card.timeStep);
    writer.integerUnlessDefault(4, card.outputInterval, 1);
    if (card.method != NonlinearTransientMethod::NotGiven)
        writer.text(5, methodWord(card.method));
    if (card.stiffnessUpdateStep)
        writer.integer(6, *card.stiffnessUpdateStep);
    if (card.maxIterations)
        writer.integer(7, *card.maxIterations);
    if (!card.convergenceFlags.empty())
        writer.text(8, card.convergenceFlags);
    // EPSU, MAXQN and MAXLS follow from METHOD when blank (PFNT): written whenever they are.
    writer.real(9, card.epsilonU);
    if (card.epsilonP)
        writer.real(10, *card.epsilonP);
    if (card.epsilonW)
        writer.real(11, *card.epsilonW);
    writer.integerUnlessDefault(12, card.maxDivergences, 2);
    writer.integer(13, card.maxQuasiNewtonVectors);
    writer.integer(14, card.maxLineSearches);
    writer.realUnlessDefault(15, card.stressSubincrementFraction, 0.2);
    if (card.maxBisections) // from MAXITER when blank: written whenever it has a value
        writer.integer(17, *card.maxBisections);
    writer.integerUnlessDefault(18, card.adjustInterval, 5);
    if (card.minStepsPerCycle)
        writer.integer(19, *card.minStepsPerCycle);
    writer.realUnlessDefault(20, card.stepBounds, 0.6);
    writer.realUnlessDefault(21, card.maxRotationRatio, 32.0);
    writer.realUnlessDefault(22, card.displacementTolerance, 0.1);
    writer.realUnlessDefault(23, card.rotationTolerance, 20.0);
    if (card.minIterations)
        writer.integer(24, *card.minIterations);
}

std::uint32_t NonlinearTimeStepsTable::append(const NonlinearTimeStepsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stepCount.push(card.stepCount);
    m_timeStep.push(card.timeStep);
    m_outputInterval.push(card.outputInterval);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_stiffnessUpdateStep.push(blankInteger(card.stiffnessUpdateStep));
    m_maxIterations.push(blankInteger(card.maxIterations));
    m_convergenceFlags.push(card.convergenceFlags);
    m_epsilonU.push(card.epsilonU);
    m_epsilonP.push(blankReal(card.epsilonP));
    m_epsilonW.push(blankReal(card.epsilonW));
    m_maxDivergences.push(card.maxDivergences);
    m_maxQuasiNewtonVectors.push(card.maxQuasiNewtonVectors);
    m_maxLineSearches.push(card.maxLineSearches);
    m_stressSubincrementFraction.push(card.stressSubincrementFraction);
    m_maxBisections.push(blankInteger(card.maxBisections));
    m_adjustInterval.push(card.adjustInterval);
    m_minStepsPerCycle.push(blankInteger(card.minStepsPerCycle));
    m_stepBounds.push(card.stepBounds);
    m_maxRotationRatio.push(card.maxRotationRatio);
    m_displacementTolerance.push(card.displacementTolerance);
    m_rotationTolerance.push(card.rotationTolerance);
    m_minIterations.push(blankInteger(card.minIterations));
    return row;
}

NonlinearTimeStepsCard NonlinearTimeStepsTable::card(std::uint32_t row) const
{
    NonlinearTimeStepsCard card;
    card.id = m_id.get(row);
    card.stepCount = m_stepCount.get(row);
    card.timeStep = m_timeStep.get(row);
    card.outputInterval = m_outputInterval.get(row);
    card.method = static_cast<NonlinearTransientMethod>(m_method.get(row));
    card.stiffnessUpdateStep = integerOrNone(m_stiffnessUpdateStep.get(row));
    card.maxIterations = integerOrNone(m_maxIterations.get(row));
    card.convergenceFlags = m_convergenceFlags.get(row);
    card.epsilonU = m_epsilonU.get(row);
    card.epsilonP = realOrNone(m_epsilonP.get(row));
    card.epsilonW = realOrNone(m_epsilonW.get(row));
    card.maxDivergences = m_maxDivergences.get(row);
    card.maxQuasiNewtonVectors = m_maxQuasiNewtonVectors.get(row);
    card.maxLineSearches = m_maxLineSearches.get(row);
    card.stressSubincrementFraction = m_stressSubincrementFraction.get(row);
    card.maxBisections = integerOrNone(m_maxBisections.get(row));
    card.adjustInterval = m_adjustInterval.get(row);
    card.minStepsPerCycle = integerOrNone(m_minStepsPerCycle.get(row));
    card.stepBounds = m_stepBounds.get(row);
    card.maxRotationRatio = m_maxRotationRatio.get(row);
    card.displacementTolerance = m_displacementTolerance.get(row);
    card.rotationTolerance = m_rotationTolerance.get(row);
    card.minIterations = integerOrNone(m_minIterations.get(row));
    return card;
}

void NonlinearTimeStepsTable::store(std::uint32_t row, const NonlinearTimeStepsCard& card)
{
    m_id.set(row, card.id);
    m_stepCount.set(row, card.stepCount);
    m_timeStep.set(row, card.timeStep);
    m_outputInterval.set(row, card.outputInterval);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_stiffnessUpdateStep.set(row, blankInteger(card.stiffnessUpdateStep));
    m_maxIterations.set(row, blankInteger(card.maxIterations));
    m_convergenceFlags.set(row, card.convergenceFlags);
    m_epsilonU.set(row, card.epsilonU);
    m_epsilonP.set(row, blankReal(card.epsilonP));
    m_epsilonW.set(row, blankReal(card.epsilonW));
    m_maxDivergences.set(row, card.maxDivergences);
    m_maxQuasiNewtonVectors.set(row, card.maxQuasiNewtonVectors);
    m_maxLineSearches.set(row, card.maxLineSearches);
    m_stressSubincrementFraction.set(row, card.stressSubincrementFraction);
    m_maxBisections.set(row, blankInteger(card.maxBisections));
    m_adjustInterval.set(row, card.adjustInterval);
    m_minStepsPerCycle.set(row, blankInteger(card.minStepsPerCycle));
    m_stepBounds.set(row, card.stepBounds);
    m_maxRotationRatio.set(row, card.maxRotationRatio);
    m_displacementTolerance.set(row, card.displacementTolerance);
    m_rotationTolerance.set(row, card.rotationTolerance);
    m_minIterations.set(row, blankInteger(card.minIterations));
}

void NonlinearTimeStepsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stepCount.keepRows(rows);
    m_timeStep.keepRows(rows);
    m_outputInterval.keepRows(rows);
    m_method.keepRows(rows);
    m_stiffnessUpdateStep.keepRows(rows);
    m_maxIterations.keepRows(rows);
    m_convergenceFlags.keepRows(rows);
    m_epsilonU.keepRows(rows);
    m_epsilonP.keepRows(rows);
    m_epsilonW.keepRows(rows);
    m_maxDivergences.keepRows(rows);
    m_maxQuasiNewtonVectors.keepRows(rows);
    m_maxLineSearches.keepRows(rows);
    m_stressSubincrementFraction.keepRows(rows);
    m_maxBisections.keepRows(rows);
    m_adjustInterval.keepRows(rows);
    m_minStepsPerCycle.keepRows(rows);
    m_stepBounds.keepRows(rows);
    m_maxRotationRatio.keepRows(rows);
    m_displacementTolerance.keepRows(rows);
    m_rotationTolerance.keepRows(rows);
    m_minIterations.keepRows(rows);
}

} // namespace oor::model
