#include "oor/model/cards/nodes/moduleBoundaryNodes/ModuleBoundaryNodesTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<ModuleBoundaryNodesCard> ModuleBoundaryNodesFormat::read(FieldReader& reader)
{
    ModuleBoundaryNodesCard card;
    card.moduleIdA = reader.integer(1, "moduleIdA", 0);
    card.otherModule = readModuleOrAll(reader, 2, "otherModule", ModuleOrAll::everything());
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodesA");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleBoundaryNodesFormat::check(const ModuleBoundaryNodesCard& card)
{
    if (const auto problem = checkBoundaryModule(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkModuleOrAll(card.otherModule, "otherModule", true))
        return problem;
    return checkIdRuns(card.nodes, "nodesA");
}

void ModuleBoundaryNodesFormat::write(const ModuleBoundaryNodesCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.moduleIdA, 0);
    writeModuleOrAll(writer, 2, card.otherModule, true);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t ModuleBoundaryNodesTable::append(const ModuleBoundaryNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_moduleIdA.push(card.moduleIdA);
    m_cards.push(card);
    return row;
}

void ModuleBoundaryNodesTable::store(std::uint32_t row, const ModuleBoundaryNodesCard& card)
{
    m_moduleIdA.set(row, card.moduleIdA);
    m_cards.set(row, card);
}

void ModuleBoundaryNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_moduleIdA.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleBoundaryNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleBoundaryNodesCard& card = m_cards.edit(row);
    visitBoundaryRuns(visitor, "nodesA", card.moduleIdA, card.nodes, ReferenceTarget::Node,
                      ReferenceTarget::NodeInRange);
}

} // namespace oor::model
