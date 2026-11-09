#include "oor/query/named/common/ShellSet.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/common/Topology.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <unordered_map>

namespace oor::query
{

ShellSet::ShellSet(const QueryRun& run, const std::optional<std::vector<std::int64_t>>& selected, ShellNeeds needs)
{
    model::Model& model = *run.model;
    std::vector<std::uint16_t> types;
    for (const std::uint16_t type : topologicalTypes())
    {
        if (analysis::dimensionOf(analysis::ElementTopology::shapeOf(type)) == 2)
            types.push_back(type);
    }
    m_list = listCards(model, types);
    m_nodes = nodesOfCards(model, m_list, run.context);
    const std::size_t count = m_list.size();
    m_indexByRow.resize(model::CardRegistry::instance().size());
    for (std::size_t index = 0; index < count; ++index)
    {
        std::vector<std::int32_t>& rows = m_indexByRow[m_list.types[index]];
        if (rows.empty())
            rows.assign(model.findTable(m_list.types[index])->rows().rowCount(), -1);
        rows[m_list.rows[index]] = static_cast<std::int32_t>(index);
    }
    if (selected)
    {
        const auto sorted = sortedIds(selected);
        m_selected.assign(count, 0);
        for (std::size_t index = 0; index < count; ++index)
            m_selected[index] =
                std::binary_search(sorted->begin(), sorted->end(), static_cast<std::int64_t>(m_list.ids[index])) ? 1
                                                                                                                 : 0;
    }
    if (count == 0)
        return;
    ListColumns columns(run.model, m_list, run.context);
    m_properties = asInt32(columns.get("propId"));
    if (needs.normals)
    {
        const std::vector<double> x = columns.reals("normalX");
        const std::vector<double> y = columns.reals("normalY");
        const std::vector<double> z = columns.reals("normalZ");
        m_normals.resize(3 * count);
        for (std::size_t index = 0; index < count; ++index)
        {
            m_normals[3 * index] = x[index];
            m_normals[3 * index + 1] = y[index];
            m_normals[3 * index + 2] = z[index];
        }
    }
    if (needs.thickness)
        m_thickness = columns.reals("thickness");
    if (needs.area)
        m_areas = columns.reals("area");
    if (needs.center)
    {
        const std::vector<double> x = columns.reals("centerX");
        const std::vector<double> y = columns.reals("centerY");
        const std::vector<double> z = columns.reals("centerZ");
        m_centers.resize(3 * count);
        for (std::size_t index = 0; index < count; ++index)
        {
            m_centers[3 * index] = x[index];
            m_centers[3 * index + 1] = y[index];
            m_centers[3 * index + 2] = z[index];
        }
    }
    if (needs.material)
    {
        CardFacts facts(model);
        m_materials.assign(count, 0);
        std::unordered_map<std::int32_t, std::int32_t> byProperty;
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::int32_t pid = m_properties[index];
            if (pid == 0)
                continue;
            const auto cached = byProperty.find(pid);
            if (cached != byProperty.end())
            {
                m_materials[index] = cached->second;
                continue;
            }
            std::int32_t material = 0;
            if (const auto location = model.locate(base::IdSpace::Property, pid))
                material = facts.firstReference(*location, model::ReferenceTarget::Material);
            byProperty.emplace(pid, material);
            m_materials[index] = material;
        }
    }
}

std::int64_t ShellSet::indexOf(const analysis::ElementRef& element) const noexcept
{
    if (element.type >= m_indexByRow.size())
        return -1;
    const std::vector<std::int32_t>& rows = m_indexByRow[element.type];
    return element.row < rows.size() ? rows[element.row] : -1;
}

std::vector<std::int32_t> ShellSet::corners(std::size_t index) const
{
    std::vector<std::int32_t> result;
    const auto nodes = m_nodes.of(index);
    for (std::size_t position = 0; position < nodes.size(); ++position)
    {
        if (isCornerPosition(m_list.types[index], position))
            result.push_back(nodes[position]);
    }
    return result;
}

bool ShellSet::runsForward(std::size_t index, std::int32_t n1, std::int32_t n2) const
{
    std::array<std::int32_t, 4> ring{};
    std::size_t count = 0;
    const auto nodes = m_nodes.of(index);
    for (std::size_t position = 0; position < nodes.size() && count < ring.size(); ++position)
    {
        if (isCornerPosition(m_list.types[index], position))
            ring[count++] = nodes[position];
    }
    for (std::size_t position = 0; position < count; ++position)
    {
        if (ring[position] == n1 && ring[(position + 1) % count] == n2)
            return true;
    }
    return false;
}

} // namespace oor::query
