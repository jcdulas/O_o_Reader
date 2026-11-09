#include "oor/model/cards/explicit/eulerUserFlowBoundary/EulerUserFlowBoundaryTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BOX_FIELD = 9;
constexpr int LAST_FIELD = 14;

} // namespace

std::optional<EulerUserFlowBoundaryCard> EulerUserFlowBoundaryFormat::read(FieldReader& reader)
{
    EulerUserFlowBoundaryCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), EulerUserFlowBoundaryCard::DEFAULT_ROUTINE_NAME);
    card.contactBodyId = reader.integer(4, "contactBodyId", 0);
    card.meshId = reader.integer(5, "meshId", 0);
    card.meshFace = readOptionalKeyword<BoundaryFace>(reader, 6, "meshFace", BOUNDARY_FACE_WORDS);
    card.box = readBoundaryBox(reader, BOX_FIELD);
    requireBlankFields(reader, {7, 8}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerUserFlowBoundaryFormat::check(const EulerUserFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem =
            checkRoutineName(card.userRoutineName, EulerUserFlowBoundaryCard::DEFAULT_ROUTINE_NAME, "userRoutineName"))
        return problem;
    if (const auto problem = checkOptionalId(card.contactBodyId, "contactBodyId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    return checkBoundaryBox(card.box);
}

void EulerUserFlowBoundaryFormat::write(const EulerUserFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.text(2, card.groupName);
    writer.textUnlessDefault(3, card.userRoutineName, EulerUserFlowBoundaryCard::DEFAULT_ROUTINE_NAME);
    writer.integerUnlessDefault(4, card.contactBodyId, 0);
    writer.integerUnlessDefault(5, card.meshId, 0);
    writeOptionalKeyword(writer, 6, BOUNDARY_FACE_WORDS, card.meshFace);
    writeBoundaryBox(writer, BOX_FIELD, card.box);
}

std::uint32_t EulerUserFlowBoundaryTable::append(const EulerUserFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void EulerUserFlowBoundaryTable::store(std::uint32_t row, const EulerUserFlowBoundaryCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void EulerUserFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerUserFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void EulerUserFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerUserFlowBoundaryCard& card = m_cards.edit(row);
    visitId(visitor, "contactBodyId", ReferenceTarget::ContactSurface, card.contactBodyId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
}

} // namespace oor::model
