#include "oor/analysis/design/DesignShapeChange.hpp"

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>

namespace oor::analysis
{

namespace
{

// A coordinate that the move leaves where it was, to rounding, is not written.
bool sameToRounding(double left, double right)
{
    const double scale = std::max(std::fabs(left), std::fabs(right));
    return std::fabs(left - right) <= 4.0 * std::numeric_limits<double>::epsilon() * scale;
}

std::string describe(const model::DesignShapeTerm& term)
{
    return std::string("DesignNodeMove") + " of the design variable " + std::to_string(term.variable) +
           " on grid point " + std::to_string(term.grid);
}

// The coordinate systems whose axes depend on the grid points that move: a CORD1x that names one, and the systems
// defined in those (a CORD2x whose RID is one, a CORD1x whose grid points are located in one), to the end of the
// chains.
std::set<base::EntityId> systemsMovedBy(const SystemDefinitions& systems, const model::NodeTable& grids,
                                        const model::Model& model, const std::set<base::EntityId>& movedGrids)
{
    const auto cpOf = [&](base::EntityId grid) -> base::EntityId
    {
        const std::optional<model::CardLocation> location = model.locate(base::IdSpace::Node, grid);
        if (!location || location->table != model::typeIndexOf<model::NodeTable>())
            return 0;
        return grids.positionFrameId(location->row);
    };
    std::set<base::EntityId> moved;
    bool grown = true;
    while (grown)
    {
        grown = false;
        for (const auto& [cid, definition] : systems)
        {
            if (moved.contains(cid))
                continue;
            bool depends = false;
            if (definition.form == SystemDefinition::Form::ThreeGrids)
            {
                for (const base::EntityId node : definition.nodes)
                    depends = depends || movedGrids.contains(node) || moved.contains(cpOf(node));
            }
            else
            {
                depends = moved.contains(definition.referenceFrameId);
            }
            if (depends)
            {
                moved.insert(cid);
                grown = true;
            }
        }
    }
    return moved;
}

} // namespace

model::DesignShapeUpdate DesignShapeChange::compute(const model::Model& model, const model::DesignModel& design,
                                                    const std::vector<double>& variables)
{
    model::DesignShapeUpdate result;
    const std::vector<model::DesignShapeTerm>& terms = design.shapeTerms();
    if (terms.empty())
        return result;
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    CoordinateTransform transform(model);
    const BasicPositionCache* positions = grids != nullptr ? &BasicPositionCache::of(model) : nullptr;

    const auto skip = [&](std::size_t index, std::string reason)
    {
        if (std::find(result.skipped.begin(), result.skipped.end(), index) != result.skipped.end())
            return;
        result.skipped.push_back(index);
        result.problems.push_back(describe(terms[index]) + ": " + std::move(reason));
    };

    // The move of each grid point in the basic system, by row of the GRID table.
    std::map<std::uint32_t, base::Vec3> displacements;
    for (std::size_t index = 0; index < terms.size(); ++index)
    {
        const model::DesignShapeTerm& term = terms[index];
        const std::optional<std::size_t> variable = design.findVariable(term.variable);
        if (!variable)
        {
            skip(index, "no " + std::string("DesignVar") + " defines the design variable");
            continue;
        }
        const std::optional<model::CardLocation> location = model.locate(base::IdSpace::Node, term.grid);
        if (!location)
        {
            skip(index, "the grid point does not exist");
            continue;
        }
        if (grids == nullptr || location->table != model::typeIndexOf<model::NodeTable>())
        {
            skip(index,
                 "the point is not a " + std::string("Node") + " (a " + std::string("GeomPoint") + " is not moved)");
            continue;
        }
        if (!transform.hasSystem(term.system))
        {
            skip(index, "the coordinate system " + std::to_string(term.system) + " does not exist");
            continue;
        }
        const double change = term.coefficient * (variables[*variable] - design.variables()[*variable].initial);
        try
        {
            const base::Vec3 here = positions->position(location->row);
            const base::Vec3 vector{term.vector[0], term.vector[1], term.vector[2]};
            const base::Vec3 basic = transform.displacementRotation(term.system, here) * vector;
            displacements[location->row] += basic * change;
        }
        catch (const CoordinateSystemError& error)
        {
            skip(index, error.what());
        }
    }

    std::set<base::EntityId> movedGrids;
    for (const auto& [row, displacement] : displacements)
    {
        if (displacement != base::Vec3{})
            movedGrids.insert(grids->idAtRow(row));
    }
    const std::set<base::EntityId> movedSystems =
        movedGrids.empty() ? std::set<base::EntityId>{}
                           : systemsMovedBy(transform.definitions(), *grids, model, movedGrids);
    for (const auto& [row, displacement] : displacements)
    {
        if (displacement == base::Vec3{})
            continue;
        const base::EntityId cp = grids->positionFrameId(row);
        if (movedSystems.contains(cp))
        {
            for (std::size_t index = 0; index < terms.size(); ++index)
            {
                if (terms[index].grid == grids->idAtRow(row))
                    skip(index, "the grid point is located in the system " + std::to_string(cp) +
                                    ", which grid points that move define");
            }
            continue;
        }
        model::DesignGridMove move;
        move.grid = grids->idAtRow(row);
        move.row = row;
        move.before = grids->xyz(row);
        move.displacement = displacement;
        try
        {
            move.after = transform.fromBasic(cp, positions->position(row) + displacement);
        }
        catch (const CoordinateSystemError& error)
        {
            for (std::size_t index = 0; index < terms.size(); ++index)
            {
                if (terms[index].grid == move.grid)
                    skip(index, error.what());
            }
            continue;
        }
        if (sameToRounding(move.before.x, move.after.x) && sameToRounding(move.before.y, move.after.y) &&
            sameToRounding(move.before.z, move.after.z))
            continue;
        result.moves.push_back(move);
    }
    // The variables of the cards applied: their XINIT becomes the value of the point.
    std::set<std::size_t> movingVariables;
    for (std::size_t index = 0; index < terms.size(); ++index)
    {
        if (std::find(result.skipped.begin(), result.skipped.end(), index) == result.skipped.end())
            movingVariables.insert(*design.findVariable(terms[index].variable));
    }
    result.variables.assign(movingVariables.begin(), movingVariables.end());
    return result;
}

} // namespace oor::analysis
