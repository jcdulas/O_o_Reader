#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace oor::deck
{

// The text of one record of a model file (.oos, docs/model-format.md): values separated by ';', spaces around a value
// ignored. A value that holds a ';' or a '"', or starts or ends with a space, is quoted with '"' and a '"' inside it is
// doubled (the CSV rule). A blank value is nothing between two separators.
struct RecordText
{
    // Cuts `line` into its values (unquoted, spaces around them removed). False, with `problem` set, when a quote is
    // not closed or is followed by something other than a separator.
    [[nodiscard]] static bool split(std::string_view line, std::vector<std::string>& values, std::string& problem);

    // The line of `values`, quoted where needed; trailing blanks are omitted. A first value beginning with # or [
    // is quoted to keep it distinct from comments and sections. Throws UsageError for an embedded CR or LF.
    [[nodiscard]] static std::string format(const std::vector<std::string>& values);

    // True when `value` must be quoted to be read back as it is.
    [[nodiscard]] static bool needsQuotes(std::string_view value) noexcept;

    // Adds a leading zero to a decimal fraction for canonical model text.
    [[nodiscard]] static std::string realText(std::string text);
};

} // namespace oor::deck
