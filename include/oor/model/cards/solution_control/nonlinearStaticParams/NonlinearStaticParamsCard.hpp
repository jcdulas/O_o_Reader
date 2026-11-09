#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// KMETHOD of a nonlinear static analysis: how the stiffness matrix is updated. The default depends on the solution
// (AUTO for SOL 106, AUTO or FNT for SOL 400): a blank is kept as "not given".
enum class StiffnessMethod : std::uint8_t
{
    NotGiven, // blank
    Auto,     // AUTO
    Iter,     // ITER
    Semi,     // SEMI
    Fnt,      // FNT (SOL 400)
    Pfnt      // PFNT (SOL 400)
};

// One NLPARM card: the iteration strategy of a nonlinear static analysis (selected by NLPARM in Case Control).
//
// Card layout:
//   NLPARM  ID     NINC    DT     KMETHOD  KSTEP  MAXITER  CONV   INTOUT
//           EPSU   EPSP    EPSW   MAXDIV   MAXQN  MAXLS    FSTRESS LSTOL
//           MAXBIS                         MAXR           RTOLB   MINITER
// A blank field takes the default of the format reference. The defaults that depend on the solution sequence, which the
// card does not know, stay "not given" (NINC, KMETHOD, KSTEP, CONV, MINITER), and so do the tolerances EPSU, EPSP and
// EPSW: when they are blank the tolerances of PARAM,NLTOL apply, which a value
// given, even the default 1.0E-2, overrides. A default that follows from this card's own KMETHOD or MAXITER is stored
// resolved and always written (MAXQN, MAXLS, MAXBIS); the other defaults do not depend on anything and are left blank
// when written. INTOUT is YES, NO, ALL or (SOL 400) a positive integer, kept as text.
struct NonlinearStaticParamsCard
{
    base::EntityId id = 0;
    std::optional<std::int32_t> incrementCount; // the number of increments
    double timeStep = 0.0;                      // the time interval of a creep analysis (0.0: no creep)
    StiffnessMethod stiffnessMethod = StiffnessMethod::NotGiven;
    std::optional<std::int32_t> stiffnessUpdateStep;
    std::int32_t maxIterations = 25;
    std::string
        convergenceFlags; // the convergence flags (U, P, W, V, N, A in any combination), upper case; empty when blank
    std::string intermediateOutput = "No";
    std::optional<double> epsilonU; // the displacement, load and work tolerances
    std::optional<double> epsilonP;
    std::optional<double> epsilonW;
    std::int32_t maxDivergences = 3;
    std::int32_t maxQuasiNewtonVectors = 25;
    std::int32_t maxLineSearches = 4;
    double stressSubincrementFraction = 0.2;
    double lineSearchTolerance = 0.5;
    std::int32_t maxBisections = 5;
    double maxRotationRatio = 20.0;
    double rotationTolerance = 20.0;
    std::optional<std::int32_t> minIterations;
};

// The defaults that follow from the card's own fields (see NonlinearStaticParamsCard).
// MAXITER, whose sign (the solution continues to the end of the step, SOL 400) does not count here.
[[nodiscard]] inline std::int32_t nonlinearStaticParamsDefaultMaxQuasiNewtonVectors(StiffnessMethod method,
                                                                                    std::int32_t maxIterations) noexcept
{
    return method == StiffnessMethod::Pfnt ? 0 : (maxIterations < 0 ? -maxIterations : maxIterations);
}

[[nodiscard]] inline std::int32_t nonlinearStaticParamsDefaultMaxLineSearches(StiffnessMethod method) noexcept
{
    return method == StiffnessMethod::Pfnt ? 0 : 4;
}

[[nodiscard]] inline std::int32_t nonlinearStaticParamsDefaultMaxBisections(std::int32_t maxIterations) noexcept
{
    return maxIterations < 0 ? 0 : 5;
}

} // namespace oor::model
