#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/results/ResultBlock.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// What turns the values of one kind from the element frame into the frame of a group: the plane tensors (their three
// components in the block, `engineering` when the third is an engineering shear) and the plane vectors.
struct KindTensors
{
    std::vector<std::array<std::size_t, 3>> tensors;
    std::vector<std::array<std::size_t, 2>> vectors;
    bool engineering = false;
};

KindTensors tensorsOf(const ResultBlockKind& kind)
{
    const auto at = [&](const char* name)
    {
        return static_cast<std::size_t>(std::find(kind.components.begin(), kind.components.end(), name) -
                                        kind.components.begin());
    };
    KindTensors result;
    if (kind.kind == "shellForces")
    {
        result.tensors = {{at("nx"), at("ny"), at("nxy")}, {at("mx"), at("my"), at("mxy")}};
        result.vectors = {{at("qx"), at("qy")}};
    }
    else
    {
        result.tensors = {{at("ex0"), at("ey0"), at("gxy0")}, {at("kx"), at("ky"), at("kxy")}};
        result.engineering = true;
    }
    return result;
}

// One element of a group: its weight, and the turn from its element axes to the axes of the group in its plane
// (inPlaneTurn).
struct Member
{
    double weight = NOT_A_NUMBER; // the area of the element for an average, its weight for a sum
    base::Mat3 rotation;
};

base::Mat3 transposed(const base::Mat3& matrix)
{
    base::Mat3 result;
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column)
            result(row, column) = matrix(column, row);
    return result;
}

// The turn from the element axes to the axes of the group in the plane of the element: the x axis of
// the group projected on the plane of the element and normalised, y the normal of the element times it, in the sense
// of the y axis of the group. Rows 0 and 1 of the result, columns 0 and 1: r(i, k) = (axis i) . (element axis k), a
// rotation (or a reflection, for an element whose normal is the other way round) of the plane of the element, so that
// a membrane state uniform in the sense of the structure (the hoop flux of a barrel) is the same in every facet of a
// curved group. On a flat group it is the 2 x 2 block of the axes of the group in those of the element.
base::Mat3 inPlaneTurn(const base::Mat3& groupRows, const base::Mat3& element)
{
    const base::Vec3 gx{groupRows(0, 0), groupRows(0, 1), groupRows(0, 2)};
    const base::Vec3 gy{groupRows(1, 0), groupRows(1, 1), groupRows(1, 2)};
    const base::Vec3 normal = element.column(2);
    base::Vec3 x = gx - base::dot(gx, normal) * normal;
    base::Vec3 y;
    if (base::norm(x) > 1.0e-9)
    {
        x = base::normalized(x);
        y = base::cross(normal, x);
        if (base::dot(y, gy) < 0.0)
            y = -y;
    }
    else
    {
        // the x axis of the group along the normal of the element (a facet at right angles to the group): from y
        y = base::normalized(gy - base::dot(gy, normal) * normal);
        x = base::cross(y, normal);
        if (base::dot(x, gx) < 0.0)
            x = -x;
    }
    base::Mat3 turn;
    for (std::size_t k = 0; k < 3; ++k)
    {
        turn(0, k) = base::dot(x, element.column(k));
        turn(1, k) = base::dot(y, element.column(k));
        turn(2, k) = base::dot(normal, element.column(k));
    }
    return turn;
}

bool parallel(const base::Vec3& one, const base::Vec3& other)
{
    return base::dot(one, other) > 1.0 - 1.0e-6;
}

bool finite(const base::Vec3& vector)
{
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

// The axes given for a group as the rows of a matrix (basic components), x projected on the plane normal to axis 3 (the
// normal given, else the mean normal of the group); QueryError naming the group when they make no frame.
base::Mat3 givenAxes(const GroupAxes& given, const base::Vec3& meanNormal, const std::string& name)
{
    const std::string where = "readBlock: the axes given for the group '" + name + "'";
    if (given.full)
    {
        const base::Mat3 axes = base::Mat3::fromColumns(given.x, given.y, given.z);
        double worst = 0.0;
        for (std::size_t row = 0; row < 3; ++row)
            for (std::size_t column = 0; column < 3; ++column)
                worst = std::max(
                    worst, std::fabs(base::dot(axes.column(row), axes.column(column)) - (row == column ? 1.0 : 0.0)));
        if (!(worst < 1.0e-6))
            throw QueryError(where + " are not orthonormal (the columns of a rotation: axes 1, 2, 3)");
        if (!(base::dot(base::cross(given.x, given.y), given.z) > 0.0))
            throw QueryError(where + " are not right-handed (axis 3 is not axis 1 x axis 2)");
        return transposed(axes);
    }
    const base::Vec3 z = base::normalized(finite(given.z) ? given.z : meanNormal);
    if (!finite(given.x) || !finite(z) || !(base::norm(z) > 0.5))
        throw QueryError(where + " have no x direction or no normal (the mean normal of the group is null)");
    const base::Vec3 projected = given.x - base::dot(given.x, z) * z;
    if (!(base::norm(projected) > 1.0e-9 * base::norm(given.x)))
        throw QueryError(where + ": the x direction is along the normal");
    const base::Vec3 x = base::normalized(projected);
    return transposed(base::Mat3::fromColumns(x, base::cross(z, x), z));
}

} // namespace

