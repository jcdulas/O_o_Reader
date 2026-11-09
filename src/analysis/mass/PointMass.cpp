#include "MassProvider.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MassProperties.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>

// The concentrated masses.
//
//   CONM2   the mass M at its centre of gravity (the grid point plus the offset X1, X2, X3 read in the system CID, or
//   the
//           point itself for CID -1), with the inertia I11 ... I33 taken about that point in the axes of CID
//   CONM1   the translational mass is the mean of the three diagonal terms of the matrix (M11, M22, M33), the centre of
//           gravity follows from the coupling between translation and rotation (a mass m at an offset r from the grid
//           point gives m [r] there), and the inertia is the rotational block less the part the offset explains; the
//           matrix is in the axes of CID
//   CMASS1, CMASS2   the mass at the grid point (the middle of the two when it joins two), when one of its components
//   is
//           a translation (1 to 3); a mass on a rotation (4 to 6) is an inertia about that axis and no mass. A mass
//           between scalar points has no place and is not counted: the format reference says the grid point weight
//           generator ignores masses on scalar points. CMASS3 and CMASS4 are such masses and have no provider here.
//   CBUSH, CFAST   the mass M of the PBUSH or the PFAST, at the point of the element

namespace oor::analysis::detail
{

namespace
{

// The inertia tensor of the card: the values I11 I21 I22 I31 I32 I33 are the integrals of the products with a positive
// sign, the tensor has them negative off the diagonal.
base::Mat3 pointMassInertiaTensor(const std::array<double, 6>& values)
{
    return base::Mat3{
        {values[0], -values[1], -values[3], -values[1], values[2], -values[4], -values[3], -values[4], values[5]}};
}

base::Mat3 zeroMatrix()
{
    return base::Mat3{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
}

// The row of a grid point in the Node table (-1 when the node is not a grid point).
std::int32_t nodeRowOf(const model::Model& model, base::EntityId node)
{
    const auto location = model.locate(base::IdSpace::Node, node);
    if (!location || location->table != model::typeIndexOf<model::NodeTable>())
        return -1;
    return static_cast<std::int32_t>(location->row);
}

struct PointMassInertiaShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center);

    static void evaluate(const model::PointMassInertiaTable& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::PointMassInertiaCard card = table.card(row);
        value.center = geometry.center;
        value.structural = card.mass;
        const bool hasInertia =
            std::any_of(card.inertia.begin(), card.inertia.end(), [](double value) { return value != 0.0; });
        if (!hasInertia)
            return;
        base::Mat3 rotation = base::Mat3::identity();
        if (card.frameId > 0)
        {
            const std::int32_t gridRow = nodeRowOf(context.model(), card.node);
            if (gridRow >= 0)
            {
                rotation = context.geometry().systemAxesAt(
                    card.frameId, context.geometry().cache().position(static_cast<std::uint32_t>(gridRow)));
            }
        }
        value.inertia = rotatedInertia(rotation, pointMassInertiaTensor(card.inertia));
        value.hasInertia = true;
    }
};

struct PointMassMatrixShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center);

    static void evaluate(const model::PointMassMatrixTable& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::PointMassMatrixCard card = table.card(row);
        // The 6 x 6 matrix as three 3 x 3 blocks: translation, coupling (rows are translations, columns rotations) and
        // rotation.
        base::Mat3 translation = zeroMatrix();
        base::Mat3 coupling = zeroMatrix();
        base::Mat3 rotation = zeroMatrix();
        const auto put = [&](std::size_t i, std::size_t j, double term)
        {
            if (i < 3 && j < 3)
                translation(i, j) = term;
            else if (i < 3)
                coupling(i, j - 3) = term;
            else if (j < 3)
                coupling(j, i - 3) = term;
            else
                rotation(i - 3, j - 3) = term;
        };
        for (std::size_t r = 1; r <= 6; ++r)
        {
            for (std::size_t c = 1; c <= r; ++c)
            {
                const double term = card.massMatrix[model::PointMassMatrixCard::indexOf(r, c)];
                put(r - 1, c - 1, term);
                put(c - 1, r - 1, term);
            }
        }
        if (card.frameId > 0)
        {
            const std::int32_t gridRow = nodeRowOf(context.model(), card.node);
            if (gridRow >= 0)
            {
                const base::Mat3 axes = context.geometry().systemAxesAt(
                    card.frameId, context.geometry().cache().position(static_cast<std::uint32_t>(gridRow)));
                translation = rotatedInertia(axes, translation);
                coupling = rotatedInertia(axes, coupling);
                rotation = rotatedInertia(axes, rotation);
            }
        }
        const double mass = (translation(0, 0) + translation(1, 1) + translation(2, 2)) / 3.0;
        base::Vec3 offset;
        if (mass != 0.0)
        {
            // A mass m at r from the grid point couples translation and rotation by m [[0, z, -y], [-z, 0, x], [y, -x,
            // 0]].
            offset = base::Vec3{(coupling(1, 2) - coupling(2, 1)) / 2.0, (coupling(2, 0) - coupling(0, 2)) / 2.0,
                                (coupling(0, 1) - coupling(1, 0)) / 2.0} /
                     mass;
        }
        value.structural = mass;
        value.center = geometry.center + offset;
        const base::Mat3 explained = pointMassInertia(mass, offset);
        for (std::size_t index = 0; index < 9; ++index)
            value.inertia.values[index] = rotation.values[index] - explained.values[index];
        value.hasInertia = true;
    }
};

