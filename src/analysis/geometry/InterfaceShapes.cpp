#include "ElementProvider.hpp"
#include "ShellGeometry.hpp"
#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Table.hpp"
#include "oor/model/cards/elements/shellAxisym/ShellAxisymTable.hpp"
#include "oor/model/cards/elements/bushing2D/Bushing2DTable.hpp"
#include "oor/model/cards/elements/cohesiveAxisymQuad/CohesiveAxisymQuadTable.hpp"
#include "oor/model/cards/elements/cohesiveHexa/CohesiveHexaTable.hpp"
#include "oor/model/cards/elements/cohesivePenta/CohesivePentaTable.hpp"
#include "oor/model/cards/elements/cohesiveQuad/CohesiveQuadTable.hpp"

#include <cstddef>

// The interface elements: the bases of the acoustic infinite elements, the axisymmetric line, the 2-D bush and the
// cohesive interface elements. Every card type gives its nodes in the order of its card (0 for a blank slot), so each
// is in the node-to-element index.
//
// CACINF3 and CACINF4 are the base of an infinite element, a triangle and a quadrilateral of acoustic grid points, with
// no thickness: like the CAABSF they have the centre, the normal by the right-hand rule on the connection order and the
// area of a shell of no thickness (no frame, no volume).
//
// CAXISYM is a line in the x-y plane. Its centre is the middle of the chord G1-G2 and its length is the chord, or, when
// the middle point G3 is given, the sum of the chords G1-G3 and G3-G2 (G3 lies on the curve: the format draws it
// between the ends). The element has no other geometry here: it is a thick shell of an axisymmetric model, whose area
// and volume depend on the property and on the revolution.
//
// CBUSH2D is the segment GA-GB (the format wants two coincident grids, so its length is normally zero): centre and
// length only. Its frame depends on the displacement systems of the two grids, which the format reference does not
// check.
//
// CIFHEX, CIFPENT, CIFQDX and CIFQUAD are cohesive layers between two faces, possibly infinitely thin (the faces may
// coincide). Only the centre is given, the mean of the corner points (8, 6, 4 and 4: the points G1 to G8, G1 to G6 and
// G1 to G4 of the cards; the optional edge points are not used). The area, volume, normal and length of such an element
// are not defined by the format (they depend on the property and on the opening of the faces) and stay NaN.

namespace oor::analysis::detail
{

namespace
{

template <typename NodeArray>
void appendNodes(const NodeArray& nodes, std::vector<base::EntityId>& ids)
{
    ids.insert(ids.end(), nodes.begin(), nodes.end());
}

// The base of an infinite element: a shell with no thickness and no coordinate system of its own.
ShellInput baseInput(std::span<const base::Vec3> corners)
{
    ShellInput input;
    input.corners = corners;
    input.hasThickness = false;
    input.hasFrame = false;
    return input;
}

// The mean of the first `cornerCount` points of the element, when every one of them is a grid point.
void evaluateCornerCentre(const Evaluation& evaluation, std::size_t cornerCount, ElementValues& values)
{
    if (!evaluation.wants(Quantity::Center))
        return;
    base::Vec3 sum{0.0, 0.0, 0.0};
    for (std::size_t corner = 0; corner < cornerCount; ++corner)
    {
        if (!isFinite(evaluation.points[corner]))
            return;
        sum = sum + evaluation.points[corner];
    }
    values.center = sum / static_cast<double>(cornerCount);
}

struct AcousticInfiniteTria3Shape
{
    static void nodes(const model::AcousticInfiniteTria3Table& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AcousticInfiniteTria3Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateTriangle(evaluation, baseInput(evaluation.points.first(3)), values);
    }
};

struct AcousticInfiniteQuad4Shape
{
    static void nodes(const model::AcousticInfiniteQuad4Table& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AcousticInfiniteQuad4Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateQuadrilateral(evaluation, baseInput(evaluation.points.first(4)), values);
    }
};

struct ShellAxisymShape
{
    static void nodes(const model::ShellAxisymTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellAxisymTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const base::Vec3& first = evaluation.points[0];
        const base::Vec3& second = evaluation.points[1];
        const base::Vec3& middle = evaluation.points[2];
        if (!isFinite(first) || !isFinite(second))
            return;
        if (evaluation.wants(Quantity::Center))
            values.center = 0.5 * (first + second);
        if (!evaluation.wants(Quantity::Length))
            return;
        if (evaluation.nodeIds[2] == 0)
            values.length = base::norm(second - first);
        else if (isFinite(middle)) // a G3 that is not a grid point leaves the length undefined
            values.length = base::norm(middle - first) + base::norm(second - middle);
    }
};

struct Bushing2DShape
{
    static void nodes(const model::Bushing2DTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::Bushing2DTable&, const Evaluation& evaluation, ElementValues& values)
    {
        const base::Vec3& inner = evaluation.points[0];
        const base::Vec3& outer = evaluation.points[1];
        if (!isFinite(inner) || !isFinite(outer))
            return;
        if (evaluation.wants(Quantity::Center))
            values.center = 0.5 * (inner + outer);
        if (evaluation.wants(Quantity::Length))
            values.length = base::norm(outer - inner);
    }
};

struct CohesiveHexaShape
{
    static void nodes(const model::CohesiveHexaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::CohesiveHexaTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateCornerCentre(evaluation, model::CohesiveHexaCard::CORNER_COUNT, values);
    }
};

struct CohesivePentaShape
{
    static void nodes(const model::CohesivePentaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::CohesivePentaTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateCornerCentre(evaluation, model::CohesivePentaCard::CORNER_COUNT, values);
    }
};

struct CohesiveAxisymQuadShape
{
    static void nodes(const model::CohesiveAxisymQuadTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::CohesiveAxisymQuadTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateCornerCentre(evaluation, model::CohesiveAxisymQuadCard::CORNER_COUNT, values);
    }
};

struct CohesiveQuadShape
{
    static void nodes(const model::CohesiveQuadTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::CohesiveQuadTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateCornerCentre(evaluation, model::CohesiveQuadCard::CORNER_COUNT, values);
    }
};

} // namespace

void registerInterfaceProviders(ProviderRegistry& registry)
{
    registry.add<model::AcousticInfiniteTria3Table, AcousticInfiniteTria3Shape>();
    registry.add<model::AcousticInfiniteQuad4Table, AcousticInfiniteQuad4Shape>();
    registry.add<model::ShellAxisymTable, ShellAxisymShape>();
    registry.add<model::Bushing2DTable, Bushing2DShape>();
    registry.add<model::CohesiveHexaTable, CohesiveHexaShape>();
    registry.add<model::CohesivePentaTable, CohesivePentaShape>();
    registry.add<model::CohesiveAxisymQuadTable, CohesiveAxisymQuadShape>();
    registry.add<model::CohesiveQuadTable, CohesiveQuadShape>();
}

} // namespace oor::analysis::detail
