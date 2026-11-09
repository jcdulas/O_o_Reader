#include "DesignQuerySupport.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// designConstraintsTable: the DCONSTR cards, with the subcases whose DESSUB or DESGLB selects their set.
class DesignConstraintsTableQuery final : public QueryBase
{
public:
    DesignConstraintsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::DesignConstrTable* table = model.findTable<model::DesignConstrTable>();
        std::vector<model::DesignConstrCard> cards;
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
                         [](const model::DesignConstrCard& left, const model::DesignConstrCard& right)
                         {
                             return left.constraintSetId != right.constraintSetId
                                        ? left.constraintSetId < right.constraintSetId
                                        : left.responseId < right.responseId;
                         });

        const DesignCase designCase = readDesignCase(model);
        const std::size_t count = cards.size();
        std::vector<std::int32_t> set(count), response(count);
        std::vector<double> lower(count), upper(count), lowFrequency(count), highFrequency(count);
        std::vector<std::int32_t> lowerTable(count), upperTable(count);
        std::vector<std::uint8_t> lowerTableValid(count), upperTableValid(count);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> subcases;
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::DesignConstrCard& card = cards[index];
            set[index] = card.constraintSetId;
            response[index] = card.responseId;
            // A bound that is a table of frequencies, or the default (no bound), is null.
            const bool noLower =
                card.lowerAllowableTableId != 0 || card.lowerAllowable <= model::DesignConstrCard::NO_LOWER_BOUND;
            const bool noUpper =
                card.upperAllowableTableId != 0 || card.upperAllowable >= model::DesignConstrCard::NO_UPPER_BOUND;
            lower[index] = noLower ? NOT_A_NUMBER : card.lowerAllowable;
            upper[index] = noUpper ? NOT_A_NUMBER : card.upperAllowable;
            // The range of frequencies the bounds apply to: null when it is the default, every frequency.
            lowFrequency[index] = card.lowFrequency == 0.0 ? NOT_A_NUMBER : card.lowFrequency;
            highFrequency[index] =
                card.highFrequency >= model::DesignConstrCard::NO_HIGH_FREQUENCY ? NOT_A_NUMBER : card.highFrequency;
            lowerTable[index] = card.lowerAllowableTableId;
            lowerTableValid[index] = card.lowerAllowableTableId != 0 ? 1 : 0;
            upperTable[index] = card.upperAllowableTableId;
            upperTableValid[index] = card.upperAllowableTableId != 0 ? 1 : 0;
            if (const auto found = designCase.subcasesOfSet.find(card.constraintSetId);
                found != designCase.subcasesOfSet.end())
                subcases.insert(subcases.end(), found->second.begin(), found->second.end());
            offsets.push_back(static_cast<std::int64_t>(subcases.size()));
        }
        TableBuilder builder;
        builder.int32("constraintSet", std::move(set));
        builder.int32("responseId", std::move(response));
        builder.real("lower", std::move(lower));
        builder.real("upper", std::move(upper));
        builder.intList("subcases", std::move(offsets), std::move(subcases));
        builder.real("lowFrequency", std::move(lowFrequency));
        builder.real("highFrequency", std::move(highFrequency));
        builder.int32("lowerTable", std::move(lowerTable), std::move(lowerTableValid));
        builder.int32("upperTable", std::move(upperTable), std::move(upperTableValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designConstraintsTable";
        info.family = "T9";
        info.description =
            "The design constraints (DCONSTR cards): the set, the response it bounds, the bounds (null for a bound the "
            "card "
            "does not give, or that is a table of frequencies) and the subcases whose DESSUB or DESGLB selects the "
            "set, "
            "directly or through a DCONADD. `lowFrequency` and `highFrequency` are LOWFQ and HIGHFQ, the range of "
            "frequencies the bounds apply to (null when the card leaves the range open: 0.0 and 1.0E20); "
            "`lowerTable` and `upperTable` are the TABLEDi that give a bound as a function of frequency (the bound "
            "is then null).";
        info.defaultSort = "constraintSet, responseId";
        ColumnListBuilder columns;
        columns.int32("constraintSet").int32("responseId").real("lower").real("upper").intList("subcases");
        columns.real("lowFrequency").real("highFrequency").int32("lowerTable").int32("upperTable");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignConstraintsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignConstraintsTableQuery>());
}

} // namespace oor::query
