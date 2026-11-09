#include "oor/model/cards/nodes/moduleRigidRod/ModuleRigidRodTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 9;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

} // namespace

std::optional<ModuleRigidRodCard> ModuleRigidRodFormat::read(FieldReader& reader)
{
    ModuleRigidRodCard card;
    card.id = reader.requiredInteger(1, "id");
    card.moduleIdA = reader.integer(2, "moduleIdA", 0);
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.moduleIdB = reader.integer(4, "moduleIdB", 0);
    card.nodeB = reader.requiredInteger(5, "nodeB");
    card.componentsAtA = reader.integer(6, "componentsAtA", 0);
    card.componentsAtB = reader.integer(7, "componentsAtB", 0);
    card.alphaCoefficient = reader.real(8, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(9, "referenceTemperature", 0.0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleRigidRodFormat::check(const ModuleRigidRodCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
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
    if (card.componentsAtA < 0 || card.componentsAtA > 3)
        return CardProblem{"card.bad-value", "{field} must be 1, 2, 3 or blank", "componentsAtA"};
    if (card.componentsAtB < 0 || card.componentsAtB > 3)
        return CardProblem{"card.bad-value", "{field} must be 1, 2, 3 or blank", "componentsAtB"};
    if (card.componentsAtA != 0 && card.componentsAtB != 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("componentsAtA") +
                               " and {field} cannot both be given: one dependent component only",
                           "componentsAtB"};
    if (const auto problem = checkReal(card.alphaCoefficient, "alphaCoefficient", RealRule::Finite))
        return problem;
    return checkReal(card.referenceTemperature, "referenceTemperature", RealRule::Finite);
}

void ModuleRigidRodFormat::write(const ModuleRigidRodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.moduleIdA, 0);
    writer.integer(3, card.nodeA);
    writer.integerUnlessDefault(4, card.moduleIdB, 0);
    writer.integer(5, card.nodeB);
    writer.integerUnlessDefault(6, card.componentsAtA, 0);
    writer.integerUnlessDefault(7, card.componentsAtB, 0);
    writer.realUnlessDefault(8, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(9, card.referenceTemperature, 0.0);
}

std::uint32_t ModuleRigidRodTable::append(const ModuleRigidRodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleRigidRodTable::store(std::uint32_t row, const ModuleRigidRodCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleRigidRodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleRigidRodTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleRigidRodTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleRigidRodCard& card = m_cards.edit(row);
    // Each grid point is looked up in its module (ModuleConnectionSupport.hpp).
    visitIdInModule(visitor, "nodeA", ReferenceTarget::Node, card.moduleIdA, card.nodeA);
    visitIdInModule(visitor, "nodeB", ReferenceTarget::Node, card.moduleIdB, card.nodeB);
}

} // namespace oor::model
