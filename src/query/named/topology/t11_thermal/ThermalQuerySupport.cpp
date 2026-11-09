#include "ThermalQuerySupport.hpp"

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"

#include <algorithm>
#include <cstdlib>

namespace oor::query::thermal
{

SurfaceElements::SurfaceElements(const model::Model& model, base::ThreadPool* pool)
{
    constexpr std::uint8_t QUANTITIES = analysis::bit(analysis::Quantity::Area) |
                                        analysis::bit(analysis::Quantity::Normal) |
                                        analysis::bit(analysis::Quantity::Center);
    for (const std::uint16_t type :
         {model::typeIndexOf<model::HeatSurfaceSideTable>(), model::typeIndexOf<model::HeatSurfaceNodesTable>(),
          model::typeIndexOf<model::HeatSurfacePropertyTable>()})
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        std::vector<std::uint32_t> rows;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                rows.push_back(row);
        }
        if (rows.empty())
            continue;
        const analysis::ElementArrays arrays = analysis::ElementGeometry::compute(model, type, rows, QUANTITIES, pool);
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            const base::EntityId eid = table->idAt(rows[index]);
            m_entries[eid] = SurfaceElement{model::CardLocation{type, rows[index]}, arrays.areas[index],
                                            arrays.normals[index], arrays.centers[index]};
            m_ids.push_back(eid);
        }
    }
    std::sort(m_ids.begin(), m_ids.end());
    m_ids.erase(std::unique(m_ids.begin(), m_ids.end()), m_ids.end());
}

const SurfaceElement* SurfaceElements::find(base::EntityId eid) const noexcept
{
    const auto found = m_entries.find(eid);
    return found == m_entries.end() ? nullptr : &found->second;
}

std::vector<base::EntityId> existingIdsOfRuns(const std::vector<model::ThermalRun>& runs,
                                              std::span<const base::EntityId> existing)
{
    std::vector<base::EntityId> ids;
    for (const model::ThermalRun& run : runs)
    {
        if (run.first == run.last)
        {
            const base::EntityId id = std::abs(run.first);
            if (std::binary_search(existing.begin(), existing.end(), id))
                ids.push_back(id);
            continue;
        }
        const base::EntityId step = run.increment > 0 ? run.increment : 1;
        for (auto at = std::lower_bound(existing.begin(), existing.end(), run.first);
             at != existing.end() && *at <= run.last; ++at)
        {
            if ((*at - run.first) % step == 0)
                ids.push_back(*at);
        }
    }
    return ids;
}

} // namespace oor::query::thermal