ResultBlock readGroupBlock(const QueryRun& run, const ResultBlockKind& kind, const std::vector<std::string>& components,
                           const std::vector<std::vector<std::int32_t>>& groups,
                           const std::vector<std::string>& groupNames, const GroupFrame& frame,
                           const std::vector<std::pair<std::string, std::int32_t>>& casePairs,
                           const std::vector<std::vector<double>>& weights)
{
    if (!frame.axes.empty() && frame.axes.size() != groups.size())
        throw QueryError("readBlock: the axes of the groups are one per group");
    if (!weights.empty())
    {
        if (weights.size() != groups.size())
            throw QueryError("readBlock: the weights of the groups are one list per group");
        for (std::size_t index = 0; index < groups.size(); ++index)
            if (weights[index].size() != groups[index].size())
                throw QueryError("readBlock: the group '" + groupNames[index] + "' has " +
                                 std::to_string(groups[index].size()) + " elements and " +
                                 std::to_string(weights[index].size()) + " weights");
    }
    if (kind.kind != "shellForces" && kind.kind != "laminateStrains")
        throw QueryError("readBlock: elementGroups averages \"shellForces\" and \"laminateStrains\", not '" +
                         kind.kind + "'");
    if (!run.model)
        throw QueryError(
            "readBlock: elementGroups needs the model (the areas and the axes of the elements): read "
            "from a model with the results attached");
    for (const std::string& name : components)
        if (std::find(kind.components.begin(), kind.components.end(), name) == kind.components.end())
            throw QueryError("readBlock: '" + name + "' is not a component of " + kind.kind);
    const model::Model& model = *run.model;

    // the rows of every element of the groups, in their element frame, all the components (the rotation needs them)
    std::vector<std::int32_t> members;
    for (const std::vector<std::int32_t>& group : groups)
        members.insert(members.end(), group.begin(), group.end());
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    const ResultBlock elements = readResultBlock(run, kind, kind.components, members, casePairs);
    const std::size_t places = elements.locationCount;
    const std::size_t width = elements.componentCount;

    // the geometry of each element
    std::vector<analysis::ElementValues> geometry(members.size());
    std::vector<std::uint8_t> located(members.size(), 0);
    for (std::size_t index = 0; index < members.size(); ++index)
        if (const auto location = model.locate(base::IdSpace::Element, members[index]))
        {
            geometry[index] = analysis::ElementGeometry::of(
                model, *location,
                analysis::bit(analysis::Quantity::Area) | analysis::bit(analysis::Quantity::Center) |
                    analysis::bit(analysis::Quantity::Frame) | analysis::bit(analysis::Quantity::MaterialFrame));
            located[index] = std::isfinite(geometry[index].area) && geometry[index].area > 0.0 ? 1 : 0;
        }
    const auto memberIndex = [&](std::int32_t eid)
    { return static_cast<std::size_t>(std::lower_bound(members.begin(), members.end(), eid) - members.begin()); };
    // the place of the centre of each element in the block
    const auto centreOf = [&](std::size_t element) -> std::optional<std::size_t>
    {
        for (std::size_t place = 0; place < places; ++place)
        {
            const std::size_t at = element * places + place;
            if (elements.mask[at] != 0 && elements.locationNames[at] == "center")
                return place;
        }
        return std::nullopt;
    };

    const KindTensors parts = tensorsOf(kind);
    analysis::CoordinateTransform transform(model);
    ResultBlock block;
    block.caseCount = elements.caseCount;
    block.elementCount = groups.size();
    block.locationCount = 1;
    block.components = components.empty() ? kind.components : components;
    block.componentCount = block.components.size();
    block.single = elements.single;
    block.values.assign(block.caseCount * block.elementCount * block.componentCount, NOT_A_NUMBER);
    block.mask.assign(block.elementCount, 0);
    block.locationNames.assign(block.elementCount, "center");
    block.locationNodes.assign(block.elementCount, 0);
    block.layers.assign(block.elementCount, 0);
    block.layerIds.assign(block.elementCount, 0);
    block.positions.assign(block.elementCount, NOT_A_NUMBER);
    block.caseAnalyses = elements.caseAnalyses;
    block.caseSubcases = elements.caseSubcases;
    block.caseSteps = elements.caseSteps;
    for (const std::string& source : elements.sources)
        block.sources.push_back(source.empty() ? source
                                               : source + (weights.empty() ? "; area-weighted average over each group"
                                                                           : "; weighted sum over each group"));
    std::vector<std::size_t> columnOf(block.componentCount);
    for (std::size_t index = 0; index < block.componentCount; ++index)
        columnOf[index] = static_cast<std::size_t>(
            std::find(kind.components.begin(), kind.components.end(), block.components[index]) -
            kind.components.begin());

    const bool summed = !weights.empty();
    const double cosineLimit = std::cos(frame.normalTolerance * std::numbers::pi / 180.0);
    // the place of the centre of each element, the same in every case
    std::vector<std::optional<std::size_t>> centrePlace(members.size());
    for (std::size_t element = 0; element < members.size(); ++element)
        centrePlace[element] = centreOf(element);
    // The members of each group with their turns, made in the order of the groups on the calling thread (the first
    // group that cannot be made raises); then the sums of the groups over the cases, on the threads of the call
    // (a companion package: a complete base reads thousands of groups in hundreds of cases). Each group writes its own
    // values: the same numbers whatever the threads.
    struct GroupSetup
    {
        std::vector<std::size_t> kept; // the positions of its elements in `members`
        std::vector<Member> members;
    };
    std::vector<GroupSetup> setups(groups.size());
    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex)
    {
        const std::string& name = groupNames[groupIndex];
        block.elementIds.push_back(static_cast<std::int32_t>(groupIndex));
        // the elements of the group that are in the model, their weights, the centre and the mean normal of the group
        // (each normal taken in the sense of the sum so far, so that a layer the other way round does not cancel it)
        std::vector<std::size_t> kept;
        std::vector<double> keptWeights;
        base::Vec3 centre{0.0, 0.0, 0.0};
        base::Vec3 normal{0.0, 0.0, 0.0};
        double area = 0.0;
        for (std::size_t position = 0; position < groups[groupIndex].size(); ++position)
        {
            const std::int32_t eid = groups[groupIndex][position];
            const std::size_t index = memberIndex(eid);
            if (index >= members.size() || members[index] != eid)
                continue;
            if (located[index] == 0)
                throw QueryError("readBlock: the element " + std::to_string(eid) + " of the group '" + name +
                                 "' has no area in the model (not a shell, or not in the model)");
            kept.push_back(index);
            keptWeights.push_back(summed ? weights[groupIndex][position] : geometry[index].area);
            const analysis::ElementValues& one = geometry[index];
            centre = centre + one.center * one.area;
            area += one.area;
            const base::Vec3 own = one.frame.column(2);
            if (finite(own))
                normal = normal + (base::dot(own, normal) < 0.0 ? -one.area : one.area) * own;
        }
        if (kept.empty() || !(area > 0.0))
            continue;
        centre = centre * (1.0 / area);
        const base::Vec3 meanNormal =
            base::norm(normal) > 0.0 ? base::normalized(normal) : base::Vec3{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
        // the axes of the group, the rows of `axes` (basic components)
        base::Mat3 axes;
        if (!frame.axes.empty())
        {
            axes = givenAxes(frame.axes[groupIndex], meanNormal, name);
        }
        else if (frame.material)
        {
            const analysis::ElementValues& first = geometry[kept.front()];
            for (const std::size_t index : kept)
            {
                const analysis::ElementValues& one = geometry[index];
                if (!parallel(one.materialFrame.column(0), first.materialFrame.column(0)) ||
                    !parallel(one.frame.column(2), first.frame.column(2)))
                    throw QueryError("readBlock: the elements of the group '" + name +
                                     "' do not share their material axes (or their normal): give a coordinate system "
                                     "or the axes of each group as the frame");
            }
            axes = transposed(base::Mat3::fromColumns(first.materialFrame.column(0),
                                                      base::cross(first.frame.column(2), first.materialFrame.column(0)),
                                                      first.frame.column(2)));
        }
        else
        {
            axes = transposed(transform.displacementRotation(frame.system, centre));
        }
        // axis 3 of the frame along the mean normal of the group, in either sense
        const base::Vec3 axis3{axes(2, 0), axes(2, 1), axes(2, 2)};
        const double cosine = std::fabs(base::dot(axis3, meanNormal)) / base::norm(axis3);
        if (!(cosine >= cosineLimit))
        {
            const double angle = std::acos(std::min(1.0, cosine)) * 180.0 / std::numbers::pi;
            throw QueryError("readBlock: axis 3 of the frame of the group '" + name + "' is " +
                             (std::isfinite(angle) ? std::to_string(angle) + " degrees" : "at an unknown angle") +
                             " from the mean normal of its elements (normalTolerance " +
                             std::to_string(frame.normalTolerance) +
                             " degrees): the plane of the frame is not that of the shells");
        }
        GroupSetup& setup = setups[groupIndex];
        for (std::size_t position = 0; position < kept.size(); ++position)
            setup.members.push_back(Member{keptWeights[position], inPlaneTurn(axes, geometry[kept[position]].frame)});
        setup.kept = std::move(kept);
    }

    constexpr std::size_t MOST_COMPONENTS = 16;
    if (width > MOST_COMPONENTS)
        throw QueryError("readBlock: a kind of more than 16 components is not averaged over groups");
    const auto sumGroup = [&](std::size_t groupIndex)
    {
        const GroupSetup& setup = setups[groupIndex];
        if (setup.kept.empty())
            return;
        bool any = false;
        for (std::size_t caseIndex = 0; caseIndex < block.caseCount; ++caseIndex)
        {
            std::array<double, MOST_COMPONENTS> sum{};
            double weight = 0.0;
            bool incomplete = false;
            for (std::size_t position = 0; position < setup.kept.size(); ++position)
            {
                const std::size_t element = setup.kept[position];
                const std::optional<std::size_t>& place = centrePlace[element];
                const double* row =
                    place ? elements.values.data() + ((caseIndex * members.size() + element) * places + *place) * width
                          : nullptr;
                // an element with no value in the case: left out of an average; a sum is unknown without it
                if (row == nullptr || std::all_of(row, row + width, [](double value) { return std::isnan(value); }))
                {
                    incomplete = true;
                    continue;
                }
                std::array<double, MOST_COMPONENTS> turned{};
                std::copy(row, row + width, turned.begin());
                const base::Mat3& r = setup.members[position].rotation;
                for (const std::array<std::size_t, 3>& tensor : parts.tensors)
                {
                    const double xy = parts.engineering ? row[tensor[2]] / 2.0 : row[tensor[2]];
                    const double t[2][2] = {{row[tensor[0]], xy}, {xy, row[tensor[1]]}};
                    // r t r^T over the plane of the element (the tensor has no out-of-plane part there)
                    double out[2][2] = {};
                    for (std::size_t i = 0; i < 2; ++i)
                        for (std::size_t j = 0; j < 2; ++j)
                            for (std::size_t k = 0; k < 2; ++k)
                                for (std::size_t l = 0; l < 2; ++l)
                                    out[i][j] += r(i, k) * t[k][l] * r(j, l);
                    turned[tensor[0]] = out[0][0];
                    turned[tensor[1]] = out[1][1];
                    turned[tensor[2]] = parts.engineering ? 2.0 * out[0][1] : out[0][1];
                }
                for (const std::array<std::size_t, 2>& vector : parts.vectors)
                {
                    turned[vector[0]] = r(0, 0) * row[vector[0]] + r(0, 1) * row[vector[1]];
                    turned[vector[1]] = r(1, 0) * row[vector[0]] + r(1, 1) * row[vector[1]];
                }
                for (std::size_t column = 0; column < width; ++column)
                    sum[column] += setup.members[position].weight * turned[column];
                weight += setup.members[position].weight;
            }
            if (summed ? incomplete : !(weight > 0.0))
                continue;
            any = true;
            for (std::size_t component = 0; component < block.componentCount; ++component)
                block.values[(caseIndex * block.elementCount + groupIndex) * block.componentCount + component] =
                    summed ? sum[columnOf[component]] : sum[columnOf[component]] / weight;
        }
        block.mask[groupIndex] = any ? 1 : 0;
    };
    if (base::ThreadPool* pool = run.context.pool(); pool != nullptr && groups.size() > 1)
        base::parallelFor(*pool, groups.size(), sumGroup, run.context.token());
    else
        for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex)
            sumGroup(groupIndex);
    return block;
}

} // namespace oor::query
