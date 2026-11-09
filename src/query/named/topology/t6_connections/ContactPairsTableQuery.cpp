#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace oor::query
{

namespace
{

// The parameters of a pair as columns, in the order of ContactPairParameters: the integers, then the reals.
constexpr std::array<std::string_view, 7> PARAMETER_NAMES{"glueOption",   "glueSeparation", "coordinateCheck",
                                                          "searchOption", "errorTolerance", "toleranceBias",
                                                          "nearDistance"};
constexpr std::size_t INTEGERS = 4;
constexpr std::size_t REALS = PARAMETER_NAMES.size() - INTEGERS;

// contactPairsTable: the pairs of contact bodies and whether their surfaces meet.
class ContactPairsTableQuery final : public QueryBase
{
public:
    ContactPairsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const analysis::ContactOptions options = contactOptionsOf(run.arguments, "contactPairsTable");
        const analysis::ContactRule rule = contactRuleOf(run.arguments, "contactPairsTable");
        const auto search = analysis::ContactSearch::of(*run.model, options.tolerance, rule, run.context.pool());
        if (search->isEmpty())
            return emptyTable(info().defaultColumns());
        std::vector<std::string> table;
        std::vector<std::uint8_t> tableValid;
        std::vector<std::int32_t> secondary;
        std::vector<std::int32_t> primary;
        std::vector<std::string> kind;
        std::vector<std::int64_t> count;
        std::vector<double> minimum;
        std::vector<double> maximum;
        std::vector<double> tolerance;
        std::vector<std::uint8_t> meets;
        // the parameters as the solver resolves them, each with its source
        std::array<std::vector<std::int32_t>, INTEGERS> integers;
        std::array<std::vector<double>, REALS> reals;
        std::array<std::vector<std::string>, INTEGERS + REALS> sources;
        for (std::size_t index = 0; index < search->pairs().size(); ++index)
        {
            const analysis::ContactPair& pair = search->pairs()[index];
            const analysis::ContactPairResult& result = search->results()[index];
            table.push_back(pair.tableLabel());
            tableValid.push_back(pair.table ? 1 : 0);
            secondary.push_back(pair.secondary->id);
            primary.push_back(pair.primary->id);
            kind.emplace_back(analysis::contactKindName(pair.kind));
            count.push_back(static_cast<std::int64_t>(result.meetings.size()));
            minimum.push_back(result.minDistance); // NaN (null) when nothing meets
            maximum.push_back(result.maxDistance);
            tolerance.push_back(result.tolerance);
            meets.push_back(result.meets ? 1 : 0);
            const analysis::ContactPairParameters& parameters = pair.parameters;
            const std::array<const analysis::ResolvedContactParameter*, INTEGERS + REALS> resolved{
                &parameters.glueOption,   &parameters.glueSeparation, &parameters.coordinateCheck,
                &parameters.searchOption, &parameters.errorTolerance, &parameters.toleranceBias,
                &parameters.nearDistance};
            for (std::size_t item = 0; item < resolved.size(); ++item)
            {
                const std::optional<double>& value = resolved[item]->value;
                if (item < INTEGERS)
                    integers[item].push_back(static_cast<std::int32_t>(value.value_or(0.0)));
                else
                    reals[item - INTEGERS].push_back(value.value_or(std::nan("")));
                sources[item].push_back(resolved[item]->source);
            }
        }
        TableBuilder builder;
        builder.text("table", table, std::move(tableValid));
        builder.int32("secondaryBody", std::move(secondary));
        builder.int32("primaryBody", std::move(primary));
        builder.dictionary("kind", kind);
        builder.int64("nodePairCount", std::move(count));
        builder.real("minDistance", std::move(minimum));
        builder.real("maxDistance", std::move(maximum));
        builder.real("tolerance", std::move(tolerance));
        builder.flag("meets", meets);
        for (std::size_t item = 0; item < INTEGERS + REALS; ++item)
        {
            const std::string name(PARAMETER_NAMES[item]);
            if (item < INTEGERS)
                builder.int32(name, std::move(integers[item]));
            else
                builder.real(name, std::move(reals[item - INTEGERS]));
            builder.dictionary(name + "Source", sources[item]);
        }
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "contactPairsTable";
        info.family = "T6";
        info.description =
            "Contact between bodies: one row per pair of contact bodies, those of every contact "
            "table of the Bulk Data (BCTABLE, BCONECT, BCTSET, BCONP: table is the card and its ID; a BCTABL1 or a "
            "BCTADD only lists them), whatever the Case Control selects, or, when the Bulk Data has no table, every "
            "two deformable bodies whose surfaces meet (table null). A pair with a rigid body (BEHAV RIGID, SYMM or "
            "HEAT, a rigid BCSURF) or a body that does not exist is left out. kind is glued (IGLUE not 0, JGLUE 0), "
            "breakable (IGLUE not 0, JGLUE 1 or 2) or touching; nodePairCount, minDistance and maxDistance "
            "describe the grid points that meet (contactNodePairs); tolerance is the distance used (the ERROR or "
            "DQNEAR of the pair, else contactTolerance, else 5 % of the median edge length of the secondary "
            "surface); meets is false for a pair whose surfaces do not meet, which validation reports "
            "(contact.pair-apart) and the components do not merge. glueOption (IGLUE), glueSeparation (JGLUE), "
            "coordinateCheck (ICOORD), "
            "searchOption (ISEARCH), errorTolerance (ERROR), toleranceBias (BIAS) and nearDistance (DQNEAR) are the "
            "parameters in force as the solver resolves them (glued contact), each with "
            "the card that sets it (`Source`: the pair's own BCTABLE section or BCONPRG, the BCONPRP for DQNEAR, "
            "BCPARA for ERROR and for the BIAS of a pair without glue, else default): errorTolerance null is the "
            "solver's "
            "automatic "
            "value, nearDistance null no near contact; the default toleranceBias is 0.0 with the glue, 0.9 without it. "
            "rule=\"solver\" "
            "counts "
            "the node pairs the solver ties (contactNodePairs), tolerance is then the solver's ERROR.";
        info.parameters = {contactToleranceParameter(), contactRuleParameter()};
        info.defaultSort = "table, secondaryBody, primaryBody";
        ColumnListBuilder columns;
        columns.text("table").int32("secondaryBody").int32("primaryBody").dictionary("kind");
        columns.int64("nodePairCount").real("minDistance").real("maxDistance").real("tolerance").flag("meets");
        for (std::size_t item = 0; item < PARAMETER_NAMES.size(); ++item)
        {
            const std::string name(PARAMETER_NAMES[item]);
            if (item < INTEGERS)
                columns.int32(name);
            else
                columns.real(name);
            columns.dictionary(name + "Source");
        }
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addContactPairsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ContactPairsTableQuery>());
}

} // namespace oor::query
