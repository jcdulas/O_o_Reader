#include "oor/model/cards/materials/orthoMat2DFailure/OrthoMat2DFailureTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELD_THEORY = 2;
constexpr int FIELD_HISTORY_VARIABLE_COUNT = 3;
constexpr int FIELD_SHEAR_STRENGTH = 4;
constexpr int FIELD_NONLINEAR_SHEAR = 5;
constexpr int FIELD_TRANSVERSE_SHEAR = 6;
constexpr int FIELD_INTERACTION_TERM12 = 7;
constexpr int FIELD_TENSION_STRENGTH1 = 9; // XT, XC, YT, YC: fields 9 to 12
constexpr int FIELD_DEGRADATION = 13;
constexpr int FIELD_DEGRADATION_PARAMETER = 14;
constexpr int FIELD_DEGRADATION_START = 15;
constexpr int FIRST_MODE_FIELD = 17;        // FBTEN to MXSHR: fields 17 to 21
constexpr int FIRST_DEGRADATION_FIELD = 33; // PRDFT to PRDSH: fields 33 to 37
constexpr int MAX_HISTORY_VARIABLES = 9;

constexpr const char* MODE_NAMES[OrthoMat2DFailureCard::MODES] = {"fiberTensionTheory", "fiberCompressionTheory",
                                                                  "matrixTensionTheory", "matrixCompressionTheory",
                                                                  "matrixShearTheory"};
constexpr const char* DEGRADATION_NAMES[OrthoMat2DFailureCard::MODES] = {
    "fiberTensionDegradation", "fiberCompressionDegradation", "matrixTensionDegradation",
    "matrixCompressionDegradation", "matrixShearDegradation"};
constexpr const char* DEGRADATION_DEFAULTS[OrthoMat2DFailureCard::MODES] = {"1111", "1010", "0110", "0110", "0001"};

// True for the theories that can define a failure mode of the COMBINAT theory.
bool definesAMode(OrthoMat2DFailureTheory theory)
{
    return theory != OrthoMat2DFailureTheory::None && theory != OrthoMat2DFailureTheory::Combination;
}

bool isDegradationWord(const std::string& word)
{
    if (word.size() != 4)
        return false;
    for (const char character : word)
    {
        if (character != '0' && character != '1')
            return false;
    }
    return true;
}

} // namespace

const char* OrthoMat2DFailureFormat::theoryWord(OrthoMat2DFailureTheory theory) noexcept
{
    switch (theory)
    {
    case OrthoMat2DFailureTheory::None:
        return "";
    case OrthoMat2DFailureTheory::Hill:
        return "Hill";
    case OrthoMat2DFailureTheory::Tsai:
        return "TsaiWu";
    case OrthoMat2DFailureTheory::ModifiedTsai:
        return "ModifiedTsaiWu";
    case OrthoMat2DFailureTheory::MaxStress:
        return "StressTheory";
    case OrthoMat2DFailureTheory::Chang:
        return "ChangChangFailure";
    case OrthoMat2DFailureTheory::Combination:
        return "CombinedFailure";
    case OrthoMat2DFailureTheory::Hashin:
        return "HashinFailure";
    }
    return "";
}

std::optional<OrthoMat2DFailureTheory> OrthoMat2DFailureFormat::parseTheory(std::string_view text) noexcept
{
    for (const OrthoMat2DFailureTheory theory :
         {OrthoMat2DFailureTheory::None, OrthoMat2DFailureTheory::Hill, OrthoMat2DFailureTheory::Tsai,
          OrthoMat2DFailureTheory::ModifiedTsai, OrthoMat2DFailureTheory::MaxStress, OrthoMat2DFailureTheory::Chang,
          OrthoMat2DFailureTheory::Combination, OrthoMat2DFailureTheory::Hashin})
    {
        if ((text == theoryWord(theory)))
            return theory;
    }
    return std::nullopt;
}

const char* OrthoMat2DFailureFormat::shearFailureWord(OrthoMat2DFailureShearFailure value) noexcept
{
    return value == OrthoMat2DFailureShearFailure::Element ? "Elements" : "SublaminateShear";
}

std::optional<OrthoMat2DFailureShearFailure> OrthoMat2DFailureFormat::parseShearFailure(std::string_view text) noexcept
{
    if ((text == "SublaminateShear"))
        return OrthoMat2DFailureShearFailure::Sublayer;
    if ((text == "Elements"))
        return OrthoMat2DFailureShearFailure::Element;
    return std::nullopt;
}

const char* OrthoMat2DFailureFormat::degradationWord(OrthoMat2DFailureDegradation value) noexcept
{
    switch (value)
    {
    case OrthoMat2DFailureDegradation::Steps:
        return "Steps";
    case OrthoMat2DFailureDegradation::Time:
        return "Time";
    case OrthoMat2DFailureDegradation::Velocity:
        return "Velocity";
    }
    return "Steps";
}

