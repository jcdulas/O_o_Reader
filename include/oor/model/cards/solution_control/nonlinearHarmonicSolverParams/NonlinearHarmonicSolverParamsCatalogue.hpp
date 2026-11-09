#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsCard.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace oor::model
{

// The kind of value a parameter takes (I, R or CHAR in the format).
enum class NonlinearHarmonicSolverParamsKind : std::uint8_t
{
    Integer,
    Real,
    Word
};

// One parameter of NHRMPRM: its name, the kind of value, what the solver uses
// when no NHRMPRM gives it and what the value may be. `integers` and `words` list the values of a parameter that has a
// short list (CONTP, DECOMP); the others take any value of their kind.
struct NonlinearHarmonicSolverParamsInfo
{
    std::string_view name; // the neutral name (NonlinearHarmonicTolerance for NLHTOL)
    NonlinearHarmonicSolverParamsKind kind;
    std::string_view defaultText;
    std::span<const std::int32_t> integers;
    std::span<const std::string_view> words;
    std::string_view note;
};

inline constexpr std::array<std::int32_t, 3> CONTINUATION_METHOD_VALUES{0, 1, 2};
inline constexpr std::array<std::string_view, 2> DECOMPOSITION_METHOD_WORDS{"LowerUpperDecomposition",
                                                                            "QrDecomposition"};

// The twelve parameters of NHRMPRM, typed by hand. MXICODE0 has two defaults, 5 and 1000 when CONTP is
// more than 0, so its default is a phrase.
inline constexpr std::array<NonlinearHarmonicSolverParamsInfo, 12> NONLINEAR_HARMONIC_SOLVER_PARAMS_CATALOGUE{{
    {"NonlinearHarmonicReduction",
     NonlinearHarmonicSolverParamsKind::Integer,
     "-1",
     {},
     {},
     "whether the nonlinear harmonic analysis is reduced"},
    {"ExtraSamplingPoints",
     NonlinearHarmonicSolverParamsKind::Integer,
     "20",
     {},
     {},
     "extra sampling points, against aliasing"},
    {"NonlinearHarmonicTolerance",
     NonlinearHarmonicSolverParamsKind::Real,
     "1.0E-5",
     {},
     {},
     "the convergence tolerance"},
    {"LineSearchStepFactor",
     NonlinearHarmonicSolverParamsKind::Real,
     "1.1",
     {},
     {},
     "the step factor of the line search after a failed convergence"},
    {"MaxNonConvergedSteps",
     NonlinearHarmonicSolverParamsKind::Integer,
     "5 (1000 when CONTP > 0)",
     {},
     {},
     "how many steps may fail to converge before a new trial displacement"},
    {"InitialConditionOutput",
     NonlinearHarmonicSolverParamsKind::Integer,
     "0",
     {},
     {},
     "-1 writes TIC cards for the d-set, to start a later transient run"},
    {"MaxNonConvergedFrequencies",
     NonlinearHarmonicSolverParamsKind::Integer,
     "1000",
     {},
     {},
     "how many frequencies in a row may fail to converge before a reset"},
    {"MaxFrequencyInterval",
     NonlinearHarmonicSolverParamsKind::Real,
     "10.0",
     {},
     {},
     "the widest frequency interval before a reset"},
    {"NonlinearHarmonicDiagnostic",
     NonlinearHarmonicSolverParamsKind::Integer,
     "0",
     {},
     {},
     "nonzero prints extra diagnostics"},
    {"ContinuationMethod",
     NonlinearHarmonicSolverParamsKind::Integer,
     "0",
     CONTINUATION_METHOD_VALUES,
     {},
     "the continuation method: 0 none, 1 restarts a non-converging frequency from zero, 2 the improved method (a "
     "modified Moore-Penrose step)"},
    {"DecompositionMethod",
     NonlinearHarmonicSolverParamsKind::Word,
     "LowerUpperDecomposition",
     {},
     DECOMPOSITION_METHOD_WORDS,
     "the matrix factorisation, LU or QR (QR needs CONTP = 2)"},
    {"StepHalvingIterations",
     NonlinearHarmonicSolverParamsKind::Integer,
     "8",
     {},
     {},
     "the frequency step is halved after this many iterations"},
}};

// The entry of a neutral name; null for a name the table does not list (a card may use such a name).
[[nodiscard]] inline const NonlinearHarmonicSolverParamsInfo* findNonlinearHarmonicSolverParam(
    std::string_view name) noexcept
{
    for (const NonlinearHarmonicSolverParamsInfo& info : NONLINEAR_HARMONIC_SOLVER_PARAMS_CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

// The name of a kind: "integer", "real" or "word".
[[nodiscard]] inline std::string_view nonlinearHarmonicSolverParamsKindName(
    NonlinearHarmonicSolverParamsKind kind) noexcept
{
    switch (kind)
    {
    case NonlinearHarmonicSolverParamsKind::Integer:
        return "integer";
    case NonlinearHarmonicSolverParamsKind::Real:
        return "real";
    case NonlinearHarmonicSolverParamsKind::Word:
        return "word";
    }
    return "integer";
}

// What the value of a parameter may be, as a phrase: "0, 1 or 2", "LU or QR", "an integer", "a real".
[[nodiscard]] inline std::string nonlinearHarmonicSolverParamsValuesText(const NonlinearHarmonicSolverParamsInfo& info)
{
    std::string phrase;
    if (!info.integers.empty())
    {
        for (std::size_t index = 0; index < info.integers.size(); ++index)
        {
            if (index > 0)
                phrase += index + 1 == info.integers.size() ? " or " : ", ";
            phrase += std::to_string(info.integers[index]);
        }
        return phrase;
    }
    if (!info.words.empty())
    {
        for (std::size_t index = 0; index < info.words.size(); ++index)
        {
            if (index > 0)
                phrase += index + 1 == info.words.size() ? " or " : ", ";
            phrase += info.words[index];
        }
        return phrase;
    }
    return info.kind == NonlinearHarmonicSolverParamsKind::Real ? "a real" : "an integer";
}

// Why a value does not suit a parameter: empty when it does. The value has the alternative of the kind of the parameter
// (a real parameter holds a double, an integer one an int32, DECOMP a string).
[[nodiscard]] inline std::string nonlinearHarmonicSolverParamsValueProblem(
    const NonlinearHarmonicSolverParamsInfo& info, const NonlinearHarmonicSolverParamsValue& value)
{
    const std::string what = std::string(info.name) + " must be " + nonlinearHarmonicSolverParamsValuesText(info);
    switch (info.kind)
    {
    case NonlinearHarmonicSolverParamsKind::Integer:
    {
        const std::int32_t* number = std::get_if<std::int32_t>(&value);
        if (number == nullptr)
            return what;
        if (info.integers.empty())
            return {};
        for (const std::int32_t allowed : info.integers)
        {
            if (allowed == *number)
                return {};
        }
        return what;
    }
    case NonlinearHarmonicSolverParamsKind::Real:
    {
        const double* number = std::get_if<double>(&value);
        return number != nullptr && std::isfinite(*number) ? std::string() : what;
    }
    case NonlinearHarmonicSolverParamsKind::Word:
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
    }
    return what;
}

} // namespace oor::model
