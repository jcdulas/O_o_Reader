#pragma once

#include "oor/model/control/case_control/CaseValue.hpp"
#include "oor/model/control/case_control/CommandDefinition.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// What is wrong with a value offered to a command.
struct ValueProblem
{
    bool isWrongKind = false; // the value is not of a kind the command takes (the Python layer raises TypeError)
    std::string message;      // otherwise it is of the right kind but not acceptable (ValueError)
};

// Turns the text after a command into a value of the kind the command's shape takes, and checks values that are made
// in memory against the same rules.
struct CaseValueParser
{
    // Reads `text` as a value of the shape of `definition`. Returns nothing, with `problem` set, when it does not fit.
    [[nodiscard]] static std::optional<CaseValue> parse(const CommandDefinition& definition, std::string_view text,
                                                        std::string& problem);

    [[nodiscard]] static std::optional<ValueProblem> check(const CommandDefinition& definition, const CaseValue& value);
};

} // namespace oor::model
