#include "oor/model/design/DesignModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// designLinksTable: the DLINK cards, one row for each independent variable of each.
class DesignLinksTableQuery final : public QueryBase
{
public:
    DesignLinksTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::DesignModel design = model::DesignModel::read(*run.model);
        std::vector<std::int32_t> dependent, independent;
        std::vector<double> constant, multiplier, coefficient;
        std::vector<const model::DesignLink*> links;
        for (const model::DesignLink& link : design.links())
            links.push_back(&link);
        std::stable_sort(links.begin(), links.end(), [](const model::DesignLink* left, const model::DesignLink* right)
                         { return left->dependent < right->dependent; });
        for (const model::DesignLink* link : links)
        {
            for (const model::DesignTerm& term : link->terms)
            {
                dependent.push_back(link->dependent);
                constant.push_back(link->constant);
                multiplier.push_back(link->multiplier);
                independent.push_back(term.designVarId);
                coefficient.push_back(term.coefficient);
            }
        }
        if (dependent.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.int32("dependentVariable", std::move(dependent));
        builder.real("constant", std::move(constant));
        builder.real("multiplier", std::move(multiplier));
        builder.int32("independentVariable", std::move(independent));
        builder.real("coefficient", std::move(coefficient));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designLinksTable";
        info.family = "T9";
        info.description =
            "The DLINK cards, one row for each independent variable: a dependent design variable equals its `constant` "
            "plus "
            "its `multiplier` times the sum of the `coefficient` times the `independentVariable` over the rows of the "
            "card.";
        info.defaultSort = "dependentVariable (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("dependentVariable").real("constant").real("multiplier").int32("independentVariable");
        columns.real("coefficient");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignLinksTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignLinksTableQuery>());
}

} // namespace oor::query
