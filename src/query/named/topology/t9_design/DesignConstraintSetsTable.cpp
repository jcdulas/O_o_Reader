#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <utility>

namespace oor::query
{

namespace
{

// designConstraintSetsTable: the DCONADD cards, one row per member set.
class DesignConstraintSetsTableQuery final : public QueryBase
{
public:
    DesignConstraintSetsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::DesignConstrUnionTable* table = run.model->findTable<model::DesignConstrUnionTable>();
        std::vector<std::pair<std::int32_t, std::vector<std::int32_t>>> unions;
        if (table != nullptr)
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const model::DesignConstrUnionCard card = table->card(row);
                unions.emplace_back(card.constraintSetId,
                                    std::vector<std::int32_t>(card.sets.begin(), card.sets.end()));
            }
        }
        std::stable_sort(unions.begin(), unions.end(),
                         [](const auto& left, const auto& right) { return left.first < right.first; });
        std::vector<std::int32_t> set, member;
        for (const auto& [dcid, sets] : unions)
        {
            for (const std::int32_t value : sets)
            {
                set.push_back(dcid);
                member.push_back(value);
            }
        }
        if (set.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.int32("setId", std::move(set));
        builder.int32("memberSet", std::move(member));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designConstraintSetsTable";
        info.family = "T9";
        info.description =
            "The unions of design constraint sets (DCONADD cards): one row for each DCONSTR set `memberSet` that the "
            "set `setId` adds, in the order of the card. A DESSUB or DESGLB that selects `setId` selects its members "
            "(`designSubcasesTable` gives them expanded).";
        info.defaultSort = "setId, the order of the card";
        ColumnListBuilder columns;
        columns.int32("setId").int32("memberSet");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignConstraintSetsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignConstraintSetsTableQuery>());
}

} // namespace oor::query
