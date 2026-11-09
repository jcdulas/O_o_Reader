#include "oor/analysis/validation/engine/ValidationContext.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/nodes/scalarPoint/ScalarPointTable.hpp"

#include <algorithm>

namespace oor::analysis
{

ValidationContext::ValidationContext(const model::Model& root, const model::Model& model, std::string part,
                                     std::size_t partIndex, const ValidationOptions& options)
    : m_root(root), m_model(model), m_part(std::move(part)), m_partIndex(partIndex), m_options(options)
{
}

std::optional<std::int64_t> ValidationContext::solution() const
{
    return m_root.control().executive.solutionNumber();
}

const ReferenceCensus& ValidationContext::references() const
{
    std::call_once(
        m_censusFlag,
        [this]
        {
            m_census = std::make_unique<ReferenceCensus>(ReferenceCensus::build(m_scan != nullptr ? *m_scan : m_model));
        });
    return *m_census;
}

const ElementMeasures& ValidationContext::measures() const
{
    std::call_once(m_measuresFlag,
                   [this] { m_measures = std::make_unique<ElementMeasures>(ElementMeasures::build(m_model)); });
    return *m_measures;
}

const std::vector<base::EntityId>& ValidationContext::sortedNodeIds() const
{
    std::call_once(m_nodesFlag,
                   [this]
                   {
                       const auto gather = [this](const model::CardTable* table)
                       {
                           if (table == nullptr)
                               return;
                           for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                           {
                               if (table->rows().isAlive(row))
                                   m_nodeIds.push_back(table->idAt(row));
                           }
                       };
                       gather(m_model.findTable<model::NodeTable>());
                       gather(m_model.findTable<model::ScalarPointTable>());
                       std::ranges::sort(m_nodeIds);
                   });
    return m_nodeIds;
}

} // namespace oor::analysis
