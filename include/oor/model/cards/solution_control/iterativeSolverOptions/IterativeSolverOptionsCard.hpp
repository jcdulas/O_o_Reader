#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace oor::model
{

// The value of an option: an integer, a real or a word, as `IterativeSolverOptionsCatalogue.hpp` says each option takes
// it. A word is kept in upper case.
using IterativeSolverOptionsValue = std::variant<std::int32_t, double, std::string>;

// An option of the iterative solver and its value.
struct IterativeSolverOptionsOption
{
    std::string name; // upper case
    IterativeSolverOptionsValue value;

    [[nodiscard]] bool operator==(const IterativeSolverOptionsOption&) const noexcept = default;
};

// Iterative solver options, selected by IterativeMethodSelection.
// Each option is one name=value text value after the identifier, for example:
// IterativeSolverOptions;20;PreconditionerKind=Jacobi;IterationTolerance=1.0e-6
// Whitespace around the equals sign is accepted. Names and registered word values use exact neutral spelling.
// The catalogue defines each registered option's type and accepted values. Unknown names retain their values
// as integers, reals or words. An option may appear only once; omitted options retain their defaults.
//
//
//
//
//
//
//
//
//
//
//
//
struct IterativeSolverOptionsCard
{
    base::EntityId id = 0; // SID
    std::vector<IterativeSolverOptionsOption> options;

    [[nodiscard]] bool operator==(const IterativeSolverOptionsCard&) const noexcept = default;
};

} // namespace oor::model
