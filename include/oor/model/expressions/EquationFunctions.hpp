#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The intrinsic functions of a DEQATN. The enumerators follow the order of EQUATION_FUNCTIONS.
enum class EquationFunction : std::uint8_t
{
    Abs,
    Acos,
    Acosh,
    Asin,
    Asinh,
    Atan,
    Atan2,
    Atanh,
    Atanh2,
    Avg,
    Cos,
    Cosh,
    Db,
    Dba,
    Dim,
    Exp,
    Invdb,
    Invdba,
    Log,
    Log10,
    Logx,
    Max,
    Min,
    Mod,
    Pi,
    Rss,
    Sin,
    Sinh,
    Sqrt,
    Ssq,
    Sum,
    Tan,
    Tanh,
};

// Arity of a function: the format reference limits the functions that take any number of arguments (AVG, MAX, MIN, RSS,
// SSQ, SUM) to fewer than 97.
inline constexpr std::size_t MAX_VARIADIC_ARGUMENTS = 96;

struct EquationFunctionInfo
{
    std::string_view name;
    std::size_t minimumArguments;
    std::size_t maximumArguments;
};

inline constexpr std::array<EquationFunctionInfo, 33> EQUATION_FUNCTIONS{{
    {"ABS", 1, 1},
    {"ACOS", 1, 1},
    {"ACOSH", 1, 1},
    {"ASIN", 1, 1},
    {"ASINH", 1, 1},
    {"ATAN", 1, 1},
    {"ATAN2", 2, 2},
    {"ATANH", 1, 1},
    {"ATANH2", 2, 2},
    {"AVG", 1, MAX_VARIADIC_ARGUMENTS},
    {"COS", 1, 1},
    {"COSH", 1, 1},
    {"DB", 2, 2},
    {"DBA", 3, 3},
    {"DIM", 2, 2},
    {"EXP", 1, 1},
    {"INVDB", 2, 2},
    {"INVDBA", 3, 3},
    {"LOG", 1, 1},
    {"LOG10", 1, 1},
    {"LOGX", 2, 2},
    {"MAX", 1, MAX_VARIADIC_ARGUMENTS},
    {"MIN", 1, MAX_VARIADIC_ARGUMENTS},
    {"MOD", 2, 2},
    {"PI", 1, 1},
    {"RSS", 1, MAX_VARIADIC_ARGUMENTS},
    {"SIN", 1, 1},
    {"SINH", 1, 1},
    {"SQRT", 1, 1},
    {"SSQ", 1, MAX_VARIADIC_ARGUMENTS},
    {"SUM", 1, MAX_VARIADIC_ARGUMENTS},
    {"TAN", 1, 1},
    {"TANH", 1, 1},
}};

[[nodiscard]] constexpr const EquationFunctionInfo& infoOf(EquationFunction function) noexcept
{
    return EQUATION_FUNCTIONS[static_cast<std::size_t>(function)];
}

// The function with this name (upper case: the parser upper-cases every name).
[[nodiscard]] std::optional<EquationFunction> findEquationFunction(std::string_view name) noexcept;

} // namespace oor::model
