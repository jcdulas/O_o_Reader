#include "oor/model/cards/nonlinear_contact/nonlinearBushLoad3D/NonlinearBushLoad3DTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BLANK_FIELD = 4; // the cell between GB and LID-1 in the grid of the format
constexpr int LOAD_ID1_FIELD = 5;
constexpr int LOAD_ID2_FIELD = 6;
constexpr int LAST_FIELD = LOAD_ID2_FIELD;

} // namespace

std::optional<NonlinearBushLoad3DCard> NonlinearBushLoad3DFormat::read(FieldReader& reader)
{
    NonlinearBushLoad3DCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.loadId1 = reader.requiredInteger(LOAD_ID1_FIELD, "loadId1");
    card.loadId2 = reader.requiredInteger(LOAD_ID2_FIELD, "loadId2");
    requireBlankFields(reader, {BLANK_FIELD}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearBushLoad3DFormat::check(const NonlinearBushLoad3DCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeB, "nodeB"))
        return problem;
    if (card.nodeA == card.nodeB)
        return CardProblem{"card.duplicate-node",
                           CardProblem::fieldNamed("nodeA") + " and " + std::string(CardProblem::FIELD) +
                               " must be two different grid points",
                           "nodeB"};
    if (const auto problem = checkRequiredId(card.loadId1, "loadId1"))
        return problem;
    return checkRequiredId(card.loadId2, "loadId2");
}

void NonlinearBushLoad3DFormat::write(const NonlinearBushLoad3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writer.integer(LOAD_ID1_FIELD, card.loadId1);
    writer.integer(LOAD_ID2_FIELD, card.loadId2);
}

std::uint32_t NonlinearBushLoad3DTable::append(const NonlinearBushLoad3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_loadId1.push(card.loadId1);
    m_loadId2.push(card.loadId2);
    return row;
}

NonlinearBushLoad3DCard NonlinearBushLoad3DTable::card(std::uint32_t row) const
{
    return NonlinearBushLoad3DCard{m_setId.get(row), m_nodeA.get(row), m_nodeB.get(row), m_loadId1.get(row),
                                   m_loadId2.get(row)};
}

void NonlinearBushLoad3DTable::store(std::uint32_t row, const NonlinearBushLoad3DCard& card)
{
    m_setId.set(row, card.setId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_loadId1.set(row, card.loadId1);
    m_loadId2.set(row, card.loadId2);
}

void NonlinearBushLoad3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_loadId1.keepRows(rows);
    m_loadId2.keepRows(rows);
}

void NonlinearBushLoad3DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
    m_loadId1.set(row, visitor.visit("loadId1", ReferenceTarget::BushingForceSet, m_loadId1.get(row)));
    m_loadId2.set(row, visitor.visit("loadId2", ReferenceTarget::BushingForceSet, m_loadId2.get(row)));
}

} // namespace oor::model
