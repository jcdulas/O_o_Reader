#include "oor/model/cards/aero/aeroPressureVector/AeroPressureVectorTable.hpp"

#include "oor/model/cards/aero/ForceMach.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6;

} // namespace

std::optional<AeroPressureVectorCard> AeroPressureVectorFormat::read(FieldReader& reader)
{
    AeroPressureVectorCard card;
    card.machNumber = reader.requiredReal(1, "machNumber");
    card.symmetricXz = readRequiredKeyword<ForceSymmetry>(reader, 2, "symmetricXz", FORCE_SYMMETRY_WORDS);
    card.symmetricXy = readRequiredKeyword<ForceSymmetry>(reader, 3, "symmetricXy", FORCE_SYMMETRY_WORDS);
    card.controlVectorId = reader.requiredInteger(4, "controlVectorId");
    if (reader.isBlank(5))
        reader.report(CardProblem{"card.missing-field", "{field} is required", "matrixName"});
    card.matrixName = std::string(reader.text(5));
    if (!reader.isBlank(6))
        card.interpolationMatrixName = std::string(reader.text(6));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has six fields: {field:machNumber}, {field:symmetricXz}, {field:symmetricXy}, "
                                      "{field:controlVectorId}, {field:matrixName} and {field:interpolationMatrixName}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroPressureVectorFormat::check(const AeroPressureVectorCard& card)
{
    if (const auto problem = checkForceMach(card.machNumber))
        return problem;
    if (const auto problem = checkRequiredId(card.controlVectorId, "controlVectorId"))
        return problem;
    if (!isMatrixName(card.matrixName))
        return CardProblem{"card.bad-value", "{field} must be a nonempty matrix name without control characters",
                           "matrixName"};
    if (card.interpolationMatrixName && !isMatrixName(*card.interpolationMatrixName))
        return CardProblem{"card.bad-value", "{field} must be a nonempty matrix name without control characters",
                           "interpolationMatrixName"};
    return std::nullopt;
}

void AeroPressureVectorFormat::write(const AeroPressureVectorCard& card, FieldWriter& writer)
{
    writer.real(1, card.machNumber);
    writeKeyword(writer, 2, FORCE_SYMMETRY_WORDS, card.symmetricXz);
    writeKeyword(writer, 3, FORCE_SYMMETRY_WORDS, card.symmetricXy);
    writer.integer(4, card.controlVectorId);
    writer.text(5, card.matrixName);
    if (card.interpolationMatrixName)
        writer.text(6, *card.interpolationMatrixName);
}

std::uint32_t AeroPressureVectorTable::append(const AeroPressureVectorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_controlVectorId.push(card.controlVectorId);
    m_cards.push(card);
    return row;
}

void AeroPressureVectorTable::store(std::uint32_t row, const AeroPressureVectorCard& card)
{
    m_controlVectorId.set(row, card.controlVectorId);
    m_cards.set(row, card);
}

void AeroPressureVectorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_controlVectorId.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroPressureVectorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The matrix names are not references. The column of the UXIDs follows the card when a renumbering changes the
    // UXID.
    AeroPressureVectorCard& card = m_cards.edit(row);
    visitId(visitor, "controlVectorId", ReferenceTarget::ControlVector, card.controlVectorId);
    m_controlVectorId.set(row, card.controlVectorId);
}

} // namespace oor::model
