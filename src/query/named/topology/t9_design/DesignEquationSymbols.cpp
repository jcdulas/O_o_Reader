#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"
#include "oor/query/named/topology/t9_design/EquationProgramTables.hpp"

namespace oor::query
{

namespace
{

// designEquationSymbols: the names of each DEQATN as an expression program.
class DesignEquationSymbolsQuery final : public QueryBase
{
public:
    DesignEquationSymbolsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        return EquationProgramTables::symbols(
            EquationProgramTables::ofModel(*run.model, sortedIds(run.arguments.ids("equationIds"))));
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designEquationSymbols";
        info.family = "T9";
        info.description =
            "Each DEQATN as an expression program (with `designEquationInstructions`): one row per name, the arguments "
            "first "
            "(`kind` argument, `slot` the position in the argument list of the first equation, from 0), then the "
            "variables (`kind` variable, `slot` the index of the statement, from 0, that gives it: the first equation "
            "gives the name of the function); `name` in upper case. The names of an equation are unique, every slot is "
            "there once per kind, and there is at least one variable. A DEQATN that does not parse, or one whose "
            "equation gives a name an argument or an earlier equation already has, has no row.";
        info.parameters = {parameter("equationIds", ParameterKind::IdList, "None", "keep these DEQATN")};
        info.defaultSort = "equationId, kind (arguments first), slot";
        info.columnSets.push_back(EquationProgramTables::symbolColumns());
        return info;
    }
};

} // namespace

void addDesignEquationSymbolsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignEquationSymbolsQuery>());
}

} // namespace oor::query
