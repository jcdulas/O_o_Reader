#include "oor/query/named/topology/t9_design/EquationProgramTables.hpp"

#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"
#include "oor/query/named/TableBuilder.hpp"

#include <algorithm>

namespace oor::query
{

std::vector<NumberedProgram> EquationProgramTables::ofModel(const model::Model& model,
                                                            const std::optional<std::vector<std::int64_t>>& equationIds)
{
    std::vector<NumberedProgram> programs;
    const model::DesignEquationTable* table = model.findTable<model::DesignEquationTable>();
    if (table == nullptr)
        return programs;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const model::DesignEquationCard card = table->card(row);
        if (equationIds &&
            !std::binary_search(equationIds->begin(), equationIds->end(), static_cast<std::int64_t>(card.id)))
            continue;
        model::EquationProgramBuild build = model::compileEquationProgram(card.text);
        if (build.ok())
            programs.push_back(NumberedProgram{card.id, std::move(*build.program)});
    }
    std::stable_sort(programs.begin(), programs.end(), [](const NumberedProgram& left, const NumberedProgram& right)
                     { return left.equationId < right.equationId; });
    return programs;
}

ColumnSet EquationProgramTables::symbolColumns()
{
    ColumnListBuilder columns;
    columns.int32("equationId").dictionary("kind").int32("slot").text("name");
    return columns.build();
}

ColumnSet EquationProgramTables::instructionColumns()
{
    ColumnListBuilder columns;
    columns.int32("equationId").int32("statement").dictionary("opcode").int32("operand").real("value");
    columns.dictionary("function").int32("position");
    return columns.build();
}

std::shared_ptr<const QueryTable> EquationProgramTables::symbols(const std::vector<NumberedProgram>& programs)
{
    std::vector<std::int32_t> equation, slot;
    std::vector<std::string> kind, name;
    for (const NumberedProgram& numbered : programs)
    {
        for (const model::EquationSymbol& symbol : numbered.program.symbols())
        {
            equation.push_back(numbered.equationId);
            kind.emplace_back(model::symbolKindName(symbol.kind));
            slot.push_back(symbol.slot);
            name.push_back(symbol.name);
        }
    }
    if (equation.empty())
        return emptyTable(symbolColumns());
    TableBuilder builder;
    builder.int32("equationId", std::move(equation));
    builder.dictionary("kind", kind);
    builder.int32("slot", std::move(slot));
    builder.text("name", name);
    return builder.finish();
}

std::shared_ptr<const QueryTable> EquationProgramTables::instructions(const std::vector<NumberedProgram>& programs)
{
    std::vector<std::int32_t> equation, statement, operand, position;
    std::vector<std::string> opcode, function;
    std::vector<std::uint8_t> functionValid;
    std::vector<double> value;
    for (const NumberedProgram& numbered : programs)
    {
        for (const model::EquationInstruction& instruction : numbered.program.instructions())
        {
            equation.push_back(numbered.equationId);
            statement.push_back(instruction.statement);
            opcode.emplace_back(model::opcodeName(instruction.opcode));
            operand.push_back(instruction.operand);
            value.push_back(instruction.value);
            function.emplace_back(instruction.function ? model::infoOf(*instruction.function).name : "");
            functionValid.push_back(instruction.function ? 1 : 0);
            position.push_back(instruction.position);
        }
    }
    if (equation.empty())
        return emptyTable(instructionColumns());
    const std::size_t count = equation.size();
    TableBuilder builder;
    builder.int32("equationId", std::move(equation));
    builder.int32("statement", std::move(statement));
    builder.dictionary("opcode", opcode);
    builder.int32("operand", std::move(operand));
    // The value of a constant; 0 for the other instructions (never null: NaN would make it one).
    builder.real("value", std::move(value), std::vector<std::uint8_t>(count, 1));
    builder.dictionary("function", function, std::move(functionValid));
    builder.int32("position", std::move(position));
    return builder.finish();
}

} // namespace oor::query
