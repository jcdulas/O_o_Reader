#include "oor/query/named/common/MassGroups.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace oor::query
{

void addElementMass(GroupTotals& group, const ElementMassData& masses, std::size_t index, double structuralShare,
                    bool withNonStructural, bool withInertia, bool counted)
{
    if (!std::isfinite(masses.mass[index]))
        return;
    const double structural = masses.structural[index] * structuralShare;
    const double nonStructural = withNonStructural ? masses.nonStructural[index] : 0.0;
    if (counted)
        ++group.count;
    group.structural += structural;
    group.nonStructural += nonStructural;
    const base::Vec3 center{masses.cx[index], masses.cy[index], masses.cz[index]};
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z))
        return;
    const auto own = withInertia ? masses.inertia.find(index) : masses.inertia.end();
    if (own != masses.inertia.end())
    {
        const base::Mat3 tensor = own->second;
        group.sums.add(structural + nonStructural, center, &tensor);
    }
    else
    {
        group.sums.add(structural + nonStructural, center);
    }
}

std::map<std::int32_t, GroupTotals> materialGroups(ElementData& data)
{
    std::map<std::int32_t, GroupTotals> groups;
    const CardList& elements = data.list();
    if (elements.size() == 0)
        return groups;
    const std::vector<std::int32_t>& pids = data.properties();
    const ElementMassData& masses = data.masses();
    const auto addNonStructural = [&](std::size_t index)
    {
        if (masses.nonStructural[index] == 0.0)
            return;
        GroupTotals& rest = groups[0];
        ++rest.count;
        rest.nonStructural += masses.nonStructural[index];
        if (std::isfinite(masses.cx[index]))
            rest.sums.add(masses.nonStructural[index],
                          base::Vec3{masses.cx[index], masses.cy[index], masses.cz[index]});
    };
    for (std::size_t index = 0; index < elements.size(); ++index)
    {
        if (!std::isfinite(masses.mass[index]))
            continue;
        const std::int32_t material = masses.material[index];
        const auto* shares = material == 0 ? data.laminateShares(pids[index]) : nullptr;
        if (material != 0)
        {
            addElementMass(groups[material], masses, index, 1.0, false, true);
            addNonStructural(index);
        }
        else if (shares != nullptr)
        {
            for (const auto& [id, share] : *shares)
                addElementMass(groups[id], masses, index, share, false, false);
            addNonStructural(index);
        }
        else
        {
            addElementMass(groups[0], masses, index, 1.0, true, true, masses.mass[index] != 0.0);
        }
    }
    return groups;
}

std::vector<std::size_t> orderByMass(const std::vector<GroupTotals>& groups)
{
    std::vector<std::size_t> order(groups.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right) { return groups[left].mass() > groups[right].mass(); });
    return order;
}

void addMassColumns(TableBuilder& builder, const std::vector<GroupTotals>& groups,
                    const std::vector<std::size_t>& order)
{
    std::vector<std::int64_t> count;
    std::vector<double> structural;
    std::vector<double> nonStructural;
    std::vector<double> mass;
    std::array<std::vector<double>, 3> centre;
    for (const std::size_t index : order)
    {
        const GroupTotals& group = groups[index];
        count.push_back(group.count);
        structural.push_back(group.structural);
        nonStructural.push_back(group.nonStructural);
        mass.push_back(group.mass());
        const base::Vec3 cg = group.sums.centerOfGravity();
        centre[0].push_back(cg.x);
        centre[1].push_back(cg.y);
        centre[2].push_back(cg.z);
    }
    builder.int64("elementCount", std::move(count));
    builder.real("structuralMass", std::move(structural));
    builder.real("nonstructuralMass", std::move(nonStructural));
    builder.real("mass", std::move(mass));
    builder.real("cx", std::move(centre[0]));
    builder.real("cy", std::move(centre[1]));
    builder.real("cz", std::move(centre[2]));
}

} // namespace oor::query
