#include "oor/model/cards/aero/aeroForceVector/AeroForceVectorTable.hpp"

#include "oor/model/cards/aero/ForceMach.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int PER_UNIT_PRESSURE_FIELD = 8;
constexpr int LAST_FIELD = PER_UNIT_PRESSURE_FIELD;

// The PERQ field: the word PERQ, or blank.
bool readPerq(FieldReader& reader)
{
    const std::string_view text = reader.text(PER_UNIT_PRESSURE_FIELD);
    if (text.empty())
        return false;
    if ((text == AERO_FORCE_VECTOR_PER_PRESSURE_WORD))
        return true;
    reader.report(CardProblem{"card.bad-value", "{field} must be PERQ or blank, not \"" + std::string(text) + "\"",
                              "perUnitPressure"});
    return false;
}

} // namespace

std::optional<AeroForceVectorCard> AeroForceVectorFormat::read(FieldReader& reader)
{
    AeroForceVectorCard card;
    card.machNumber = reader.requiredReal(1, "machNumber");
    card.symmetricXz = readRequiredKeyword<ForceSymmetry>(reader, 2, "symmetricXz", FORCE_SYMMETRY_WORDS);
    card.symmetricXy = readRequiredKeyword<ForceSymmetry>(reader, 3, "symmetricXy", FORCE_SYMMETRY_WORDS);
    card.controlVectorId = reader.requiredInteger(4, "controlVectorId");
    card.mesh = readRequiredKeyword<AeroForceVectorMesh>(reader, 5, "mesh", AERO_FORCE_VECTOR_MESH_WORDS);
    card.loadSetId = reader.integer(6, "loadSetId", 0);
    if (!reader.isBlank(7))
        card.matrixName = std::string(reader.text(7));
    card.perUnitPressure = readPerq(reader);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.report(CardProblem{"card.bad-value",
                                  "an " + std::string(reader.cardName()) +
                                      " has eight fields: {field:machNumber}, {field:symmetricXz}, "
                                      "{field:symmetricXy}, {field:controlVectorId}, {field:mesh}, {field:loadSetId}, "
                                      "{field:matrixName} and {field:perUnitPressure}",
                                  ""});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroForceVectorFormat::check(const AeroForceVectorCard& card)
{
    if (const auto problem = checkForceMach(card.machNumber))
        return problem;
    if (const auto problem = checkRequiredId(card.controlVectorId, "controlVectorId"))
        return problem;
    if (const auto problem = checkOptionalId(card.loadSetId, "loadSetId"))
        return problem;
    if (card.matrixName && !isMatrixName(*card.matrixName))
        return CardProblem{"card.bad-value", "{field} must be a nonempty matrix name without control characters",
                           "matrixName"};
    // The MESH selects the fields that are used; the one that belongs to the other mesh must be blank (PERQ is only
    // ignored for AERO, so it is allowed with both).
    switch (card.mesh)
    {
    case AeroForceVectorMesh::Aero:
        if (!card.matrixName)
            return CardProblem{"card.missing-field", "{field} is required when {field:mesh} is AERO", "matrixName"};
        if (card.loadSetId != 0)
            return CardProblem{"card.bad-value",
                               "{field} is only used when {field:mesh} is STRUCT: leave it blank for AERO",
                               "loadSetId"};
        break;
    case AeroForceVectorMesh::Struct:
        if (card.loadSetId == 0)
            return CardProblem{"card.missing-field", "{field} is required when {field:mesh} is STRUCT", "loadSetId"};
        if (card.matrixName)
            return CardProblem{"card.bad-value",
                               "{field} is only used when {field:mesh} is AERO: leave it blank for STRUCT",
                               "matrixName"};
        break;
    }
    return std::nullopt;
}

void AeroForceVectorFormat::write(const AeroForceVectorCard& card, FieldWriter& writer)
{
    writer.real(1, card.machNumber);
    writeKeyword(writer, 2, FORCE_SYMMETRY_WORDS, card.symmetricXz);
    writeKeyword(writer, 3, FORCE_SYMMETRY_WORDS, card.symmetricXy);
    writer.integer(4, card.controlVectorId);
    writeKeyword(writer, 5, AERO_FORCE_VECTOR_MESH_WORDS, card.mesh);
    writer.integerUnlessDefault(6, card.loadSetId, 0);
    if (card.matrixName)
        writer.text(7, *card.matrixName);
    if (card.perUnitPressure)
        writer.text(PER_UNIT_PRESSURE_FIELD, AERO_FORCE_VECTOR_PER_PRESSURE_WORD);
}

std::uint32_t AeroForceVectorTable::append(const AeroForceVectorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_controlVectorId.push(card.controlVectorId);
    m_cards.push(card);
    return row;
}

void AeroForceVectorTable::store(std::uint32_t row, const AeroForceVectorCard& card)
{
    m_controlVectorId.set(row, card.controlVectorId);
    m_cards.set(row, card);
}

void AeroForceVectorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_controlVectorId.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroForceVectorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The DMIK is a name, not a reference. The column of the UXIDs follows the card when a renumbering changes the
    // UXID.
    AeroForceVectorCard& card = m_cards.edit(row);
    visitId(visitor, "controlVectorId", ReferenceTarget::ControlVector, card.controlVectorId);
    visitId(visitor, "loadSetId", ReferenceTarget::LoadCombinationSet, card.loadSetId);
    m_controlVectorId.set(row, card.controlVectorId);
}

} // namespace oor::model
