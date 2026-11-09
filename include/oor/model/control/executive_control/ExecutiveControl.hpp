#pragma once

#include "oor/model/control/statement/ControlStatementList.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// Typed executive records and editing by exact neutral names.
class ExecutiveControl : public ControlStatementList
{
public:
    // --- queries -----------------------------------------------------------------------------------------------------
    // `setSolution` takes a number or a neutral name of SolutionSequences ("101", "LinearStatics").
    [[nodiscard]] std::optional<std::int64_t> solutionNumber() const;
    [[nodiscard]] std::vector<std::int64_t> diagnosticSelectionNumbers() const;

    // --- editing -----------------------------------------------------------------------------------------------------
    // Throw base::UsageError for a value that cannot be written as the statement (an empty name, a word with blanks).
    ControlStatement& setJobIdentifier(std::vector<std::string> names);
    ControlStatement& setSolution(std::string solution, std::string second = {},
                                  std::vector<StatementParameter> options = {});
    ControlStatement& setTimeLimit(std::vector<std::string> numbers);
    void setDiagnosticSelection(const std::vector<std::int64_t>& numbers);
    // A new statement of a keyword shape, by its neutral name (one of the definitions of `executiveStatements()`), at
    // its place.
    ControlStatement& addKeyword(std::string_view name, std::vector<StatementParameter> parameters);
    bool removeStatement(std::uint32_t key);

private:
    // Where a new statement of this neutral name goes.
    [[nodiscard]] std::size_t insertionIndex(std::string_view name) const;
};

} // namespace oor::model
