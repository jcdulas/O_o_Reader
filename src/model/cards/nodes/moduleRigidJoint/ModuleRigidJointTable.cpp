#include "oor/model/cards/nodes/moduleRigidJoint/ModuleRigidJointTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6;

} // namespace

std::optional<ModuleRigidJointCard> ModuleRigidJointFormat::read(FieldReader& reader)
{
    ModuleRigidJointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.moduleIdA = reader.integer(2, "moduleIdA", 0);
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.moduleIdB = reader.integer(4, "moduleIdB", 0);
    card.nodeB = reader.requiredInteger(5, "nodeB");
    // A blank CB is 123456: GB moves with GA as a rigid body.
    card.componentsB = base::sortedDigits(reader.integer(6, "componentsB", components::ALL));
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRigidJointFormat::check(const ModuleRigidJointCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdB, "moduleIdB"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeB, "nodeB"))
        return problem;
    // The two points of one module are two points; two modules may number their points alike.
    if (card.moduleIdA == card.moduleIdB && card.nodeA == card.nodeB)
        return CardProblem{"card.duplicate-node",
                           "{field} must not be the same grid point as " + CardProblem::fieldNamed("nodeA"), "nodeB"};
    if (card.componentsB == 0 || !base::isComponentSet(card.componentsB))
        return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                           "componentsB"};
    return std::nullopt;
}

void ModuleRigidJointFormat::write(const ModuleRigidJointCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.moduleIdA, 0);
    writer.integer(3, card.nodeA);
    writer.integerUnlessDefault(4, card.moduleIdB, 0);
    writer.integer(5, card.nodeB);
    writer.integerUnlessDefault(6, card.componentsB, components::ALL);
}

std::uint32_t ModuleRigidJointTable::append(const ModuleRigidJointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRigidJointTable::store(std::uint32_t row, const ModuleRigidJointCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRigidJointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRigidJointTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRigidJointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRigidJointCard& card = m_cards.edit(row);
    // Each grid point is looked up in its module (ModuleConnectionSupport.hpp).
    visitIdInModule(visitor, "nodeA", ReferenceTarget::Node, card.moduleIdA, card.nodeA);
    visitIdInModule(visitor, "nodeB", ReferenceTarget::Node, card.moduleIdB, card.nodeB);
}

} // namespace oor::model
