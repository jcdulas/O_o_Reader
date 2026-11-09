#include "oor/model/cards/nodes/moduleBushing/ModuleBushingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include <variant>

namespace oor::model
{

std::optional<ModuleBushingCard> ModuleBushingFormat::read(FieldReader& reader)
{
    ModuleBushingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.integer(4, "nodeB", 0); // blank: grounded
    // Field 5 is GO (an integer) or X1 (a real); X2 and X3 only count when X1 is there, as for the CBUSH.
    if (const auto orientation = reader.integerOrReal(5, "orientationNode/position1"))
    {
        if (std::holds_alternative<std::int32_t>(*orientation))
        {
            card.orientationNode = std::get<std::int32_t>(*orientation);
            if (card.orientationNode <= 0)
                reader.error("card.bad-value", std::string("orientationNode") + " must be a positive grid point ID",
                             "orientationNode");
        }
        else
        {
            card.orientationVector[0] = std::get<double>(*orientation);
            card.orientationVector[1] = reader.real(6, "position2", 0.0);
            card.orientationVector[2] = reader.real(7, "position3", 0.0);
        }
    }
    card.frameId = reader.integer(8, "frameId", ModuleBushingCard::NO_FRAME_ID);
    card.offsetFraction = reader.real(9, "offsetFraction", ModuleBushingCard::DEFAULT_OFFSET_FRACTION);
    card.offsetFrameId = reader.integer(10, "offsetFrameId", ModuleBushingCard::NO_OFFSET_FRAME_ID);
    card.offsets[0] = reader.real(11, "offset1", 0.0);
    card.offsets[1] = reader.real(12, "offset2", 0.0);
    card.offsets[2] = reader.real(13, "offset3", 0.0);
    card.moduleIdA = reader.integer(14, "moduleIdA", 0);
    card.moduleIdB = reader.integer(15, "moduleIdB", 0);
    card.orientationModuleId = reader.integer(16, "orientationModuleId", 0);
    if (reader.lastUsedField() > 16)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 16 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleBushingFormat::check(const ModuleBushingCard& card)
{
    if (card.id <= 0 || card.id > ModuleBushingCard::MAX_ID)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer below 100 000 000", "id"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeA, "nodeA"))
        return problem;
    if (card.nodeB < 0)
        return CardProblem{"card.bad-id", "{field} must be 0 (blank) or a grid point ID", "nodeB"};
    // The two end points may have the same number when they are in different modules.
    if (card.nodeB != 0 && card.nodeB == card.nodeA && card.moduleIdA == card.moduleIdB)
        return CardProblem{"card.duplicate-node",
                           CardProblem::fieldNamed("nodeA") + " and {field} must be different points", "nodeB"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode != 0 && card.hasVector())
        return CardProblem{"card.exclusive-fields",
                           "the orientation is a grid point {field} or a vector " +
                               CardProblem::fieldNamed("position1") + " " + CardProblem::fieldNamed("position2") + " " +
                               CardProblem::fieldNamed("position3") + ", not both",
                           "orientationNode"};
    if (card.frameId < ModuleBushingCard::NO_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be blank or a coordinate system ID", "frameId"};
    if (card.offsetFrameId < ModuleBushingCard::NO_OFFSET_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be -1 or a coordinate system ID", "offsetFrameId"};
    if (const auto problem = checkReal(card.offsetFraction, "offsetFraction", RealRule::Finite))
        return problem;
    for (const double value : card.orientationVector)
    {
        if (const auto problem = checkReal(value, "position1", RealRule::Finite))
            return problem;
    }
    for (const double value : card.offsets)
    {
        if (const auto problem = checkReal(value, "offset1", RealRule::Finite))
            return problem;
    }
    if (const auto problem = checkBoundaryModule(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkBoundaryModule(card.moduleIdB, "moduleIdB"))
        return problem;
    return checkBoundaryModule(card.orientationModuleId, "orientationModuleId");
}

void ModuleBushingFormat::write(const ModuleBushingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integerUnlessDefault(4, card.nodeB, 0);
    if (card.orientationNode != 0)
    {
        writer.integer(5, card.orientationNode);
    }
    else if (card.hasVector())
    {
        // X2 and X3 are only read when X1 is there: the three are written together.
        writer.real(5, card.orientationVector[0]);
        writer.real(6, card.orientationVector[1]);
        writer.real(7, card.orientationVector[2]);
    }
    if (card.frameId != ModuleBushingCard::NO_FRAME_ID)
        writer.integer(8, card.frameId);
    // S is a static default (0.5): a blank is 0.5, and a written 0.0 must stay 0.0.
    writer.realUnlessDefault(9, card.offsetFraction, ModuleBushingCard::DEFAULT_OFFSET_FRACTION);
    writer.integerUnlessDefault(10, card.offsetFrameId, ModuleBushingCard::NO_OFFSET_FRAME_ID);
    writer.realUnlessDefault(11, card.offsets[0], 0.0);
    writer.realUnlessDefault(12, card.offsets[1], 0.0);
    writer.realUnlessDefault(13, card.offsets[2], 0.0);
    writer.integerUnlessDefault(14, card.moduleIdA, 0);
    writer.integerUnlessDefault(15, card.moduleIdB, 0);
    writer.integerUnlessDefault(16, card.orientationModuleId, 0);
}

std::uint32_t ModuleBushingTable::append(const ModuleBushingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleBushingTable::store(std::uint32_t row, const ModuleBushingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleBushingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleBushingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleBushingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleBushingCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    visitBoundaryId(visitor, "nodeA", ReferenceTarget::Node, card.moduleIdA, card.nodeA);
    visitBoundaryId(visitor, "nodeB", ReferenceTarget::Node, card.moduleIdB, card.nodeB);
    // GO, CID and OCID are in module MODC; CID 0 is the basic system and NO_FRAME_ID is blank, neither is a card (the
    // same for OCID), and visitBoundaryId skips an ID that is not positive.
    visitBoundaryId(visitor, "orientationNode", ReferenceTarget::Node, card.orientationModuleId, card.orientationNode);
    visitBoundaryId(visitor, "frameId", ReferenceTarget::Coordinate, card.orientationModuleId, card.frameId);
    visitBoundaryId(visitor, "offsetFrameId", ReferenceTarget::Coordinate, card.orientationModuleId,
                    card.offsetFrameId);
}

} // namespace oor::model