// A scalar mass between grid points or a grid point and ground, on translations or on rotations.
void scalarMass(MassContext& context, double mass, base::EntityId g1, std::int32_t c1, base::EntityId g2,
                std::int32_t c2, const ElementValues& geometry, MassValue& value)
{
    const auto translation = [](std::int32_t component) { return component >= 1 && component <= 3; };
    const auto rotation = [](std::int32_t component) { return component >= 4 && component <= 6; };
    value.center = geometry.center;
    if (translation(c1) || translation(c2))
    {
        value.structural = mass;
        return;
    }
    // A mass on a rotation is the inertia about the axis of that displacement component at the grid point.
    const bool first = rotation(c1);
    const bool second = rotation(c2);
    if (!first && !second)
        return;
    const base::EntityId node = first ? g1 : g2;
    const std::int32_t component = first ? c1 : c2;
    const std::int32_t gridRow = nodeRowOf(context.model(), node);
    if (gridRow < 0)
        return;
    const base::Mat3 axes = context.geometry().displacementRotation(static_cast<std::uint32_t>(gridRow));
    const base::Vec3 axis = axes.column(static_cast<std::size_t>(component - 4));
    value.center = context.geometry().cache().position(static_cast<std::uint32_t>(gridRow));
    value.inertia = base::Mat3{{mass * axis.x * axis.x, mass * axis.x * axis.y, mass * axis.x * axis.z,
                                mass * axis.y * axis.x, mass * axis.y * axis.y, mass * axis.y * axis.z,
                                mass * axis.z * axis.x, mass * axis.z * axis.y, mass * axis.z * axis.z}};
    value.hasInertia = true;
}

struct MassDofPropertyShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center);

    static void evaluate(const model::MassDofPropertyTable& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::MassDofPropertyCard card = table.card(row);
        value.property = card.propId;
        const double mass = context.lumpedMass(card.propId, Owner{model::MassDofPropertyTable::NAME.data(), card.id}, true);
        scalarMass(context, mass, card.node1, card.components1, card.node2, card.components2, geometry, value);
    }
};

struct MassDofInlineShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center);

    static void evaluate(const model::MassDofInlineTable& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::MassDofInlineCard card = table.card(row);
        scalarMass(context, card.mass, card.node1, card.components1, card.node2, card.components2, geometry, value);
    }
};

// A bush or a fastener carries the mass of its property.
template <typename TableT>
struct LumpedShape
{
    static constexpr std::uint8_t QUANTITIES = bit(Quantity::Center);

    static void evaluate(const TableT& table, std::uint32_t row, MassContext& context, const ElementValues& geometry,
                         MassValue& value)
    {
        const base::EntityId pid = table.card(row).propId;
        value.property = pid;
        value.center = geometry.center;
        value.structural = context.lumpedMass(pid, Owner{TableT::NAME.data(), table.idAtRow(row)}, false);
    }
};

} // namespace

void registerPointMass(MassRegistry& registry)
{
    registry.add<model::PointMassMatrixTable, PointMassMatrixShape>();
    registry.add<model::PointMassInertiaTable, PointMassInertiaShape>();
    registry.add<model::MassDofPropertyTable, MassDofPropertyShape>();
    registry.add<model::MassDofInlineTable, MassDofInlineShape>();
    registry.add<model::BushingTable, LumpedShape<model::BushingTable>>();
    registry.add<model::FastenerTable, LumpedShape<model::FastenerTable>>();
}

} // namespace oor::analysis::detail
