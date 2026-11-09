#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The vector of a DVGRID in the basic system: N of the card along the directions of its system CID at the grid point
// (the axes of a rectangular system, R, theta, Z of a cylindrical one at the point). Null when the grid point or the
// system cannot be found.
std::optional<base::Vec3> basicDirection(const model::Model& model, analysis::CoordinateTransform& transform,
                                         const analysis::BasicPositionCache* positions,
                                         const model::DesignShapeTerm& term)
{
    const std::optional<model::CardLocation> location = model.locate(base::IdSpace::Node, term.grid);
    if (!location || positions == nullptr || location->table != model::typeIndexOf<model::NodeTable>() ||
        !transform.hasSystem(term.system))
        return std::nullopt;
    try
    {
        const base::Vec3 vector{term.vector[0], term.vector[1], term.vector[2]};
        return transform.displacementRotation(term.system, positions->position(location->row)) * vector;
    }
    catch (const analysis::CoordinateSystemError&)
    {
        return std::nullopt;
    }
}

// shapeVariablesTable: the DVGRID cards, the vector of each in the basic system.
class ShapeVariablesTableQuery final : public QueryBase
{
public:
    ShapeVariablesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::DesignModel design = model::DesignModel::read(model);
        const std::vector<model::DesignShapeTerm>& terms = design.shapeTerms();
        if (terms.empty())
            return emptyTable(info().defaultColumns());
        std::vector<std::size_t> order(terms.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::stable_sort(order.begin(), order.end(),
                         [&](std::size_t left, std::size_t right)
                         {
                             return std::tie(terms[left].variable, terms[left].grid) <
                                    std::tie(terms[right].variable, terms[right].grid);
                         });
        analysis::CoordinateTransform transform(model);
        const analysis::BasicPositionCache* positions =
            model.findTable<model::NodeTable>() != nullptr ? &analysis::BasicPositionCache::of(model) : nullptr;
        const std::size_t count = terms.size();
        std::vector<std::int32_t> variable(count), grid(count), system(count);
        std::vector<double> coefficient(count), x(count, NOT_A_NUMBER), y(count, NOT_A_NUMBER), z(count, NOT_A_NUMBER);
        for (std::size_t row = 0; row < count; ++row)
        {
            const model::DesignShapeTerm& term = terms[order[row]];
            variable[row] = term.variable;
            grid[row] = term.grid;
            system[row] = term.system;
            coefficient[row] = term.coefficient;
            if (const std::optional<base::Vec3> direction = basicDirection(model, transform, positions, term))
            {
                x[row] = direction->x;
                y[row] = direction->y;
                z[row] = direction->z;
            }
        }
        TableBuilder builder;
        builder.int32("designVariable", std::move(variable));
        builder.int32("nodeId", std::move(grid));
        builder.int32("frameId", std::move(system));
        builder.real("coefficient", std::move(coefficient));
        builder.real("directionX", std::move(x));
        builder.real("directionY", std::move(y));
        builder.real("directionZ", std::move(z));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "shapeVariablesTable";
        info.family = "T9";
        info.description =
            "The shape design variables (DVGRID cards), one row per card: the design variable, the grid point it "
            "moves, the system CID of the vector, COEFF, and the vector (N1, N2, N3) in the basic system, taken along "
            "the directions of CID at the grid point and not made a unit vector (the "
            "grid point "
            "moves by COEFF * (X - XINIT) * N; the cards on one grid point add). The vector is null when the grid "
            "point or the system does not exist. `model.design.apply` moves the grid points.";
        info.defaultSort = "designVariable, nodeId";
        ColumnListBuilder columns;
        columns.int32("designVariable").int32("nodeId").int32("frameId").real("coefficient");
        columns.reals("directionX directionY directionZ");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addShapeVariablesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ShapeVariablesTableQuery>());
}

} // namespace oor::query
