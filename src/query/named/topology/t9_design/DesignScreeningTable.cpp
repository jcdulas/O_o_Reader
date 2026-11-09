#include "oor/model/cards/optimisation/designConstrScreen/DesignConstrScreenTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// designScreeningTable: the DSCREEN cards.
class DesignScreeningTableQuery final : public QueryBase
{
public:
    DesignScreeningTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::DesignConstrScreenTable* table = run.model->findTable<model::DesignConstrScreenTable>();
        std::vector<model::DesignConstrScreenCard> cards;
        if (table != nullptr)
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row))
                    cards.push_back(table->card(row));
            }
        }
        if (cards.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(cards.begin(), cards.end(),
                         [](const model::DesignConstrScreenCard& left, const model::DesignConstrScreenCard& right)
                         { return left.responseKind < right.responseKind; });
        const std::size_t count = cards.size();
        std::vector<std::string> type(count);
        std::vector<double> threshold(count);
        std::vector<std::int32_t> retained(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            type[index] = cards[index].responseKind;
            threshold[index] = cards[index].threshold;
            retained[index] = cards[index].maxRetained;
        }
        TableBuilder builder;
        builder.dictionary("responseType", type);
        builder.real("threshold", std::move(threshold));
        builder.int32("maxRetained", std::move(retained));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designScreeningTable";
        info.family = "T9";
        info.description =
            "The screening of the design constraints (DSCREEN cards), one row per card: the response type it applies "
            "to (a DRESP1 type, EQUA for the DRESP2 responses, DRESP3, or ALL), the truncation `threshold` TRS (a "
            "constraint is kept only above it; -0.5 when blank) and `maxRetained` NSTR, the most constraints kept for "
            "each region of each subcase (20 when blank). The defaults of the solver for the types without a DSCREEN "
            "card are not listed.";
        info.defaultSort = "responseType";
        ColumnListBuilder columns;
        columns.dictionary("responseType").real("threshold").int32("maxRetained");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignScreeningTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignScreeningTableQuery>());
}

} // namespace oor::query
