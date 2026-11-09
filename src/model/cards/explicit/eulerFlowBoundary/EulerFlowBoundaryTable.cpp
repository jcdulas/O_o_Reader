#include "oor/model/cards/explicit/eulerFlowBoundary/EulerFlowBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BOX_FIELD = 9;
constexpr int FIRST_PROPERTY_FIELD = 17; // TYPE1; each pair takes two fields
constexpr int LAST_FIELD = 26;
constexpr EulerFlowBoundaryPropertyRules RULES{true};

} // namespace

std::optional<EulerFlowBoundaryCard> EulerFlowBoundaryFormat::read(FieldReader& reader)
{
    EulerFlowBoundaryCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.contactBodyId = reader.integer(2, "contactBodyId", 0);
    card.meshId = reader.integer(3, "meshId", 0);
    card.meshFace = readOptionalKeyword<BoundaryFace>(reader, 4, "meshFace", BOUNDARY_FACE_WORDS);
    card.box = readBoundaryBox(reader, BOX_FIELD);
    for (std::size_t index = 0; index < EulerFlowBoundaryCard::MAX_PROPERTIES; ++index)
    {
        if (const std::optional<EulerFlowBoundaryProperty> property =
                readFlowProperty(reader, FIRST_PROPERTY_FIELD + 2 * static_cast<int>(index), index, RULES))
            card.properties.push_back(*property);
    }
    requireBlankFields(reader, {5, 6, 7, 8, 15, 16}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerFlowBoundaryFormat::check(const EulerFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkOptionalId(card.contactBodyId, "contactBodyId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    if (const auto problem = checkBoundaryBox(card.box))
        return problem;
    if (card.properties.size() > EulerFlowBoundaryCard::MAX_PROPERTIES)
        return CardProblem{"card.bad-value",
                           "the card has at most 5 pairs of " + CardProblem::fieldNamed("propertyKind1") + " and " +
                               CardProblem::fieldNamed("propertyValue1"),
                           "propertyKind5"};
    for (std::size_t index = 0; index < card.properties.size(); ++index)
    {
        if (const auto problem = checkFlowProperty(card.properties[index], index, RULES))
            return problem;
    }
    return std::nullopt;
}

void EulerFlowBoundaryFormat::write(const EulerFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.integerUnlessDefault(2, card.contactBodyId, 0);
    writer.integerUnlessDefault(3, card.meshId, 0);
    writeOptionalKeyword(writer, 4, BOUNDARY_FACE_WORDS, card.meshFace);
    writeBoundaryBox(writer, BOX_FIELD, card.box);
    for (std::size_t index = 0; index < card.properties.size(); ++index)
        writeFlowProperty(writer, FIRST_PROPERTY_FIELD + 2 * static_cast<int>(index), card.properties[index]);
}

std::uint32_t EulerFlowBoundaryTable::append(const EulerFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void EulerFlowBoundaryTable::store(std::uint32_t row, const EulerFlowBoundaryCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void EulerFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void EulerFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerFlowBoundaryCard& card = m_cards.edit(row);
    visitId(visitor, "contactBodyId", ReferenceTarget::ContactSurface, card.contactBodyId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
    for (std::size_t index = 0; index < card.properties.size(); ++index)
        visitFlowProperty(visitor, card.properties[index], index);
}

} // namespace oor::model
