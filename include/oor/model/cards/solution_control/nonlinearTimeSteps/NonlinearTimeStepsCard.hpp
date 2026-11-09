#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// METHOD of a nonlinear transient analysis: how the stiffness is updated and the time integrated. The default depends
// on the solution (ADAPT for SOLs 129 and 159, AUTO or FNT for SOL 400): a blank is kept as "not given".
enum class NonlinearTransientMethod : std::uint8_t
{
    NotGiven, // blank
    Auto,     // AUTO
    Iter,     // ITER
    Adapt,    // ADAPT
    Semi,     // SEMI
    Fnt,      // FNT
    Pfnt      // PFNT
};

// One TSTEPNL card: the controls of a nonlinear transient analysis (selected by TSTEP in Case Control, or TSTEPNL).
//
// Card layout:
//   TSTEPNL  ID     NDT     DT     NO      METHOD  KSTEP  MAXITER  CONV
//            EPSU   EPSP    EPSW   MAXDIV  MAXQN   MAXLS  FSTRESS
//            MAXBIS ADJUST  MSTEP  RB      MAXR    UTOL   RTOLB    MINITER
// A blank field takes the default of the format reference, which sometimes depends on the solution sequence (SOLs 129
// and 159 or SOL 400), that the card does not know: those fields are kept "not given" when blank (KSTEP and MAXITER
// when METHOD is not given, EPSP, EPSW, MSTEP, CONV, MINITER, and MAXBIS when MAXITER is negative). A default that
// follows from this card's own METHOD or MAXITER is stored resolved and always written (KSTEP and MAXITER from an
// explicit METHOD, EPSU, MAXQN and MAXLS from PFNT, MAXBIS from MAXITER); the other defaults do not depend on anything
// and are left blank when written.
struct NonlinearTimeStepsCard
{
    base::EntityId id = 0;
    std::int32_t stepCount = 0;      // the number of time steps of DT
    double timeStep = 0.0;           // the time increment
    std::int32_t outputInterval = 1; // every NO-th step is output
    NonlinearTransientMethod method = NonlinearTransientMethod::NotGiven;
    std::optional<std::int32_t> stiffnessUpdateStep;
    std::optional<std::int32_t> maxIterations;
    std::string
        convergenceFlags; // the convergence flags (U, P, W, V, N, D in any combination), upper case; empty when blank
    double epsilonU = 1.0E-2;
    std::optional<double> epsilonP;
    std::optional<double> epsilonW;
    std::int32_t maxDivergences = 2;
    std::int32_t maxQuasiNewtonVectors = 10;
    std::int32_t maxLineSearches = 2;
    double stressSubincrementFraction = 0.2;
    std::optional<std::int32_t> maxBisections;
    std::int32_t adjustInterval = 5;
    std::optional<std::int32_t> minStepsPerCycle;
    double stepBounds = 0.6;
    double maxRotationRatio = 32.0;
    double displacementTolerance = 0.1;
    double rotationTolerance = 20.0;
    std::optional<std::int32_t> minIterations;
};

// The defaults that follow from the card's own fields (see NonlinearTimeStepsCard).
[[nodiscard]] inline std::optional<std::int32_t> nonlinearTimeStepsDefaultStiffnessUpdateStep(
    NonlinearTransientMethod method) noexcept
{
    if (method == NonlinearTransientMethod::Adapt)
        return 2;
    if (method == NonlinearTransientMethod::Iter)
        return 10;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::int32_t> nonlinearTimeStepsDefaultMaxIterations(
    NonlinearTransientMethod method) noexcept
{
    if (method == NonlinearTransientMethod::NotGiven)
        return std::nullopt;
    return method == NonlinearTransientMethod::Adapt ? 10 : 25;
}

[[nodiscard]] inline double nonlinearTimeStepsDefaultEpsilonU(NonlinearTransientMethod method) noexcept
{
    return method == NonlinearTransientMethod::Pfnt ? -1.0E-2 : 1.0E-2;
}

[[nodiscard]] inline std::int32_t nonlinearTimeStepsDefaultMaxQuasiNewtonVectors(
    NonlinearTransientMethod method) noexcept
{
    return method == NonlinearTransientMethod::Pfnt ? 0 : 10;
}

[[nodiscard]] inline std::int32_t nonlinearTimeStepsDefaultMaxLineSearches(NonlinearTransientMethod method) noexcept
{
    return method == NonlinearTransientMethod::Pfnt ? 0 : 2;
}

// 5 bisections, except that a negative MAXITER (the solution continues to the end of the step) has its own default
// in SOL 400 and 5 in SOL 129: not given.
[[nodiscard]] inline std::optional<std::int32_t> nonlinearTimeStepsDefaultMaxBisections(
    const std::optional<std::int32_t>& maxIterations) noexcept
{
    if (maxIterations && *maxIterations < 0)
        return std::nullopt;
    return 5;
}

} // namespace oor::model
