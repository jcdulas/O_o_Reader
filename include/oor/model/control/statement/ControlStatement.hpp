#pragma once

#include "oor/model/control/statement/StatementParameter.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// The typed shape of an executive record.
enum class ControlKind : std::uint8_t
{
    Keyword,
    JobIdentifier,
    SolutionSequence,
    TimeLimit,
    DiagnosticSelection,
    Raw,
    Comment,
    IncludeBegin,
    IncludeEnd
};

struct ControlStatement
{
    ControlKind kind = ControlKind::Keyword;
    std::uint32_t key = 0;
    std::string name;
    std::vector<std::string> words;
    std::vector<StatementParameter> parameters;
    std::string leadingComments;
    std::string trailingComment;
    std::string original;
    std::string text;
    std::uint32_t includedFileIndex = 0;
    void markChanged() noexcept { original.clear(); }
    [[nodiscard]] bool isTyped() const noexcept
    {
        return kind != ControlKind::Raw && kind != ControlKind::Comment && kind != ControlKind::IncludeBegin &&
               kind != ControlKind::IncludeEnd;
    }
};
} // namespace oor::model
