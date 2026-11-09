#include "ElementProvider.hpp"
#include "oor/model/cards/elements/damperDofPropertyExplicit/DamperDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/damperDofInlineExplicit/DamperDofInlineExplicitTable.hpp"
#include "oor/model/cards/elements/springDofPropertyExplicit/SpringDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/springDofInlineExplicit/SpringDofInlineExplicitTable.hpp"

// The scalar springs and dampers of SOL 700: CELAS1D, CELAS2D, CDAMP1D and CDAMP2D.
//
// They are the CELAS1, CELAS2, CDAMP1 and CDAMP2 with a coordinate system and a FOLLOW word, and have the same
// geometry: the centre is the middle of the segment between the two end points (the point itself when only one end is a
// grid point), and the length is the distance between the two grid points. An end that is a scalar point or is blank
// (ground) has no position, so a spring between scalar points, or between a scalar point and ground, has no centre and
// no length (NaN); a scalar point gives nothing. The format reference gives no area, volume, normal or frame for them,
// and the coordinate system of CORD is the one the degree of freedom is defined in, not an element system: those
// quantities stay NaN, and the element is in the node-to-element index through its two points.

namespace oor::analysis::detail
{

namespace
{

using base::Vec3;

// The segment between two points, either of which may be missing (a scalar point, or a blank field).
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

struct SpringDofPropertyExplicitShape
{
    static void nodes(const model::SpringDofPropertyExplicitTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::SpringDofPropertyExplicitCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::SpringDofPropertyExplicitTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct SpringDofInlineExplicitShape
{
    static void nodes(const model::SpringDofInlineExplicitTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::SpringDofInlineExplicitCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::SpringDofInlineExplicitTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct DamperDofPropertyExplicitShape
{
    static void nodes(const model::DamperDofPropertyExplicitTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::DamperDofPropertyExplicitCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::DamperDofPropertyExplicitTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct DamperDofInlineExplicitShape
{
    static void nodes(const model::DamperDofInlineExplicitTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::DamperDofInlineExplicitCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::DamperDofInlineExplicitTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

} // namespace

void registerSol700ScalarProviders(ProviderRegistry& registry)
{
    registry.add<model::SpringDofPropertyExplicitTable, SpringDofPropertyExplicitShape>();
    registry.add<model::SpringDofInlineExplicitTable, SpringDofInlineExplicitShape>();
    registry.add<model::DamperDofPropertyExplicitTable, DamperDofPropertyExplicitShape>();
    registry.add<model::DamperDofInlineExplicitTable, DamperDofInlineExplicitShape>();
}

} // namespace oor::analysis::detail
