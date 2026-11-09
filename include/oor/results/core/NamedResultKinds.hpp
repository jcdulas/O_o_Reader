#pragma once

#include <span>
#include <string_view>

namespace oor::results
{

// What a kind of named result holds, for the documentation and the tests: the arrays and texts by name. A named result
// of a file has exactly these (a test compares them with the decoded results).
struct NamedArrayInfo
{
    std::string_view name;
    std::string_view type;  // int32, int64, float (single in a 32-bit file, double in a 64-bit one)
    std::string_view shape; // "" for a number alone, else "(n)", "(n, m)", ...
    std::string_view description;
    bool text = false; // a list of texts, one a row, not a numeric array
    // The array may be absent: a result read from a file has none of the optional arrays (the labels of the rows and
    // columns of a matrix), one made in memory may give them.
    bool optional = false;
};

struct NamedKindInfo
{
    std::string_view kind;
    std::string_view description;
    std::span<const NamedArrayInfo> arrays;
    // The arrays of several dimensions whose first extent is the number of rows are columns of fixed lists in the
    // table of the result (`to_table()`, the query sources): the six forces of an aerodynamic grid point, the six
    // directions of a modal effective mass. False: the table has the one-dimensional arrays only.
    bool listColumns = false;
};

[[nodiscard]] std::span<const NamedKindInfo> allNamedKinds() noexcept;
[[nodiscard]] const NamedKindInfo* findNamedKind(std::string_view kind) noexcept;

} // namespace oor::results
