#pragma once

#include "oor/analysis/adjacency/ElementIndexes.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/query/named/NamedQuery.hpp"
#include "oor/query/named/common/CardList.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::query
{

// What the boundary queries compare between neighbouring shells.
struct ShellNeeds
{
    bool normals = false;
    bool thickness = false;
    bool material = false;
    bool area = false;
    bool center = false;
};

// The two-dimensional elements of a model (shells, plane and axisymmetric elements, shear panels), by ID, with the data
// the boundary queries need (property, normal, thickness, main material), and the selection a call restricted them to.
// An element that is not selected is treated as absent: the free edges of a selection are the boundary of that region.
class ShellSet
{
public:
    ShellSet(const QueryRun& run, const std::optional<std::vector<std::int64_t>>& selected, ShellNeeds needs);

    [[nodiscard]] std::size_t size() const noexcept { return m_list.size(); }
    [[nodiscard]] const CardList& list() const noexcept { return m_list; }
    // The position in the list of a shell, -1 for an element that is not a shell.
    [[nodiscard]] std::int64_t indexOf(const analysis::ElementRef& element) const noexcept;
    [[nodiscard]] bool isSelected(std::size_t index) const noexcept
    {
        return m_selected.empty() || m_selected[index] != 0;
    }

    [[nodiscard]] std::int32_t id(std::size_t index) const noexcept { return m_list.ids[index]; }
    [[nodiscard]] std::int32_t property(std::size_t index) const noexcept { return m_properties[index]; }
    [[nodiscard]] double thickness(std::size_t index) const noexcept { return m_thickness[index]; }
    [[nodiscard]] std::int32_t material(std::size_t index) const noexcept { return m_materials[index]; }
    [[nodiscard]] double area(std::size_t index) const noexcept { return m_areas[index]; }
    [[nodiscard]] base::Vec3 center(std::size_t index) const noexcept
    {
        return base::Vec3{m_centers[3 * index], m_centers[3 * index + 1], m_centers[3 * index + 2]};
    }
    // The nodes of a shell in the order of the card (corner and mid-side nodes).
    [[nodiscard]] std::span<const std::int32_t> nodes(std::size_t index) const noexcept { return m_nodes.of(index); }
    [[nodiscard]] base::Vec3 normal(std::size_t index) const noexcept
    {
        return base::Vec3{m_normals[3 * index], m_normals[3 * index + 1], m_normals[3 * index + 2]};
    }

    // The corner nodes of a shell in the order of the card (3 or 4).
    [[nodiscard]] std::vector<std::int32_t> corners(std::size_t index) const;
    // True when the shell goes along the edge n1 -> n2 (the next corner after n1 is n2), false when it goes n2 -> n1 or
    // the shell does not have the edge.
    [[nodiscard]] bool runsForward(std::size_t index, std::int32_t n1, std::int32_t n2) const;

private:
    CardList m_list;
    NodeLists m_nodes;
    std::vector<std::vector<std::int32_t>> m_indexByRow; // by card type, then row
    std::vector<std::uint8_t> m_selected;                // empty: every shell
    std::vector<std::int32_t> m_properties;
    std::vector<double> m_thickness;
    std::vector<double> m_normals;
    std::vector<std::int32_t> m_materials;
    std::vector<double> m_areas;
    std::vector<double> m_centers;
};

} // namespace oor::query