std::optional<OrthoMat2DFailureDegradation> OrthoMat2DFailureFormat::parseDegradation(std::string_view text) noexcept
{
    for (const OrthoMat2DFailureDegradation value :
         {OrthoMat2DFailureDegradation::Steps, OrthoMat2DFailureDegradation::Time,
          OrthoMat2DFailureDegradation::Velocity})
    {
        if ((text == degradationWord(value)))
            return value;
    }
    return std::nullopt;
}

const char* OrthoMat2DFailureFormat::degradationStartWord(OrthoMat2DFailureDegradationStart value) noexcept
{
    return value == OrthoMat2DFailureDegradationStart::All ? "All" : "IndividualDegradation";
}

std::optional<OrthoMat2DFailureDegradationStart> OrthoMat2DFailureFormat::parseDegradationStart(
    std::string_view text) noexcept
{
    if ((text == "IndividualDegradation"))
        return OrthoMat2DFailureDegradationStart::Individual;
    if ((text == "All"))
        return OrthoMat2DFailureDegradationStart::All;
    return std::nullopt;
}

std::optional<OrthoMat2DFailureCard> OrthoMat2DFailureFormat::read(FieldReader& reader)
{
    OrthoMat2DFailureCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const auto theory = parseTheory(reader.text(FIELD_THEORY)))
        card.theory = *theory;
    else
        reader.error("card.bad-value",
                     std::string("theory") +
                         " must be blank, HILL, TSAI, MODTSAI, STRSS, CHANG, COMBINAT or HASHIN, not \"" +
                         std::string(reader.text(FIELD_THEORY)) + "\"",
                     "theory");
    card.historyVariableCount = reader.integer(FIELD_HISTORY_VARIABLE_COUNT, "historyVariableCount", 0);
    card.shearStrength = reader.requiredReal(FIELD_SHEAR_STRENGTH, "shearStrength");
    card.nonlinearShearCoefficient = reader.real(FIELD_NONLINEAR_SHEAR, "nonlinearShearCoefficient", 0.0);
    if (const std::string_view word = reader.text(FIELD_TRANSVERSE_SHEAR); !word.empty())
    {
        if (const auto value = parseShearFailure(word))
            card.transverseShear = *value;
        else
            reader.error("card.bad-value",
                         std::string("transverseShear") + " must be ELEM or SUBL, not \"" + std::string(word) + "\"",
                         "transverseShear");
    }
    card.interactionTerm12 = reader.real(FIELD_INTERACTION_TERM12, "interactionTerm12", 0.0);
    card.tensionStrength1 = reader.real(FIELD_TENSION_STRENGTH1, "tensionStrength1", 0.0);
    card.compressionStrength1 = reader.real(FIELD_TENSION_STRENGTH1 + 1, "compressionStrength1", 0.0);
    card.tensionStrength2 = reader.real(FIELD_TENSION_STRENGTH1 + 2, "tensionStrength2", 0.0);
    card.compressionStrength2 = reader.real(FIELD_TENSION_STRENGTH1 + 3, "compressionStrength2", 0.0);
    if (const std::string_view word = reader.text(FIELD_DEGRADATION); !word.empty())
    {
        if (const auto value = parseDegradation(word))
            card.degradation = *value;
        else
            reader.error(
                "card.bad-value",
                std::string("degradation") + " must be STEPS, TIME or VELOC, not \"" + std::string(word) + "\"",
                "degradation");
    }
    card.degradationParameter = reader.real(FIELD_DEGRADATION_PARAMETER, "degradationParameter", 100.0);
    if (const std::string_view word = reader.text(FIELD_DEGRADATION_START); !word.empty())
    {
        if (const auto value = parseDegradationStart(word))
            card.degradationStart = *value;
        else
            reader.error("card.bad-value",
                         std::string("degradationStart") + " must be INDV or ALL, not \"" + std::string(word) + "\"",
                         "degradationStart");
    }
    for (std::size_t index = 0; index < card.modes.size(); ++index)
    {
        const std::string_view word = reader.text(FIRST_MODE_FIELD + static_cast<int>(index));
        const auto theory = parseTheory(word);
        if (!theory || !(definesAMode(*theory) || *theory == OrthoMat2DFailureTheory::None))
            reader.error("card.bad-value",
                         std::string(MODE_NAMES[index]) + " must name a failure theory (not COMBINAT), not \"" +
                             std::string(word) + "\"",
                         MODE_NAMES[index]);
        else
            card.modes[index] = *theory;
        const std::string_view degradation = reader.text(FIRST_DEGRADATION_FIELD + static_cast<int>(index));
        if (!degradation.empty())
            card.propertyDegradation[index] = std::string(degradation);
    }
    requireBlankFields(reader, {8, 16, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
                       FIRST_DEGRADATION_FIELD + static_cast<int>(OrthoMat2DFailureCard::MODES) - 1);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat2DFailureFormat::check(const OrthoMat2DFailureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.historyVariableCount < 0 || card.historyVariableCount > MAX_HISTORY_VARIABLES)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be from 0 to 9",
                           "historyVariableCount"};
    if (!(card.shearStrength > 0.0) || !std::isfinite(card.shearStrength))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "shearStrength"};
    if (!(card.nonlinearShearCoefficient >= 0.0) || !std::isfinite(card.nonlinearShearCoefficient))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "nonlinearShearCoefficient"};
    if (!std::isfinite(card.interactionTerm12))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "interactionTerm12"};
    if (!(card.tensionStrength1 >= 0.0 && card.compressionStrength1 >= 0.0 && card.tensionStrength2 >= 0.0 &&
          card.compressionStrength2 >= 0.0) ||
        !std::isfinite(card.tensionStrength1) || !std::isfinite(card.compressionStrength1) ||
        !std::isfinite(card.tensionStrength2) || !std::isfinite(card.compressionStrength2))
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("tensionStrength1") + ", " +
                               CardProblem::fieldNamed("compressionStrength1") + ", " +
                               CardProblem::fieldNamed("tensionStrength2") + " and " +
                               CardProblem::fieldNamed("compressionStrength2") + " must not be negative",
                           "tensionStrength1"};
    // the failure stresses are required when a failure theory is selected.
    if (card.theory != OrthoMat2DFailureTheory::None &&
        !(card.tensionStrength1 > 0.0 && card.compressionStrength1 > 0.0 && card.tensionStrength2 > 0.0 &&
          card.compressionStrength2 > 0.0))
        return CardProblem{
            "card.bad-value",
            CardProblem::fieldNamed("tensionStrength1") + ", " + CardProblem::fieldNamed("compressionStrength1") +
                ", " + CardProblem::fieldNamed("tensionStrength2") + " and " +
                CardProblem::fieldNamed("compressionStrength2") + " are required when a failure theory is selected",
            "tensionStrength1"};
    if (!(card.degradationParameter > 0.0) || !std::isfinite(card.degradationParameter))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "degradationParameter"};
    for (std::size_t index = 0; index < card.modes.size(); ++index)
    {
        if (card.modes[index] == OrthoMat2DFailureTheory::Combination)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be COMBINAT",
                               MODE_NAMES[index]};
        // the individual failure modes are data of the COMBINAT theory.
        if (card.theory == OrthoMat2DFailureTheory::Combination && card.modes[index] == OrthoMat2DFailureTheory::None)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is required when " +
                                   CardProblem::fieldNamed("theory") + " is COMBINAT",
                               MODE_NAMES[index]};
        if (!isDegradationWord(card.propertyDegradation[index]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be four digits, each 0 or 1",
                               DEGRADATION_NAMES[index]};
    }
    return std::nullopt;
}

