#include "oor/model/cards/aero/aeroFlutter/AeroFlutterTable.hpp"

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 8;
constexpr int EIGENVALUE_COUNT_OR_MAX_FREQUENCY_FIELD = 7;
constexpr double DEFAULT_EPSILON = 1.0E-3;

} // namespace

AeroFlutterTable::AeroFlutterTable() : m_maxFrequency(BLANK_REAL), m_epsilon(DEFAULT_EPSILON) {}

std::optional<AeroFlutterCard> AeroFlutterFormat::read(FieldReader& reader)
{
    AeroFlutterCard card;
    card.id = reader.requiredInteger(1, "id");
    card.method = readRequiredKeyword<AeroFlutterMethod>(reader, 2, "method", FLUTTER_METHOD_WORDS);
    card.densityFactorsId = reader.requiredInteger(3, "densityFactorsId");
    card.machFactorsId = reader.requiredInteger(4, "machFactorsId");
    card.reducedFreqFactorsId = reader.requiredInteger(5, "reducedFreqFactorsId");
    card.interpolationMethod =
        readKeyword(reader, 6, "interpolationMethod", FLUTTER_INTERPOLATION_WORDS, AeroFlutterInterpolation::Linear);
    // Field 7 is an integer or a real by the method; only the one the method uses is read, so that the other kind of
    // number is an error of the field and not a value that is dropped.
    if (isSweepMethod(card.method))
    {
        card.maxFrequency = reader.optionalReal(EIGENVALUE_COUNT_OR_MAX_FREQUENCY_FIELD, "maxFrequency");
    }
    else if (const std::optional<std::int32_t> eigenvalueCount =
                 reader.optionalInteger(EIGENVALUE_COUNT_OR_MAX_FREQUENCY_FIELD, "eigenvalueCount"))
    {
        // A written zero is refused: it would read back as a blank field.
        if (*eigenvalueCount <= 0)
            reader.report(
                CardProblem{"card.bad-value", "{field} must be a positive integer, or blank", "eigenvalueCount"});
        card.eigenvalueCount = *eigenvalueCount;
    }
    card.epsilon = reader.real(8, "epsilon", DEFAULT_EPSILON);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroFlutterFormat::check(const AeroFlutterCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.densityFactorsId, "densityFactorsId"))
        return problem;
    if (const auto problem = checkRequiredId(card.machFactorsId, "machFactorsId"))
        return problem;
    if (const auto problem = checkRequiredId(card.reducedFreqFactorsId, "reducedFreqFactorsId"))
        return problem;
    // the termwise cubic interpolation is for the PK methods.
    const bool kMethod = card.method == AeroFlutterMethod::K || card.method == AeroFlutterMethod::Ke;
    if (kMethod && card.interpolationMethod == AeroFlutterInterpolation::TermwiseCubic)
        return CardProblem{"card.bad-value", "{field} TCUB is for the PK methods, not for K and KE",
                           "interpolationMethod"};
    if (card.eigenvalueCount < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer, or blank", "eigenvalueCount"};
    if (const auto problem = checkOptionalReal(card.maxFrequency, "maxFrequency", RealRule::Positive))
        return problem;
    // Field 7 holds one number: the member of the other kind of method would be lost in the writing.
    if (isSweepMethod(card.method) && card.eigenvalueCount != 0)
        return CardProblem{"card.bad-value",
                           "{field} is for the K, KE, PK and PKNL methods; PKS and PKNLS take {field:maxFrequency}",
                           "eigenvalueCount"};
    if (!isSweepMethod(card.method) && card.maxFrequency)
        return CardProblem{"card.bad-value",
                           "{field} is for the PKS and PKNLS methods; the others take {field:eigenvalueCount}",
                           "maxFrequency"};
    return checkReal(card.epsilon, "epsilon", RealRule::Positive);
}

