#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

enum class CaseValueKind : std::uint8_t
{
    None,        // the command has no value (OUTPUT(PLOT), a request with no "= ...")
    Integer,     // SPC = 10
    Real,        // XPAPER = 29.
    Keyword,     // one exact neutral word, such as All or None
    Text,        // TITLE = a plate: the rest of the line as written
    IntegerList, // SEPLOT 0,1,2
    RealList     // SUBSEQ = 1.0,-0.5
};

// The value of one Case Control command, of one of the kinds above. A plain value type: the kind says which member
// holds the value (`integer`, `real`, `text`, `integers` or `reals`), the others stay empty.
struct CaseValue
{
    CaseValueKind kind = CaseValueKind::None;
    std::int64_t integer = 0; // with, for SUPER = i, l, the load sequence l as the one item of `integers`
    double real = 0.0;
    std::string text;
    std::vector<std::int64_t> integers;
    std::vector<double> reals;

    [[nodiscard]] static CaseValue ofInteger(std::int64_t value);
    [[nodiscard]] static CaseValue ofReal(double value);
    [[nodiscard]] static CaseValue ofKeyword(std::string word);
    [[nodiscard]] static CaseValue ofText(std::string text);
    [[nodiscard]] static CaseValue ofIntegers(std::vector<std::int64_t> values);
    [[nodiscard]] static CaseValue ofReals(std::vector<double> values);

    [[nodiscard]] bool isNone() const noexcept { return kind == CaseValueKind::None; }
    [[nodiscard]] bool operator==(const CaseValue&) const = default;

    // The value as canonical text: "10", "1.5", "NONE", the text itself, "1,2,3". Empty for None.
    [[nodiscard]] std::string format() const;
    // The items of a list value as text, one per entry (a scalar is one item): what the writer wraps over lines.
    [[nodiscard]] std::vector<std::string> items() const;
};

} // namespace oor::model
