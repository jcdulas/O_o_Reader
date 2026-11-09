#include "oor/model/cards/solution_control/cosimulationService/CosimulationServiceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/ExternalName.hpp"

namespace oor::model
{

namespace
{

constexpr int SERVICE_NAME_FIELD = 2;
constexpr int FOLLOWER_FORCES_FIELD = 4; // field 3 is blank
constexpr int INPUT_AREA_FIELD = 9;      // the first field of the continuation line
constexpr int INPUT_ID_FIELD = 10;
constexpr int OUTPUT_AREA_FIELD = 11;
constexpr int OUTPUT_ID_FIELD = 12; // the last field of the card

} // namespace

std::optional<CosimulationServiceCard> CosimulationServiceFormat::read(FieldReader& reader)
{
    CosimulationServiceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.serviceName = std::string(reader.text(SERVICE_NAME_FIELD));
    card.followerForces = readKeyword(reader, FOLLOWER_FORCES_FIELD, "followerForces", COSIMULATION_FORCE_WORDS,
                                      CosimulationServiceCard::DEFAULT_FORCE_MODE);
    card.inputAreaId = reader.requiredInteger(INPUT_AREA_FIELD, "inputAreaId");
    card.inputId = reader.requiredInteger(INPUT_ID_FIELD, "inputId");
    card.outputAreaId = reader.requiredInteger(OUTPUT_AREA_FIELD, "outputAreaId");
    card.outputId = reader.requiredInteger(OUTPUT_ID_FIELD, "outputId");
    requireBlankFields(reader, {3, 5, 6, 7, 8}, OUTPUT_ID_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CosimulationServiceFormat::check(const CosimulationServiceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkExternalName(card.serviceName, "serviceName"))
        return problem;
    if (const auto problem = checkRequiredId(card.inputAreaId, "inputAreaId"))
        return problem;
    if (const auto problem = checkRequiredId(card.inputId, "inputId"))
        return problem;
    if (const auto problem = checkRequiredId(card.outputAreaId, "outputAreaId"))
        return problem;
    return checkRequiredId(card.outputId, "outputId");
}

void CosimulationServiceFormat::write(const CosimulationServiceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(SERVICE_NAME_FIELD, card.serviceName);
    writeKeywordUnlessDefault(writer, FOLLOWER_FORCES_FIELD, COSIMULATION_FORCE_WORDS, card.followerForces,
                              CosimulationServiceCard::DEFAULT_FORCE_MODE);
    writer.integer(INPUT_AREA_FIELD, card.inputAreaId);
    writer.integer(INPUT_ID_FIELD, card.inputId);
    writer.integer(OUTPUT_AREA_FIELD, card.outputAreaId);
    writer.integer(OUTPUT_ID_FIELD, card.outputId);
}

std::uint32_t CosimulationServiceTable::append(const CosimulationServiceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CosimulationServiceTable::store(std::uint32_t row, const CosimulationServiceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CosimulationServiceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CosimulationServiceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CosimulationServiceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The CSRVID is selected by a COSMSEL and names nothing; the four IDs after it name the areas and the quantities.
    CosimulationServiceCard& card = m_cards.edit(row);
    visitId(visitor, "inputAreaId", ReferenceTarget::CosimulationGroup, card.inputAreaId);
    visitId(visitor, "inputId", ReferenceTarget::CosimulationInput, card.inputId);
    visitId(visitor, "outputAreaId", ReferenceTarget::CosimulationGroup, card.outputAreaId);
    visitId(visitor, "outputId", ReferenceTarget::CosimulationOutput, card.outputId);
}

} // namespace oor::model
