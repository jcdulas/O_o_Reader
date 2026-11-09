#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsCard.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace oor::model
{

// The kind of value an option of the ITER card takes.
enum class IterativeSolverOptionsKind : std::uint8_t
{
    Word,
    Real,
    Integer
};

// What the format allows the value to be.
enum class IterativeSolverOptionsRule : std::uint8_t
{
    OneOfWords,   // a word of a list
    PositiveReal, // a real greater than zero
    IntegerRange  // an integer from `low` to `high`
};

// One option of the ITER entry: its name, the kind of value, what the value may be and its default.
struct IterativeSolverOptionsInfo
{
    std::string_view name; // the neutral name (PreconditionerKind for PRECOND)
    IterativeSolverOptionsKind kind;
    IterativeSolverOptionsRule rule;
    std::int32_t low; // the ends of an IntegerRange
    std::int32_t high;
    std::span<const std::string_view> words; // the words of OneOfWords
    std::string_view defaultText;            // what the solver uses when the card does not give the option
    std::string_view note;
};

inline constexpr std::int32_t ITERATIVE_SOLVER_MOST_INTEGER = 2147483647;

inline constexpr std::array<std::string_view, 10> PRECONDITIONER_WORDS{"Jacobi",
                                                                       "JacobiScaled",
                                                                       "IncompleteCholesky",
                                                                       "IncompleteCholeskyScaled",
                                                                       "RowIncompleteCholesky",
                                                                       "RowIncompleteCholeskySymmetric",
                                                                       "BlockIncompleteCholesky",
                                                                       "BlockIncompleteCholeskyComplex",
                                                                       "CacheAwareIncomplete",
                                                                       "User"};
inline constexpr std::array<std::string_view, 4> CONVERGENCE_CRITERIA_WORDS{
    "ResidualRatioCriterion", "EnergyNormCriterion", "ResidualRatioCriterionExtended", "EnergyNormCriterionExtended"};
inline constexpr std::array<std::string_view, 2> ITERATION_MESSAGES_WORDS{"Yes", "No"};

// The eight options of the ITER entry, typed by hand. IPAD asks for a positive integer and also gives the
// default 0 for the reduced incomplete Cholesky preconditioners, so zero is accepted; IEXT is from 0 to 7 and PREFONLY
// is 0 or -1.
inline constexpr std::array<IterativeSolverOptionsInfo, 8> ITERATIVE_SOLVER_OPTIONS_CATALOGUE{{
    {"PreconditionerKind", IterativeSolverOptionsKind::Word, IterativeSolverOptionsRule::OneOfWords, 0, 0,
     PRECONDITIONER_WORDS, "BIC for a real analysis, BICCMPLX for a complex one", "the preconditioner"},
    {"ConvergenceCriteria", IterativeSolverOptionsKind::Word, IterativeSolverOptionsRule::OneOfWords, 0, 0,
     CONVERGENCE_CRITERIA_WORDS, "ResidualRatioCriterionExtended", "the convergence criterion"},
    {"IterationMessages", IterativeSolverOptionsKind::Word, IterativeSolverOptionsRule::OneOfWords, 0, 0,
     ITERATION_MESSAGES_WORDS, "No", "YES reports the progress of every iteration"},
    {"IterationTolerance",
     IterativeSolverOptionsKind::Real,
     IterativeSolverOptionsRule::PositiveReal,
     0,
     0,
     {},
     "1.E-6; 1.E-4 for PRECOND = CASI and 1.E-8 for CASI with contact in the model",
     "the convergence tolerance"},
    {"MaxIterationSteps",
     IterativeSolverOptionsKind::Integer,
     IterativeSolverOptionsRule::IntegerRange,
     1,
     ITERATIVE_SOLVER_MOST_INTEGER,
     {},
     "a quarter of the matrix size",
     "the iteration limit"},
    {"PreconditionerPadding",
     IterativeSolverOptionsKind::Integer,
     IterativeSolverOptionsRule::IntegerRange,
     0,
     ITERATIVE_SOLVER_MOST_INTEGER,
     {},
     "0 for RIC and RICS, 2 (3 for two-dimensional and mixed models) for BIC, 5 for BICCMPLX",
     "the fill-in padding of the RIC, RICS, BIC and BICCMPLX preconditioners"},
    {"PreconditionerExtractionLevel",
     IterativeSolverOptionsKind::Integer,
     IterativeSolverOptionsRule::IntegerRange,
     0,
     7,
     {},
     "0",
     "the extraction level of the RIC and RICS preconditioners (0: from the degree-of-freedom sets)"},
    {"EstimateResourcesOnly",
     IterativeSolverOptionsKind::Integer,
     IterativeSolverOptionsRule::IntegerRange,
     -1,
     0,
     {},
     "0",
     "-1 ends the run once the resources are estimated"},
}};

// The entry of a neutral name; null for a name the catalogue does not list (a card may use such a name).
[[nodiscard]] inline const IterativeSolverOptionsInfo* findIterativeSolverOption(std::string_view name) noexcept
{
    for (const IterativeSolverOptionsInfo& info : ITERATIVE_SOLVER_OPTIONS_CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

// The name of a kind: "word", "real" or "integer".
[[nodiscard]] inline std::string_view iterativeSolverOptionsKindName(IterativeSolverOptionsKind kind) noexcept
{
    switch (kind)
    {
    case IterativeSolverOptionsKind::Word:
        return "word";
    case IterativeSolverOptionsKind::Real:
        return "real";
    case IterativeSolverOptionsKind::Integer:
        return "integer";
    }
    return "word";
}

// What the value of an option may be, as a phrase: "one of J, JS or C", "a real greater than 0", "an integer from 0 to
// 7".
[[nodiscard]] inline std::string iterativeSolverOptionsRuleText(const IterativeSolverOptionsInfo& info)
{
    switch (info.rule)
    {
    case IterativeSolverOptionsRule::OneOfWords:
    {
        std::string phrase = "one of ";
        for (std::size_t index = 0; index < info.words.size(); ++index)
        {
            if (index > 0)
                phrase += index + 1 == info.words.size() ? " or " : ", ";
            phrase += info.words[index];
        }
        return phrase;
    }
    case IterativeSolverOptionsRule::PositiveReal:
        return "a real greater than 0";
    case IterativeSolverOptionsRule::IntegerRange:
        if (info.high == ITERATIVE_SOLVER_MOST_INTEGER)
            return info.low > 0 ? "an integer greater than 0" : "an integer, 0 or more";
        return "an integer from " + std::to_string(info.low) + " to " + std::to_string(info.high);
    }
    return {};
}

// Why a value does not suit an option: empty when it does. The value has the alternative of the kind of the option (a
// real option holds a double, an integer option an int32, a word option a string).
[[nodiscard]] inline std::string iterativeSolverOptionsValueProblem(const IterativeSolverOptionsInfo& info,
                                                                    const IterativeSolverOptionsValue& value)
{
    const std::string what = std::string(info.name) + " must be " + iterativeSolverOptionsRuleText(info);
    switch (info.kind)
    {
    case IterativeSolverOptionsKind::Word:
    {
        const std::string* word = std::get_if<std::string>(&value);
        if (word == nullptr)
            return what;
        for (const std::string_view allowed : info.words)
        {
            if ((allowed == *word))
                return {};
        }
        return what + ", not " + *word;
    }
    case IterativeSolverOptionsKind::Real:
    {
        const double* number = std::get_if<double>(&value);
        return number != nullptr && std::isfinite(*number) && *number > 0.0 ? std::string() : what;
    }
    case IterativeSolverOptionsKind::Integer:
    {
        const std::int32_t* number = std::get_if<std::int32_t>(&value);
        return number != nullptr && *number >= info.low && *number <= info.high ? std::string() : what;
    }
    }
    return what;
}

} // namespace oor::model
