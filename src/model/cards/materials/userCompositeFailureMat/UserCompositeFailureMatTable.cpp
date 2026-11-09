#include "oor/model/cards/materials/userCompositeFailureMat/UserCompositeFailureMatTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_GROUP = 2;
constexpr int FIELD_USER_ROUTINE_NAME = 3;
constexpr int FIELD_YOUNGS_MODULUS1 = 9;            // E1, E2, N12, G12, G1Z, G2Z, RHO: fields 9 to 15
constexpr int FIELD_HISTORY_VARIABLE_COUNT = 17;    // NV, S, ALPHA, TRSFAIL, F12: fields 17 to 21
constexpr int FIELD_TENSION_STRENGTH1 = 25;         // XT, XC, YT, YC, PFD, VALUE, PFDST: fields 25 to 31
constexpr int FIELD_FIBER_TENSION_THEORY = 33;      // FBTEN, FBCOM, MXTEN, MXCOM, MXSHR: fields 33 to 37
constexpr int FIELD_FIBER_TENSION_DEGRADATION = 41; // PRDFT, PRDFC, PRDMT, PRDMC, PRDSH: fields 41 to 45
constexpr int LAST_FIELD = 45;

// The five degradation words are four digits of 0 and 1 each.
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

// A failure mode may be blank; a word is a name of at most eight characters.
std::optional<CardProblem> checkMode(const std::string& word, const char* field)
{
    if (word.empty())
        return std::nullopt;
    return checkNameWord(word, field);
}

// A degradation word that the deck leaves blank is the default of its field.
std::string readDegradation(FieldReader& reader, int field, const char* defaultWord)
{
    const std::string_view word = reader.text(field);
    return word.empty() ? std::string(defaultWord) : std::string(word);
}

std::optional<CardProblem> checkDegradation(const std::string& word, const char* field)
{
    if (!isDegradationWord(word))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be four digits, each 0 or 1",
                           field};
    return std::nullopt;
}

} // namespace

