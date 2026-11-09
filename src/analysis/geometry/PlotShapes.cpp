#include "ElementProvider.hpp"
#include "ShellGeometry.hpp"
#include "oor/model/cards/elements/plotQuad4/PlotQuad4Table.hpp"
#include "oor/model/cards/elements/plotSurface/PlotSurfaceTable.hpp"
#include "oor/model/cards/elements/plotTria3/PlotTria3Table.hpp"

// The dummy plot elements with more than two nodes, PLOTEL3 and PLOTEL4 (the two-noded PLOTEL is a segment: see
// LineShapes.cpp), and the surface plot mesh PLTSURF.
//
// All are surfaces that are drawn and nothing else, so they have what a shell has with no thickness and no element
// system, as the acoustic absorber CAABSF does: the centre is the mean of the corners, the normal follows the
// right-hand rule on the order of the nodes, and the area is that of the triangle or of the quadrilateral. No thickness
// (they have no property), no volume and no frame. A PLTSURF has 3, 4, 6 or 8 nodes in the order of the CTRIA3, CQUAD4,
// CTRIA6 and CQUAD8: the corners are the first three or four, the mid-side nodes after them are not used for the planar
// quantities (as for the shells).

namespace oor::analysis::detail
{

namespace
{

struct PlotTria3Shape
{
    static void nodes(const model::PlotTria3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::PlotTria3Card card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
        ids.push_back(card.node3);
    }
    static void evaluate(const model::PlotTria3Table&, const Evaluation& evaluation, ElementValues& values)
    {
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.hasThickness = false;
        input.hasFrame = false;
        evaluateTriangle(evaluation, input, values);
    }
};

struct PlotQuad4Shape
{
    static void nodes(const model::PlotQuad4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::PlotQuad4Card card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
        ids.push_back(card.node3);
        ids.push_back(card.node4);
    }
    static void evaluate(const model::PlotQuad4Table&, const Evaluation& evaluation, ElementValues& values)
    {
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.hasThickness = false;
        input.hasFrame = false;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct PlotSurfaceShape
{
    static void nodes(const model::PlotSurfaceTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::PlotSurfaceCard card = table.card(row);
        ids.insert(ids.end(), card.grids.begin(), card.grids.begin() + static_cast<std::ptrdiff_t>(card.gridCount()));
    }
    static void evaluate(const model::PlotSurfaceTable&, const Evaluation& evaluation, ElementValues& values)
    {
        // Six and eight nodes are the triangle and the quadrilateral with a node in the middle of each side.
        const std::size_t count = evaluation.nodeIds.size();
        ShellInput input;
        input.hasThickness = false;
        input.hasFrame = false;
        if (count == 3 || count == 6)
        {
            input.corners = evaluation.points.first(3);
            evaluateTriangle(evaluation, input, values);
        }
        else if (count == 4 || count == 8)
        {
            input.corners = evaluation.points.first(4);
            evaluateQuadrilateral(evaluation, input, values);
        }
    }
};

} // namespace

void registerPlotProviders(ProviderRegistry& registry)
{
    registry.add<model::PlotTria3Table, PlotTria3Shape>();
    registry.add<model::PlotQuad4Table, PlotQuad4Shape>();
    registry.add<model::PlotSurfaceTable, PlotSurfaceShape>();
}

} // namespace oor::analysis::detail
