#include "oor/model/cards/nodes/moduleExcludedNodes/ModuleExcludedNodesTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<ModuleExcludedNodesCard> ModuleExcludedNodesFormat::read(FieldReader& reader)
{
    ModuleExcludedNodesCard card;
    card.moduleIdA = reader.integer(1, "moduleIdA", 0);
    card.otherModule = readModuleOrAll(reader, 2, "otherModule", ModuleOrAll::everything());
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodesA");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleExcludedNodesFormat::check(const ModuleExcludedNodesCard& card)
{
    if (const auto problem = checkBoundaryModule(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkModuleOrAll(card.otherModule, "otherModule", true))
        return problem;
    return checkIdRuns(card.nodes, "nodesA");
}

void ModuleExcludedNodesFormat::write(const ModuleExcludedNodesCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.moduleIdA, 0);
    writeModuleOrAll(writer, 2, card.otherModule, true);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t ModuleExcludedNodesTable::append(const ModuleExcludedNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_moduleIdA.push(card.moduleIdA);
    m_cards.push(card);
    return row;
}

void ModuleExcludedNodesTable::store(std::uint32_t row, const ModuleExcludedNodesCard& card)
{
    m_moduleIdA.set(row, card.moduleIdA);
    m_cards.set(row, card);
}

void ModuleExcludedNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_moduleIdA.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleExcludedNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleExcludedNodesCard& card = m_cards.edit(row);
    visitBoundaryRuns(visitor, "nodesA", card.moduleIdA, card.nodes, ReferenceTarget::Node,
                      ReferenceTarget::NodeInRange);
}

} // namespace oor::model
