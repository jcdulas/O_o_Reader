#include "oor/model/cards/explicit/eulerCyclicFlowBoundary/EulerCyclicFlowBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BOX_FIELD = 9;
constexpr int METHOD_FIELD = 17;
constexpr int TABLE_ID_FIELD = 18;

} // namespace

std::optional<EulerCyclicFlowBoundaryCard> EulerCyclicFlowBoundaryFormat::read(FieldReader& reader)
{
    EulerCyclicFlowBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.contactBodyId = reader.integer(2, "contactBodyId", 0);
    card.type = readRequiredKeyword<FlowBoundaryType>(reader, 3, "flowType", FLOW_BOUNDARY_TYPE_WORDS);
    card.meshId = reader.integer(4, "meshId", 0);
    card.meshFace = readOptionalKeyword<BoundaryFace>(reader, 5, "meshFace", BOUNDARY_FACE_WORDS);
    card.pairedBoundaryId = reader.requiredInteger(6, "pairedBoundaryId");
    card.box = readBoundaryBox(reader, BOX_FIELD);
    card.method = reader.integer(METHOD_FIELD, "method", EulerCyclicFlowBoundaryCard::DEFAULT_METHOD);
    card.tableId = reader.integer(TABLE_ID_FIELD, "tableId", 0);
    requireBlankFields(reader, {7, 8, 15, 16}, TABLE_ID_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerCyclicFlowBoundaryFormat::check(const EulerCyclicFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.contactBodyId, "contactBodyId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    if (const auto problem = checkRequiredId(card.pairedBoundaryId, "pairedBoundaryId"))
        return problem;
    if (const auto problem = checkBoundaryBox(card.box))
        return problem;
    if (card.method < 1 || card.method > EulerCyclicFlowBoundaryCard::LAST_METHOD)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3", "method"};
    return checkOptionalId(card.tableId, "tableId");
}

void EulerCyclicFlowBoundaryFormat::write(const EulerCyclicFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.contactBodyId, 0);
    writeKeyword(writer, 3, FLOW_BOUNDARY_TYPE_WORDS, card.type);
    writer.integerUnlessDefault(4, card.meshId, 0);
    writeOptionalKeyword(writer, 5, BOUNDARY_FACE_WORDS, card.meshFace);
    writer.integer(6, card.pairedBoundaryId);
    writeBoundaryBox(writer, BOX_FIELD, card.box);
    writer.integerUnlessDefault(METHOD_FIELD, card.method, EulerCyclicFlowBoundaryCard::DEFAULT_METHOD);
    writer.integerUnlessDefault(TABLE_ID_FIELD, card.tableId, 0);
}

std::uint32_t EulerCyclicFlowBoundaryTable::append(const EulerCyclicFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerCyclicFlowBoundaryTable::store(std::uint32_t row, const EulerCyclicFlowBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerCyclicFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerCyclicFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerCyclicFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerCyclicFlowBoundaryCard& card = m_cards.edit(row);
    visitId(visitor, "contactBodyId", ReferenceTarget::ContactSurface, card.contactBodyId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
    visitId(visitor, "tableId", ReferenceTarget::DynamicTable,
            card.tableId); // fid2 is a plain ID: no category names a FLOWC
}

} // namespace oor::model