std::optional<UserCompositeFailureMatCard> UserCompositeFailureMatFormat::read(FieldReader& reader)
{
    UserCompositeFailureMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.isBlank(FIELD_GROUP))
        reader.error("card.missing-field",
                     std::string("groupName") + " is required: the group name of the CONNECT SERVICE statement",
                     "groupName");
    card.groupName = std::string(reader.text(FIELD_GROUP));
    card.userRoutineName = std::string(reader.text(FIELD_USER_ROUTINE_NAME));
    card.youngsModulus1 = reader.requiredReal(FIELD_YOUNGS_MODULUS1, "youngsModulus1");
    card.youngsModulus2 = reader.requiredReal(FIELD_YOUNGS_MODULUS1 + 1, "youngsModulus2");
    card.poissonRatio12 = reader.requiredReal(FIELD_YOUNGS_MODULUS1 + 2, "poissonRatio12");
    card.shearModulus12 = reader.requiredReal(FIELD_YOUNGS_MODULUS1 + 3, "shearModulus12");
    card.shearModulus1Z = reader.real(FIELD_YOUNGS_MODULUS1 + 4, "shearModulus1Z",
                                      card.shearModulus12); // format reference: G1Z defaults to G12
    card.shearModulus2Z =
        reader.real(FIELD_YOUNGS_MODULUS1 + 5, "shearModulus2Z", card.shearModulus12); // the same as the MAT8
    card.density = reader.requiredReal(FIELD_YOUNGS_MODULUS1 + 6, "density");
    card.historyVariableCount = reader.integer(FIELD_HISTORY_VARIABLE_COUNT, "historyVariableCount", 0);
    card.shearStrength = reader.real(FIELD_HISTORY_VARIABLE_COUNT + 1, "shearStrength", 0.0);
    card.nonlinearShearCoefficient = reader.real(FIELD_HISTORY_VARIABLE_COUNT + 2, "nonlinearShearCoefficient", 0.0);
    card.transverseShear =
        readKeyword(reader, FIELD_HISTORY_VARIABLE_COUNT + 3, "transverseShear",
                    USER_COMPOSITE_FAILURE_SHEAR_FAILURE_WORDS, UserCompositeFailureMatShearFailure::Sublayer);
    card.interactionTerm12 = reader.real(FIELD_HISTORY_VARIABLE_COUNT + 4, "interactionTerm12", 0.0);
    card.tensionStrength1 = reader.requiredReal(FIELD_TENSION_STRENGTH1, "tensionStrength1");
    card.compressionStrength1 = reader.requiredReal(FIELD_TENSION_STRENGTH1 + 1, "compressionStrength1");
    card.tensionStrength2 = reader.requiredReal(FIELD_TENSION_STRENGTH1 + 2, "tensionStrength2");
    card.compressionStrength2 = reader.requiredReal(FIELD_TENSION_STRENGTH1 + 3, "compressionStrength2");
    card.degradation = readKeyword(reader, FIELD_TENSION_STRENGTH1 + 4, "degradation",
                                   USER_COMPOSITE_FAILURE_DEGRADATION_WORDS, UserCompositeFailureMatDegradation::Steps);
    card.degradationParameter =
        reader.real(FIELD_TENSION_STRENGTH1 + 5, "degradationParameter", UserCompositeFailureMatCard::DEFAULT_VALUE);
    card.degradationStart = readKeyword(reader, FIELD_TENSION_STRENGTH1 + 6, "degradationStart",
                                        USER_COMPOSITE_FAILURE_DEGRADATION_START_WORDS,
                                        UserCompositeFailureMatDegradationStart::Individual);
    card.fiberTensionTheory = std::string(reader.text(FIELD_FIBER_TENSION_THEORY));
    card.fiberCompressionTheory = std::string(reader.text(FIELD_FIBER_TENSION_THEORY + 1));
    card.matrixTensionTheory = std::string(reader.text(FIELD_FIBER_TENSION_THEORY + 2));
    card.matrixCompressionTheory = std::string(reader.text(FIELD_FIBER_TENSION_THEORY + 3));
    card.matrixShearTheory = std::string(reader.text(FIELD_FIBER_TENSION_THEORY + 4));
    card.fiberTensionDegradation = readDegradation(reader, FIELD_FIBER_TENSION_DEGRADATION, "1111");
    card.fiberCompressionDegradation = readDegradation(reader, FIELD_FIBER_TENSION_DEGRADATION + 1, "1010");
    card.matrixTensionDegradation = readDegradation(reader, FIELD_FIBER_TENSION_DEGRADATION + 2, "0110");
    card.matrixCompressionDegradation = readDegradation(reader, FIELD_FIBER_TENSION_DEGRADATION + 3, "0110");
    card.matrixShearDegradation = readDegradation(reader, FIELD_FIBER_TENSION_DEGRADATION + 4, "0001");
    // The cells the format draws empty: the rest of the first line, the end of each line, and what follows the last
    // field.
    requireBlankFields(reader, {4, 5, 6, 7, 8, 16, 22, 23, 24, 32, 38, 39, 40}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserCompositeFailureMatFormat::check(const UserCompositeFailureMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (!card.userRoutineName.empty())
    {
        if (const auto problem = checkNameWord(card.userRoutineName, "userRoutineName"))
            return problem;
    }
    if (const auto problem = checkReal(card.youngsModulus1, "youngsModulus1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.youngsModulus2, "youngsModulus2", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.poissonRatio12, "poissonRatio12", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.shearModulus12, "shearModulus12", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.shearModulus1Z, "shearModulus1Z", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.shearModulus2Z, "shearModulus2Z", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.density, "density", RealRule::Positive))
        return problem;
    if (card.historyVariableCount < 0 || card.historyVariableCount > UserCompositeFailureMatCard::MAX_HISTORY_VARIABLES)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be from 0 to 9",
                           "historyVariableCount"};
    if (const auto problem = checkReal(card.shearStrength, "shearStrength", RealRule::NonNegative))
        return problem;
    if (const auto problem =
            checkReal(card.nonlinearShearCoefficient, "nonlinearShearCoefficient", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.interactionTerm12, "interactionTerm12", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.tensionStrength1, "tensionStrength1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.compressionStrength1, "compressionStrength1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.tensionStrength2, "tensionStrength2", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.compressionStrength2, "compressionStrength2", RealRule::Positive))
        return problem;
    // The MAT8A has the same field (the format says the definitions are the same): a number, greater than zero.
    if (const auto problem = checkReal(card.degradationParameter, "degradationParameter", RealRule::Positive))
        return problem;
    if (const auto problem = checkMode(card.fiberTensionTheory, "fiberTensionTheory"))
        return problem;
    if (const auto problem = checkMode(card.fiberCompressionTheory, "fiberCompressionTheory"))
        return problem;
    if (const auto problem = checkMode(card.matrixTensionTheory, "matrixTensionTheory"))
        return problem;
    if (const auto problem = checkMode(card.matrixCompressionTheory, "matrixCompressionTheory"))
        return problem;
    if (const auto problem = checkMode(card.matrixShearTheory, "matrixShearTheory"))
        return problem;
    if (const auto problem = checkDegradation(card.fiberTensionDegradation, "fiberTensionDegradation"))
        return problem;
    if (const auto problem = checkDegradation(card.fiberCompressionDegradation, "fiberCompressionDegradation"))
        return problem;
    if (const auto problem = checkDegradation(card.matrixTensionDegradation, "matrixTensionDegradation"))
        return problem;
    if (const auto problem = checkDegradation(card.matrixCompressionDegradation, "matrixCompressionDegradation"))
        return problem;
    return checkDegradation(card.matrixShearDegradation, "matrixShearDegradation");
}

void UserCompositeFailureMatFormat::write(const UserCompositeFailureMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(FIELD_GROUP, card.groupName);
    if (!card.userRoutineName.empty())
        writer.text(FIELD_USER_ROUTINE_NAME, card.userRoutineName);
    writer.real(FIELD_YOUNGS_MODULUS1, card.youngsModulus1);
    writer.real(FIELD_YOUNGS_MODULUS1 + 1, card.youngsModulus2);
    writer.real(FIELD_YOUNGS_MODULUS1 + 2, card.poissonRatio12);
    writer.real(FIELD_YOUNGS_MODULUS1 + 3, card.shearModulus12);
    writer.real(FIELD_YOUNGS_MODULUS1 + 4,
                card.shearModulus1Z); // their default depends on G12: stored resolved, so always written
    writer.real(FIELD_YOUNGS_MODULUS1 + 5, card.shearModulus2Z);
    writer.real(FIELD_YOUNGS_MODULUS1 + 6, card.density);
    writer.integerUnlessDefault(FIELD_HISTORY_VARIABLE_COUNT, card.historyVariableCount, 0);
    writer.realUnlessDefault(FIELD_HISTORY_VARIABLE_COUNT + 1, card.shearStrength, 0.0);
    writer.realUnlessDefault(FIELD_HISTORY_VARIABLE_COUNT + 2, card.nonlinearShearCoefficient, 0.0);
    writeKeywordUnlessDefault(writer, FIELD_HISTORY_VARIABLE_COUNT + 3, USER_COMPOSITE_FAILURE_SHEAR_FAILURE_WORDS,
                              card.transverseShear, UserCompositeFailureMatShearFailure::Sublayer);
    writer.realUnlessDefault(FIELD_HISTORY_VARIABLE_COUNT + 4, card.interactionTerm12, 0.0);
    writer.real(FIELD_TENSION_STRENGTH1, card.tensionStrength1);
    writer.real(FIELD_TENSION_STRENGTH1 + 1, card.compressionStrength1);
    writer.real(FIELD_TENSION_STRENGTH1 + 2, card.tensionStrength2);
    writer.real(FIELD_TENSION_STRENGTH1 + 3, card.compressionStrength2);
    writeKeywordUnlessDefault(writer, FIELD_TENSION_STRENGTH1 + 4, USER_COMPOSITE_FAILURE_DEGRADATION_WORDS,
                              card.degradation, UserCompositeFailureMatDegradation::Steps);
    writer.realUnlessDefault(FIELD_TENSION_STRENGTH1 + 5, card.degradationParameter,
                             UserCompositeFailureMatCard::DEFAULT_VALUE);
    writeKeywordUnlessDefault(writer, FIELD_TENSION_STRENGTH1 + 6, USER_COMPOSITE_FAILURE_DEGRADATION_START_WORDS,
                              card.degradationStart, UserCompositeFailureMatDegradationStart::Individual);
    if (!card.fiberTensionTheory.empty())
        writer.text(FIELD_FIBER_TENSION_THEORY, card.fiberTensionTheory);
    if (!card.fiberCompressionTheory.empty())
        writer.text(FIELD_FIBER_TENSION_THEORY + 1, card.fiberCompressionTheory);
    if (!card.matrixTensionTheory.empty())
        writer.text(FIELD_FIBER_TENSION_THEORY + 2, card.matrixTensionTheory);
    if (!card.matrixCompressionTheory.empty())
        writer.text(FIELD_FIBER_TENSION_THEORY + 3, card.matrixCompressionTheory);
    if (!card.matrixShearTheory.empty())
        writer.text(FIELD_FIBER_TENSION_THEORY + 4, card.matrixShearTheory);
    writer.textUnlessDefault(FIELD_FIBER_TENSION_DEGRADATION, card.fiberTensionDegradation, "1111");
    writer.textUnlessDefault(FIELD_FIBER_TENSION_DEGRADATION + 1, card.fiberCompressionDegradation, "1010");
    writer.textUnlessDefault(FIELD_FIBER_TENSION_DEGRADATION + 2, card.matrixTensionDegradation, "0110");
    writer.textUnlessDefault(FIELD_FIBER_TENSION_DEGRADATION + 3, card.matrixCompressionDegradation, "0110");
    writer.textUnlessDefault(FIELD_FIBER_TENSION_DEGRADATION + 4, card.matrixShearDegradation, "0001");
}

std::uint32_t UserCompositeFailureMatTable::append(const UserCompositeFailureMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserCompositeFailureMatTable::store(std::uint32_t row, const UserCompositeFailureMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserCompositeFailureMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserCompositeFailureMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
