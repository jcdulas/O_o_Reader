#include "oor/analysis/coordinates/ModelBounds.hpp"
#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r7_checks/CheckQueries.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// massCheck: the mass and the centre of gravity of the model against those of the grid point weight table of the
// result file.
class MassCheckQuery final : public ResultQuery
{
public:
    MassCheckQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const double tolerance = context.arguments().realOr("tolerance", 1.0e-3);
        if (!(tolerance >= 0.0))
            throw QueryError("tolerance is a relative difference, not negative");
        const model::Model& model = *context.modelPointer();
        // the mass of the model as the grid point weight generator computes it: the non-structural mass of the Case
        // Control set, in the units of the deck. The table of the file is not multiplied by PARAM,WTMASS (it holds the
        // weight of the deck's units: with a WTMASS other than 1 it is the model mass divided by WTMASS).
        const MassSetting setting(model, context.exec().pool());
        const analysis::MassOptions options = setting.options();
        const analysis::MassTotals totals = analysis::MassCalculator::total(
            model, analysis::MassCalculator::allElements(model), options, context.exec().pool());
        const double modelMass = totals.sums.mass();
        const base::Vec3 modelCentre = totals.sums.centerOfGravity();
        std::vector<ResultQueryContext::NamedRef> weights = context.named("nodePointWeight");
        if (weights.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(
                    "no way of obtaining the mass of the result file gave it: the result file has no grid "
                    "point weight table (PARAM,GRDPNT was not requested)");
            return emptyTable(info().defaultColumns());
        }
        double size = 0.0;
        if (const auto box = analysis::boundingBox(model, context.exec().pool()))
            size = base::norm(box->maximum - box->minimum);
        ResultNodePositions positions(context);
        std::vector<double> mass;
        std::vector<double> relative;
        std::array<std::vector<double>, 3> modelC, resultC;
        std::vector<double> modelMasses;
        std::vector<double> distance;
        std::vector<std::uint8_t> consistent;
        std::vector<std::uint8_t> consistentValid;
        std::set<std::size_t> done;
        bool single = true;
        for (const ResultQueryContext::NamedRef& weight : weights)
        {
            if (!done.insert(weight.set).second)
                continue; // one grid point weight for each result set: the mass does not depend on the subcase
            const std::shared_ptr<const results::NamedResultData> data = weight.result->data(context.decode());
            const results::ResultArray* mo = data->find("mo");
            const results::ResultArray* cg = data->find("cg");
            const results::ResultArray* reference = data->find("referencePoint");
            if (mo == nullptr || cg == nullptr)
                continue;
            const std::vector<double> matrix = arrayDoubles(*mo);
            const std::vector<double> centres = arrayDoubles(*cg);
            const double resultMass = (matrix[0] + matrix[7] + matrix[14]) / 3.0;
            // the centre of gravity is given from the reference point of the table (the origin when it is 0)
            double base[3] = {0.0, 0.0, 0.0};
            if (reference != nullptr)
            {
                const std::vector<double> ids = arrayDoubles(*reference);
                if (!ids.empty() && ids[0] > 0.0)
                {
                    const std::array<double, 3> at = positions.position(weight.set, static_cast<std::int32_t>(ids[0]));
                    for (std::size_t axis = 0; axis < 3; ++axis)
                        base[axis] = std::isnan(at[axis]) ? NOT_A_NUMBER : at[axis];
                }
            }
            // row i of the table is the direction i of the mass axes: the coordinate along x comes from the y row
            const double cx = centres[3] + base[0];
            const double cy = centres[1] + base[1];
            const double cz = centres[2] + base[2];
            mass.push_back(resultMass);
            modelMasses.push_back(modelMass);
            relative.push_back(resultMass != 0.0 ? std::fabs(modelMass - resultMass) / std::fabs(resultMass)
                                                 : NOT_A_NUMBER);
            modelC[0].push_back(modelCentre.x);
            modelC[1].push_back(modelCentre.y);
            modelC[2].push_back(modelCentre.z);
            resultC[0].push_back(cx);
            resultC[1].push_back(cy);
            resultC[2].push_back(cz);
            const double gap =
                std::sqrt((modelCentre.x - cx) * (modelCentre.x - cx) + (modelCentre.y - cy) * (modelCentre.y - cy) +
                          (modelCentre.z - cz) * (modelCentre.z - cz));
            distance.push_back(gap);
            const bool fine = !std::isnan(relative.back()) && relative.back() <= tolerance &&
                              (std::isnan(gap) || gap <= tolerance * std::max(size, 1.0e-30));
            consistent.push_back(fine ? 1 : 0);
            consistentValid.push_back(std::isnan(relative.back()) ? 0 : 1);
            single = single && weight.result->meta().wordSize == 4;
        }
        if (mass.empty())
            return emptyTable(info().defaultColumns());
        RowBlock block(mass.size());
        block.reals("modelMass", std::move(modelMasses));
        block.reals("resultMass", std::move(mass));
        block.reals("relativeDifference", std::move(relative));
        block.reals("modelCx", std::move(modelC[0]));
        block.reals("modelCy", std::move(modelC[1]));
        block.reals("modelCz", std::move(modelC[2]));
        block.reals("resultCx", std::move(resultC[0]));
        block.reals("resultCy", std::move(resultC[1]));
        block.reals("resultCz", std::move(resultC[2]));
        block.reals("cgDistance", std::move(distance));
        block.flags("consistent", std::move(consistent), std::move(consistentValid));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        const std::int16_t modelWay = accumulator.addProvider("model", false);
        const std::int16_t resultWay = accumulator.addProvider("grid point weight table of the result file", false);
        accumulator.note(modelWay, {"modelMass", "modelCx", "modelCy", "modelCz"},
                         static_cast<std::int64_t>(block.rows()));
        accumulator.note(resultWay, {"resultMass", "resultCx", "resultCy", "resultCz"},
                         static_cast<std::int64_t>(block.rows()));
        accumulator.add(std::move(block));
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massCheck";
        info.family = "R7";
        info.resultQuery = true;
        info.needsModel = true;
        info.description =
            "The mass and the centre of gravity of the model (the mass of the elements and the non-structural mass, in "
            "the units "
            "of the deck: the table of the file is not multiplied by PARAM,WTMASS either) against those of the grid "
            "point weight table of the result file (the mean of the three translational "
            "masses of the rigid body mass matrix, and the centre of gravity from the table, measured from its "
            "reference point), "
            "for each result set. `relativeDifference` is the difference of the masses over the mass of the table; "
            "`consistent` "
            "is true when it is within `tolerance` and the centres are closer than `tolerance` times the size of the "
            "model. A "
            "deck that leaves out elements the solver takes (or a mass the library does not compute) shows here.";
        info.parameters = {
            parameter("tolerance", ParameterKind::Real, "0.001", "the relative difference that is accepted")};
        info.defaultSort = "result set";
        ColumnListBuilder columns;
        columns.real("modelMass").result("resultMass").real("relativeDifference").reals("modelCx modelCy modelCz");
        columns.results("resultCx resultCy resultCz").real("cgDistance").flag("consistent");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassCheckQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassCheckQuery>());
}

} // namespace oor::query
