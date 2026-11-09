#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"

#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

// nodePointForcesTable: the grid point force balance, one row for each contribution at each grid point.
class NodePointForcesTableQuery final : public ResultQuery
{
public:
    NodePointForcesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const auto nodes = context.arguments().ids("nodeIds");
        const auto elements = context.arguments().ids("elementIds");
        std::set<std::int64_t> wantedNodes;
        std::set<std::int64_t> wantedElements;
        if (nodes)
            wantedNodes.insert(nodes->begin(), nodes->end());
        if (elements)
            wantedElements.insert(elements->begin(), elements->end());
        std::vector<TableRef> tables = context.tables(results::ResultKind::NodePointForces);
        sortTables(tables);
        RowAccumulator accumulator;
        const std::int16_t provider = accumulator.addProvider("stored table", false);
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const results::ResultText* names = nullptr;
            for (const results::ResultText& text : decoded->texts)
                if (text.name == "elementName")
                    names = &text;
            int component[6];
            const char* const columns[6] = {"f1", "f2", "f3", "m1", "m2", "m3"};
            for (int number = 0; number < 6; ++number)
                component[number] = values.component(columns[number]);
            std::vector<std::size_t> kept;
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                if (nodes && !wantedNodes.contains(decoded->ids[row]))
                    continue;
                if (elements &&
                    (row >= decoded->elementIds.size() || !wantedElements.contains(decoded->elementIds[row])))
                    continue;
                kept.push_back(row);
            }
            const std::size_t count = values.steps() * kept.size();
            if (count == 0)
                continue;
            RowBlock block(count);
            std::vector<std::int32_t> nid;
            std::vector<std::int32_t> eid;
            std::vector<std::uint8_t> eidValid;
            std::vector<std::string> contribution;
            std::vector<std::size_t> stepOf;
            std::array<std::vector<double>, 6> real;
            std::array<std::vector<double>, 6> imaginary;
            for (std::size_t step = 0; step < values.steps(); ++step)
                for (const std::size_t row : kept)
                {
                    nid.push_back(decoded->ids[row]);
                    const std::int32_t element = row < decoded->elementIds.size() ? decoded->elementIds[row] : 0;
                    eid.push_back(element);
                    // a multiPointConstrForce row split by relation gives its relation (minus the set ID of an MPC
                    // equation)
                    eidValid.push_back(element != 0 ? 1 : 0);
                    contribution.push_back(names != nullptr ? names->at(row) : std::string());
                    stepOf.push_back(step);
                    for (std::size_t number = 0; number < 6; ++number)
                    {
                        real[number].push_back(
                            component[number] < 0
                                ? std::nan("")
                                : values.real(step, row, static_cast<std::size_t>(component[number])));
                        if (values.isComplex())
                            imaginary[number].push_back(
                                component[number] < 0
                                    ? std::nan("")
                                    : values.imag(step, row, static_cast<std::size_t>(component[number])));
                    }
                }
            block.ints("subcase", std::vector<std::int32_t>(count, meta.subcase));
            block.ints("nodeId", std::move(nid));
            block.ints("elementId", std::move(eid), std::move(eidValid));
            block.texts("contribution", std::move(contribution));
            static const char* const NAMES6[6] = {"t1", "t2", "t3", "r1", "r2", "r3"};
            for (std::size_t number = 0; number < 6; ++number)
            {
                if (values.isComplex())
                    block.complex(NAMES6[number], std::move(real[number]), std::move(imaginary[number]));
                else
                    block.reals(NAMES6[number], std::move(real[number]));
            }
            block.texts("frame", std::vector<std::string>(count, context.frameNameOf(ref)));
            addStepsToBlock(block, meta, *decoded, stepOf);
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
        info.name = "nodePointForcesTable";
        info.family = "R4";
        info.resultQuery = true;
        info.description =
            "The grid point force balance: for each subcase, step and grid point, the force and moment that each "
            "element pushes on the point (`contribution` is the element type as the table names it, `elementId` the "
            "element), then the applied load (`appliedLoad`), the constraint forces (`singlePointConstrForce`, "
            "`multiPointConstrForce`) and the sum (`total`; `elementId` is null for the rows that are not an "
            "element), with the frame they are written in. A table whose multiPointConstrForce rows are split by "
            "relation (a row per relation and grid point, as a solver may write them) gives the relation in "
            "`elementId`: the element ID of a rigid element, minus the set ID of an MPC equation.";
        info.parameters = {parameter("nodeIds", ParameterKind::IdList, "None", "the grid points to list"),
                           parameter("elementIds", ParameterKind::IdList, "None", "the elements whose rows to list")};
        info.defaultSort = "subcase, [step], nodeId";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("nodeId").int32("elementId").dictionary("contribution");
        columns.results("t1 t2 t3 r1 r2 r3").dictionary("frame");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNodePointForcesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NodePointForcesTableQuery>());
}

} // namespace oor::query
