#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/expressions/EquationProgram.hpp"
#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace oor::query
{

// An expression program with the number of its equation (the EQID of the DEQATN; 0 for `compileEquationProgram`).
struct NumberedProgram
{
    std::int32_t equationId = 0;
    model::EquationProgram program;
};

// The tables of the expression programs (`designEquationSymbols` and `designEquationInstructions`), shared by the two
// queries and by `compileEquationProgram`, which gives the same two tables for one equation.
class EquationProgramTables
{
public:
    // The programs of the DEQATN cards of a model, by increasing EQID; with `equationIds`, those only. An equation that
    // does not parse (the card is then kept as text) or cannot be a program (EquationProgram.hpp) is left out.
    [[nodiscard]] static std::vector<NumberedProgram> ofModel(
        const model::Model& model, const std::optional<std::vector<std::int64_t>>& equationIds);

    // One row per symbol: equationId, kind (argument, variable), slot, name.
    [[nodiscard]] static std::shared_ptr<const QueryTable> symbols(const std::vector<NumberedProgram>& programs);
    // One row per instruction: equationId, statement, opcode, operand, value, function, position.
    [[nodiscard]] static std::shared_ptr<const QueryTable> instructions(const std::vector<NumberedProgram>& programs);

    [[nodiscard]] static ColumnSet symbolColumns();
    [[nodiscard]] static ColumnSet instructionColumns();
};

} // namespace oor::query
