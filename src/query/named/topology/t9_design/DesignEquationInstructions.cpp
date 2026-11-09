#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"
#include "oor/query/named/topology/t9_design/EquationProgramTables.hpp"

namespace oor::query
{

namespace
{

// designEquationInstructions: the stack machine instructions of each DEQATN.
class DesignEquationInstructionsQuery final : public QueryBase
{
public:
    DesignEquationInstructionsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        return EquationProgramTables::instructions(
            EquationProgramTables::ofModel(*run.model, sortedIds(run.arguments.ids("equationIds"))));
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designEquationInstructions";
        info.family = "T9";
        info.description =
            "Each DEQATN as the instructions of a stack machine, one row per instruction, the rows of an equation "
            "together and in the order they run; each `statement` (from 0) leaves the value of its variable on the "
            "stack, and the value of the equation is that of its last statement. `opcode`: `realConstant` and "
            "`integerConstant` push `value` (an integer constant is written without a point or an exponent: two "
            "integers give an integer, so 1/2 is 0; variables, arguments and the "
            "values of the "
            "functions are real); `argument` pushes the argument of slot `operand`, `variable` the value of the "
            "earlier statement `operand`; `negate` pops one value; `add`, `subtract`, `multiply`, `divide` and `power` "
            "pop b, then a, and push a + b, a - b, a * b, a / b, a ** b; `call` pops `operand` values (the last "
            "argument was pushed last) and pushes the value of `function` (its format reference name: SIN, AVG, ...). "
            "`position` is "
            "the character of the text of the equation (the lines of the card joined) the instruction comes from, "
            "from 1. The precedence of the format reference, the names (PI alone is the number) and the arities (AVG, "
            "MAX, MIN, "
            "RSS, SSQ and SUM take 1 to 96 arguments) are resolved; the design equation evaluator gives the same "
            "value.";
        info.parameters = {parameter("equationIds", ParameterKind::IdList, "None", "keep these DEQATN")};
        info.defaultSort = "equationId, execution order";
        info.columnSets.push_back(EquationProgramTables::instructionColumns());
        return info;
    }
};

} // namespace

void addDesignEquationInstructionsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignEquationInstructionsQuery>());
}

} // namespace oor::query
