#pragma once

#include "oor/results/core/NamedResult.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::results
{

// A named result made in memory (a companion package or a test hands its results to this package without
// a result file). The kind is one of the registered kinds (`allNamedKinds()`): the flutter summary, the trim variables,
// the stability derivatives, the aerodynamic pressures and forces, the optimisation tables, ... The arrays and texts
// given must be those the kind documents, by name, with the shapes it says; the numbers are copied.

// One array of the result: its name, the type of its numbers, its shape, and the numbers as bytes in C order (the
// layout of a NumPy array of that type).
struct NamedArrayInput
{
    std::string name;
    ArrayType type = ArrayType::Float64;
    std::vector<std::size_t> shape;
    std::span<const std::byte> bytes;
};

// One list of texts, a text for each row.
struct NamedTextInput
{
    std::string name;
    std::vector<std::string> values;
};

// What a result made in memory is told besides its arrays: the subcase, free-form facts (the Mach number, the reference
// values), and the analysis code (1 for a static analysis, 6 for a transient one, ...) when it is known.
struct NamedResultOrigin
{
    std::int32_t subcase = 1;
    std::int32_t analysisCode = 0;
    // The design cycle of a SOL 200 run the result belongs to (-1: none) and the name it is called by (the kind when
    // empty).
    std::int32_t designCycle = -1;
    std::string name;
    std::vector<std::pair<std::string, std::string>> attributes;
};

// The problems with `arrays` and `texts` as a result of `kind` (an empty list when they are right): an unknown kind, an
// array or a text the kind does not have, one that is missing, a type that is not the kind's (the numbers of a table of
// "float" arrays are all single precision or all double precision), a shape that is not the kind's, and rows that
// differ between the arrays of the same result (the first extent of every array that has rows is the same).
[[nodiscard]] std::vector<std::string> checkNamedResult(std::string_view kind, std::span<const NamedArrayInput> arrays,
                                                        std::span<const NamedTextInput> texts);

// Builds the result: its status is `notValidated` (no reader of a file vouches for it), `origin` is "memory" among its
// attributes, and it is decoded already. Throws UsageError with every problem `checkNamedResult` finds.
[[nodiscard]] std::shared_ptr<NamedResult> makeNamedResult(std::string_view kind, const NamedResultOrigin& origin,
                                                           std::span<const NamedArrayInput> arrays,
                                                           std::span<const NamedTextInput> texts);

} // namespace oor::results
