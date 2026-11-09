#include "PlyData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"

namespace oor::query
{

namespace
{

// compositePlyTable: the plies of the laminates, with their stresses, the failure theory and the failure index.
class CompositePlyTableQuery final : public ResultQuery
{
public:
    CompositePlyTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const ElementRequest request = ElementRequest::from(context.arguments());
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        PlyData data = buildPlyData(context, request);
        if (data.flats.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts("the plies of the laminates", data.attempts) +
                                               "\nand the result file has no stress table of laminates either");
            return emptyTable(info().defaultColumns());
        }
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : data.providers)
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        std::int64_t plyRows = 0;
        for (ElementFlat& flat : data.flats)
        {
            plyRows += static_cast<std::int64_t>(flat.rows());
            accumulator.add(plyBlock(context, std::move(flat), providers));
        }
        // the stresses of the plies are always those of the table of the file
        accumulator.note(accumulator.addProvider("ply stress table of the result file", false),
                         {"o11", "o22", "t12", "t1z", "t2z", "omax", "omin", "maxShear"}, plyRows);
        std::vector<std::string> keys = leadingKeys();
        keys.insert(keys.end(), {"elementId", "ply"});
        return sortedResult(accumulator.finish(context, info()), context.exec(), keys);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "compositePlyTable";
        info.envelopeValue = "failureIndex";
        info.family = "R2";
        info.resultQuery = true;
        info.description =
            "The plies of the laminates, one row for each subcase, step, element and ply: its stresses in the ply "
            "system "
            "(`o11 o22 t12 t1z t2z`, the principal values `omax omin` and `maxShear` as the table gives them), the "
            "failure "
            "theory and the failure index, and the strength ratio. The failure index comes from the failure index "
            "table "
            "of the result file, else is recomputed from the ply stresses and the MAT8 allowables with the failure "
            "theory of the "
            "property (`table.sources` and `withSource` say which); the ply number, its global ID (PCOMPG), the "
            "material, the "
            "angle and the thickness come from the model when the results are attached to one. The strength ratio is "
            "computed with the theory of the row (the factor the stresses can be multiplied by before the index "
            "reaches 1) and "
            "is null without the model. The bonding index (`bondIndex`, the PCOMP SB) is the solver's failure index "
            "for "
            "bonding when the failure index table holds it, else the larger of the interlaminar shears `t1z`, `t2z` "
            "over SB (null without SB or without the model). With a theory of failure modes (`theory` \"hashin\", "
            "\"puck\" or \"larc03\" of the margins and the kernels, or a PCOMP whose FT is HFAIL: Hashin), "
            "`fiberIndex` and `matrixIndex` are the indices of the fibre and of the matrix modes and `failureMode` "
            "the one that governs; null for the other theories.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
                           parameter("propertyIds", ParameterKind::IdList, "None", "the properties (needs the model)"),
                           parameter("plies", ParameterKind::IdList, "None", "the ply numbers to keep")};
        info.defaultSort = "subcase, [step], elementId, ply";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .int32("propId")
            .int32("ply")
            .int32("plyId")
            .int32("matId")
            .real("theta")
            .real("thickness");
        columns.results("o11 o22 t12 t1z t2z omax omin maxShear")
            .dictionary("failureTheory")
            .results("failureIndex strengthRatio bondIndex fiberIndex matrixIndex")
            .dictionary("failureMode");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCompositePlyTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CompositePlyTableQuery>());
}

} // namespace oor::query
