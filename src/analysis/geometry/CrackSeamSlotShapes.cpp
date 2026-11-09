#include "ElementProvider.hpp"
#include "ShellGeometry.hpp"
#include "oor/model/cards/elements/acousticSlot3/AcousticSlot3Table.hpp"
#include "oor/model/cards/elements/acousticSlot4/AcousticSlot4Table.hpp"
#include "oor/model/cards/elements/crackTip2D/CrackTip2DTable.hpp"
#include "oor/model/cards/elements/crackTip3D/CrackTip3DTable.hpp"
#include "oor/model/cards/elements/seam/SeamTable.hpp"
#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitTable.hpp"

// The crack tip, seam, slot and spring elements.
//
// What each gives:
//   CRAC2D, CRAC3D  the nodes only (G1 to G18 and G1 to G64, 0 for a blank field), so that the element is in the
//                   node-to-element index; every quantity is NaN. The format reference draws the crack tip elements in
//                   figures but gives no rule for an area or a volume.
//   CSEAM           a segment from GS to GE when the seam is given by its two grid points: the centre is the middle and
//                   the length the distance between them (the format reference: the length of the seam is the distance
//                   between GS and GE). A seam given by coordinates (the alternate format) has no nodes and no
//                   quantity.
//   CSPR            a segment from G1 to G2: the centre and the length.
//   CSLOT3, CSLOT4  a surface of three or four points, as the acoustic absorber CAABSF is: the centre is the mean of
//                   the points, the normal follows the right-hand rule on the order of the points and the area is that
//                   of a shell with no thickness (no thickness, no volume, no element system). The points are GRIDS
//                   slot points: a GRIDS has no position of its own in the basic system, so the element lies in the
//                   plane of the slot cavity, x being the R of a point and y its Z (NodeLookup::slotPoint; the slot
//                   width W is not a coordinate). A point that is a grid point is used as it is, one that is another
//                   point (a scalar point) has no position and the quantities are NaN, and a point that no card defines
//                   at all makes the geometry of the whole card type fail, as it does for every element.

namespace oor::analysis::detail
{

namespace
{

using base::Vec3;

// The segment between two points, either of which may be missing (a blank field or a point that is not a grid point):
// the centre is the middle, or the one point there is, and the length is the distance when both are there.
void evaluateSegment(const Evaluation& evaluation, const Vec3& first, const Vec3& second, ElementValues& values)
{
    const bool hasFirst = isFinite(first);
    const bool hasSecond = isFinite(second);
    if (evaluation.wants(Quantity::Center))
    {
        if (hasFirst && hasSecond)
            values.center = 0.5 * (first + second);
        else if (hasFirst)
            values.center = first;
        else if (hasSecond)
            values.center = second;
    }
    if (hasFirst && hasSecond && evaluation.wants(Quantity::Length))
        values.length = base::norm(second - first);
}

struct CrackTip2DShape
{
    static void nodes(const model::CrackTip2DTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::CrackTip2DCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    // No area, volume or centre rule in the format reference: every quantity stays NaN.
    static void evaluate(const model::CrackTip2DTable&, const Evaluation&, ElementValues&) {}
};

struct CrackTip3DShape
{
    static void nodes(const model::CrackTip3DTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::CrackTip3DCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    // No area, volume or centre rule in the format reference: every quantity stays NaN.
    static void evaluate(const model::CrackTip3DTable&, const Evaluation&, ElementValues&) {}
};

struct SeamShape
{
    // GS and GE; both are 0 for a seam given by coordinates.
    static void nodes(const model::SeamTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::SeamCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::SeamTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct SpringOffsetExplicitShape
{
    static void nodes(const model::SpringOffsetExplicitTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        const model::SpringOffsetExplicitCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::SpringOffsetExplicitTable&, const Evaluation& evaluation,
                         ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct AcousticSlot3Shape
{
    static void nodes(const model::AcousticSlot3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::AcousticSlot3Card card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::AcousticSlot3Table&, const Evaluation& evaluation, ElementValues& values)
    {
        ShellInput input;
        input.hasThickness = false;
        input.hasFrame = false;
        input.corners = evaluation.points.first(3);
        evaluateTriangle(evaluation, input, values);
    }
};

struct AcousticSlot4Shape
{
    static void nodes(const model::AcousticSlot4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::AcousticSlot4Card card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::AcousticSlot4Table&, const Evaluation& evaluation, ElementValues& values)
    {
        ShellInput input;
        input.hasThickness = false;
        input.hasFrame = false;
        input.corners = evaluation.points.first(4);
        evaluateQuadrilateral(evaluation, input, values);
    }
};

} // namespace

void registerCrackSeamSlotProviders(ProviderRegistry& registry)
{
    registry.add<model::CrackTip2DTable, CrackTip2DShape>();
    registry.add<model::CrackTip3DTable, CrackTip3DShape>();
    registry.add<model::SeamTable, SeamShape>();
    registry.add<model::SpringOffsetExplicitTable, SpringOffsetExplicitShape>();
    registry.add<model::AcousticSlot3Table, AcousticSlot3Shape>();
    registry.add<model::AcousticSlot4Table, AcousticSlot4Shape>();
}

} // namespace oor::analysis::detail