void OrthoMat2DFailureFormat::write(const OrthoMat2DFailureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.theory != OrthoMat2DFailureTheory::None)
        writer.text(FIELD_THEORY, theoryWord(card.theory));
    writer.integerUnlessDefault(FIELD_HISTORY_VARIABLE_COUNT, card.historyVariableCount, 0);
    writer.real(FIELD_SHEAR_STRENGTH, card.shearStrength);
    writer.realUnlessDefault(FIELD_NONLINEAR_SHEAR, card.nonlinearShearCoefficient, 0.0);
    writer.textUnlessDefault(FIELD_TRANSVERSE_SHEAR, shearFailureWord(card.transverseShear), "SublaminateShear");
    writer.realUnlessDefault(FIELD_INTERACTION_TERM12, card.interactionTerm12, 0.0);
    writer.realUnlessDefault(FIELD_TENSION_STRENGTH1, card.tensionStrength1, 0.0);
    writer.realUnlessDefault(FIELD_TENSION_STRENGTH1 + 1, card.compressionStrength1, 0.0);
    writer.realUnlessDefault(FIELD_TENSION_STRENGTH1 + 2, card.tensionStrength2, 0.0);
    writer.realUnlessDefault(FIELD_TENSION_STRENGTH1 + 3, card.compressionStrength2, 0.0);
    writer.textUnlessDefault(FIELD_DEGRADATION, degradationWord(card.degradation), "Steps");
    writer.realUnlessDefault(FIELD_DEGRADATION_PARAMETER, card.degradationParameter, 100.0);
    writer.textUnlessDefault(FIELD_DEGRADATION_START, degradationStartWord(card.degradationStart),
                             "IndividualDegradation");
    for (std::size_t index = 0; index < card.modes.size(); ++index)
    {
        if (card.modes[index] != OrthoMat2DFailureTheory::None)
            writer.text(FIRST_MODE_FIELD + static_cast<int>(index), theoryWord(card.modes[index]));
        writer.textUnlessDefault(FIRST_DEGRADATION_FIELD + static_cast<int>(index), card.propertyDegradation[index],
                                 DEGRADATION_DEFAULTS[index]);
    }
}

std::uint32_t OrthoMat2DFailureTable::append(const OrthoMat2DFailureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void OrthoMat2DFailureTable::store(std::uint32_t row, const OrthoMat2DFailureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void OrthoMat2DFailureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void OrthoMat2DFailureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void OrthoMat2DFailureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT8 that the card extends.
    OrthoMat2DFailureCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
