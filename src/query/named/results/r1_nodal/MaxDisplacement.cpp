#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r1_nodal/NodalResultQueries.hpp"

#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

// maxDisplacement: for each subcase and step, the grid point that moves most (by norm of the translations, or by the
// absolute value of one component), with its translations and its position.
class MaxDisplacementQuery final : public ResultQuery
{
public:
    MaxDisplacementQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::string by = context.arguments().textOr("by", "magnitude");
        static const std::set<std::string> COMPONENTS = {"t1", "t2", "t3", "r1", "r2", "r3"};
        if (by != "magnitude" && !COMPONENTS.contains(by))
            throw QueryError("by is \"magnitude\" or a component (t1 t2 t3 r1 r2 r3), not '" + by + "'");
        const auto nodes = context.arguments().ids("nodeIds");
        std::set<std::int64_t> wanted;
        if (nodes)
            wanted.insert(nodes->begin(), nodes->end());
        std::vector<TableRef> tables = context.tables(results::ResultKind::Displacements);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        ResultNodePositions positions(context);
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const int t1 = values.component("t1");
            const int component = by == "magnitude" ? t1 : values.component(by);
            if (t1 < 0 || component < 0)
                continue;
            std::vector<std::size_t> steps;
            std::vector<std::int32_t> subcase;
            std::vector<std::int32_t> nid;
            std::array<std::vector<double>, 3> real;
            std::array<std::vector<double>, 3> imaginary;
            std::vector<double> value;
            std::array<std::vector<double>, 3> position;
            for (std::size_t step = 0; step < values.steps(); ++step)
            {
                double best = -1.0;
                std::size_t bestRow = 0;
                bool found = false;
                for (std::size_t row = 0; row < values.rows(); ++row)
                {
                    if (nodes && !wanted.contains(decoded->ids[row]))
                        continue;
                    double score = 0.0;
                    if (by == "magnitude")
                    {
                        for (int offset = 0; offset < 3; ++offset)
                        {
                            const double re = values.real(step, row, static_cast<std::size_t>(t1 + offset));
                            const double im = values.imag(step, row, static_cast<std::size_t>(t1 + offset));
                            score += re * re + im * im;
                        }
                        score = std::sqrt(score);
                    }
                    else
                    {
                        score = values.magnitude(step, row, static_cast<std::size_t>(component));
                    }
                    if (!found || score > best)
                    {
                        best = score;
                        bestRow = row;
                        found = true;
                    }
                }
                if (!found)
                    continue;
                steps.push_back(step);
                subcase.push_back(meta.subcase);
                nid.push_back(decoded->ids[bestRow]);
                // `value` is the norm, or the component (its magnitude for a complex result)
                value.push_back(by == "magnitude"
                                    ? best
                                    : (values.isComplex()
                                           ? best
                                           : values.real(step, bestRow, static_cast<std::size_t>(component))));
                for (int offset = 0; offset < 3; ++offset)
                {
                    real[static_cast<std::size_t>(offset)].push_back(
                        values.real(step, bestRow, static_cast<std::size_t>(t1 + offset)));
                    imaginary[static_cast<std::size_t>(offset)].push_back(
                        values.imag(step, bestRow, static_cast<std::size_t>(t1 + offset)));
                }
                const std::array<double, 3> point = positions.position(ref.set, decoded->ids[bestRow]);
                for (std::size_t axis = 0; axis < 3; ++axis)
                    position[axis].push_back(point[axis]);
            }
            if (steps.empty())
                continue;
            RowBlock block(steps.size());
            block.ints("subcase", std::move(subcase));
            block.ints("nodeId", std::move(nid));
            block.reals("value", std::move(value));
            static const char* const NAMES[3] = {"t1", "t2", "t3"};
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                if (values.isComplex())
                    block.complex(NAMES[axis], std::move(real[axis]), std::move(imaginary[axis]));
                else
                    block.reals(NAMES[axis], std::move(real[axis]));
            }
            block.reals("x", std::move(position[0]));
            block.reals("y", std::move(position[1]));
            block.reals("z", std::move(position[2]));
            addStepsToBlock(block, meta, *decoded, steps);
            block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
            block.precision(precisionOf(*decoded));
            block.storedAsMagnitudePhase(meta.storedFormat == results::ResultFormat::MagnitudePhase);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "maxDisplacement";
        info.family = "R1";
        info.resultQuery = true;
        info.envelopeValue = "value";
        info.envelopeKeys = {"subcase", "designCycle"};
        info.description =
            "The grid point that moves most in each subcase and step: by the norm of its translations "
            "(`by=\"magnitude\"`) "
            "or by the absolute value of one component (`by=\"t3\"`). `value` is that norm or that component (signed; "
            "the magnitude for a complex result), then the three translations of the point and, when the results are "
            "attached to a model, its position in the basic system.";
        info.parameters = {parameter("by", ParameterKind::Text, "\"magnitude\"", "\"magnitude\" or a component name"),
                           parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to look at")};
        info.defaultSort = "subcase, [step]";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("nodeId").result("value").results("t1 t2 t3").reals("x y z");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMaxDisplacementQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MaxDisplacementQuery>());
}

} // namespace oor::query
