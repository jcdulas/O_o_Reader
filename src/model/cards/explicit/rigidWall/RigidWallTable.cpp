#include "oor/model/cards/explicit/rigidWall/RigidWallTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<RigidWallCard> RigidWallFormat::read(FieldReader& reader)
{
    RigidWallCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointX = reader.real(2, "pointX", 0.0);
    card.pointY = reader.real(3, "pointY", 0.0);
    card.pointZ = reader.real(4, "pointZ", 0.0);
    card.normalX = reader.real(5, "normalX", 0.0);
    card.normalY = reader.real(6, "normalY", 0.0);
    card.normalZ = reader.real(7, "normalZ", 0.0);
    card.nodeSetId = reader.requiredInteger(8, "nodeSetId");
    card.method = readKeyword(reader, 9, "method", RIGID_WALL_METHOD_WORDS, RigidWallMethod::Penalty);
    card.staticFriction = reader.real(10, "staticFriction", 0.0);
    card.kineticFriction = reader.real(11, "kineticFriction", 0.0);
    card.frictionDecay = reader.real(12, "frictionDecay", 0.0);
    if (reader.lastUsedField() > 12)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 12 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidWallFormat::check(const RigidWallCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.pointX, "pointX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.pointY, "pointY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.pointZ, "pointZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.normalX, "normalX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.normalY, "normalY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.normalZ, "normalZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkReal(card.staticFriction, "staticFriction", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.kineticFriction, "kineticFriction", RealRule::NonNegative))
        return problem;
    return checkReal(card.frictionDecay, "frictionDecay", RealRule::NonNegative);
}

void RigidWallFormat::write(const RigidWallCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.pointX, 0.0);
    writer.realUnlessDefault(3, card.pointY, 0.0);
    writer.realUnlessDefault(4, card.pointZ, 0.0);
    writer.realUnlessDefault(5, card.normalX, 0.0);
    writer.realUnlessDefault(6, card.normalY, 0.0);
    writer.realUnlessDefault(7, card.normalZ, 0.0);
    writer.integer(8, card.nodeSetId);
    writeKeywordUnlessDefault(writer, 9, RIGID_WALL_METHOD_WORDS, card.method, RigidWallMethod::Penalty);
    writer.realUnlessDefault(10, card.staticFriction, 0.0);
    writer.realUnlessDefault(11, card.kineticFriction, 0.0);
    writer.realUnlessDefault(12, card.frictionDecay, 0.0);
}

std::uint32_t RigidWallTable::append(const RigidWallCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RigidWallTable::store(std::uint32_t row, const RigidWallCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RigidWallTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidWallTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RigidWallTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "nodeSetId", ReferenceTarget::ContactBody, m_cards.edit(row).nodeSetId);
}

} // namespace oor::model