void AeroFlutterFormat::write(const AeroFlutterCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, FLUTTER_METHOD_WORDS, card.method);
    writer.integer(3, card.densityFactorsId);
    writer.integer(4, card.machFactorsId);
    writer.integer(5, card.reducedFreqFactorsId);
    writeKeywordUnlessDefault(writer, 6, FLUTTER_INTERPOLATION_WORDS, card.interpolationMethod,
                              AeroFlutterInterpolation::Linear);
    if (isSweepMethod(card.method))
    {
        if (card.maxFrequency)
            writer.real(EIGENVALUE_COUNT_OR_MAX_FREQUENCY_FIELD, *card.maxFrequency);
    }
    else if (card.eigenvalueCount != 0)
    {
        writer.integer(EIGENVALUE_COUNT_OR_MAX_FREQUENCY_FIELD, card.eigenvalueCount);
    }
    writer.realUnlessDefault(8, card.epsilon, DEFAULT_EPSILON);
}

std::uint32_t AeroFlutterTable::append(const AeroFlutterCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_densityFactorsId.push(card.densityFactorsId);
    m_machFactorsId.push(card.machFactorsId);
    m_reducedFreqFactorsId.push(card.reducedFreqFactorsId);
    m_interpolationMethod.push(static_cast<std::uint8_t>(card.interpolationMethod));
    m_eigenvalueCount.push(card.eigenvalueCount);
    m_maxFrequency.push(realToColumn(card.maxFrequency));
    m_epsilon.push(card.epsilon);
    return row;
}

AeroFlutterCard AeroFlutterTable::card(std::uint32_t row) const
{
    AeroFlutterCard card;
    card.id = m_id.get(row);
    card.method = static_cast<AeroFlutterMethod>(m_method.get(row));
    card.densityFactorsId = m_densityFactorsId.get(row);
    card.machFactorsId = m_machFactorsId.get(row);
    card.reducedFreqFactorsId = m_reducedFreqFactorsId.get(row);
    card.interpolationMethod = static_cast<AeroFlutterInterpolation>(m_interpolationMethod.get(row));
    card.eigenvalueCount = m_eigenvalueCount.get(row);
    card.maxFrequency = realFromColumn(m_maxFrequency.get(row));
    card.epsilon = m_epsilon.get(row);
    return card;
}

void AeroFlutterTable::store(std::uint32_t row, const AeroFlutterCard& card)
{
    m_id.set(row, card.id);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_densityFactorsId.set(row, card.densityFactorsId);
    m_machFactorsId.set(row, card.machFactorsId);
    m_reducedFreqFactorsId.set(row, card.reducedFreqFactorsId);
    m_interpolationMethod.set(row, static_cast<std::uint8_t>(card.interpolationMethod));
    m_eigenvalueCount.set(row, card.eigenvalueCount);
    m_maxFrequency.set(row, realToColumn(card.maxFrequency));
    m_epsilon.set(row, card.epsilon);
}

void AeroFlutterTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_method.keepRows(rows);
    m_densityFactorsId.keepRows(rows);
    m_machFactorsId.keepRows(rows);
    m_reducedFreqFactorsId.keepRows(rows);
    m_interpolationMethod.keepRows(rows);
    m_eigenvalueCount.keepRows(rows);
    m_maxFrequency.keepRows(rows);
    m_epsilon.keepRows(rows);
}

void AeroFlutterTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId densityFactorsId = m_densityFactorsId.get(row); densityFactorsId > 0)
        m_densityFactorsId.set(row,
                               visitor.visit("densityFactorsId", ReferenceTarget::AeroFlutterFactor, densityFactorsId));
    if (const base::EntityId machFactorsId = m_machFactorsId.get(row); machFactorsId > 0)
        m_machFactorsId.set(row, visitor.visit("machFactorsId", ReferenceTarget::AeroFlutterFactor, machFactorsId));
    if (const base::EntityId reducedFreqFactorsId = m_reducedFreqFactorsId.get(row); reducedFreqFactorsId > 0)
        m_reducedFreqFactorsId.set(
            row, visitor.visit("reducedFreqFactorsId", ReferenceTarget::AeroFlutterFactor, reducedFreqFactorsId));
}

} // namespace oor::model
