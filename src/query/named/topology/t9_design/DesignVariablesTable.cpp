#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

namespace oor::query
{

namespace
{

// designVariablesTable: the DESVAR cards.
class DesignVariablesTableQuery final : public QueryBase
{
public:
    DesignVariablesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::DesignModel design = model::DesignModel::read(*run.model);
        const std::vector<model::DesignVariable>& variables = design.variables();
        if (variables.empty())
            return emptyTable(info().defaultColumns());
        const std::size_t count = variables.size();
        std::vector<std::int32_t> id(count);
        std::vector<std::string> label(count);
        std::vector<double> value(count), lower(count), upper(count), delta(count);
        std::vector<std::uint8_t> within(count), ddvalValid(count);
        std::vector<std::int32_t> ddval(count);
        std::vector<std::uint16_t> types(count, model::typeIndexOf<model::DesignVarTable>());
        std::vector<std::uint32_t> keys(count);
        for (std::size_t row = 0; row < count; ++row)
        {
            const model::DesignVariable& variable = variables[row];
            id[row] = variable.id;
            label[row] = variable.label;
            value[row] = variable.initial;
            lower[row] = variable.lower;
            upper[row] = variable.upper;
            delta[row] = variable.moveLimit;
            within[row] = variable.initial >= variable.lower && variable.initial <= variable.upper ? 1 : 0;
            keys[row] = variable.key.value;
            ddval[row] = variable.discreteValues;
            ddvalValid[row] = variable.discreteValues != 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("id", std::move(id), {}, ColumnOrigin::Stored, "id");
        builder.text("label", label);
        builder.real("value", std::move(value), {}, ColumnOrigin::Stored, "xinit");
        builder.real("lower", std::move(lower), {}, ColumnOrigin::Stored, "xlb");
        builder.real("upper", std::move(upper), {}, ColumnOrigin::Stored, "xub");
        builder.real("delta", std::move(delta));
        builder.flag("isWithinBounds", within);
        builder.int32("discreteValuesId", std::move(ddval), std::move(ddvalValid));
        builder.rows(std::move(types), std::move(keys), run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designVariablesTable";
        info.family = "T9";
        info.description =
            "The design variables (DESVAR cards): label, initial value (`value`, XINIT), bounds (`lower` XLB, `upper` "
            "XUB), "
            "`delta` (the fraction the variable may change in one design cycle: DELXV of the card, else DELX of the "
            "DOPTPRM, 0.5 without one) and whether the initial value is within the bounds. `value`, `lower` and "
            "`upper` "
            "can be edited and applied back to the cards. `discreteValuesId` is the DDVAL card that lists the discrete "
            "values the "
            "variable may take (null for a continuous variable).";
        info.defaultSort = "id";
        ColumnListBuilder columns;
        columns.int32("id").text("label").real("value", true).real("lower", true).real("upper", true).real("delta");
        columns.flag("isWithinBounds").int32("discreteValuesId");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignVariablesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignVariablesTableQuery>());
}

} // namespace oor::query
