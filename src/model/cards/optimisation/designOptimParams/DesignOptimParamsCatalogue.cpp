#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsCatalogue.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <array>

namespace oor::model
{

namespace
{

// The parameters of DOPTPRM, typed by hand. A parameter whose default
// depends on something else (the objective of the first cycle, the size of the model) has no default here.
constexpr DesignOptimParamsKind INT = DesignOptimParamsKind::Integer;
constexpr DesignOptimParamsKind REAL = DesignOptimParamsKind::Real;
constexpr DesignOptimParamsKind WORD = DesignOptimParamsKind::Word;

constexpr auto CATALOGUE = std::to_array<DesignOptimParamsInfo>({
    {"ApproximationMethod", INT, true, 2.0,
     "approximation method: 1 direct linearisation, 2 mixed method, 3 convex linearisation"},
    {"AutomaticSuperelementCreation", INT, true, 0.0, "1 starts an AESO creation run"},
    {"RelativeObjectiveConvergence", REAL, true, 0.001, "relative convergence criterion of the objective"},
    {"AbsoluteObjectiveConvergence", REAL, true, 1.0e-20, "absolute convergence criterion of the objective"},
    {"DesignVarConvergence", REAL, true, 0.001,
     "relative convergence criterion of the design variables (0.0001 for topology)"},
    {"PropertyConvergence", REAL, true, 0.001, "relative convergence criterion of the properties"},
    {"ConstraintActiveThreshold", REAL, true, -0.03, "constraint tolerance: a constraint above it is active"},
    {"ConstraintViolationThreshold", REAL, true, 0.003, "a constraint above it is violated"},
    {"AbsoluteObjectiveChange", REAL, false, 0.0,
     "computed: the larger of 0.001*|F0| and 0.0001, F0 the first objective"},
    {"FiniteDifferenceStep", REAL, true, 0.0001,
     "relative step of the finite differences (0.001 for fatigue responses)"},
    {"RelativeObjectiveChange", REAL, true, 0.001, "maximum relative change of the objective between two cycles"},
    {"PropertyMoveLimit", REAL, true, 0.2, "largest relative change of a property in one cycle"},
    {"DesignVarMoveLimit", REAL, true, 0.5,
     "largest relative change of a design variable in one cycle (0.2 for topology and topography)"},
    {"EquivalentLoadMoveLimit", REAL, true, 0.5, "largest relative change of a design variable in the ESLNRO loop"},
    {"MaxDesignCycles", INT, true, 5.0, "maximum number of design cycles (30 for topology, topography and topometry)"},
    {"DiscreteMethod", INT, true, 1.0, "discrete processing method: 0 none, 1 design of experiments, 2, 3, 4"},
    {"DiscreteStartCycle", INT, true, 0.0, "the cycle that starts the discrete processing (0: the last)"},
    {"MaxPropertyChange", REAL, true, 0.5, "trust region: largest relative change of a designed property"},
    {"MinPropertyChange", REAL, true, 0.01, "smallest move limit"},
    {"SuperelementStopRatio", REAL, true, 0.1, "threshold that turns off an active AESO job"},
    {"EquivalentLoadMaxCycles", INT, true, 20.0, "maximum number of design cycles of the ESLNRO loop"},
    {"MaxDesignVarChange", REAL, true, 1.0, "trust region: largest relative change of a design variable"},
    {"MinDesignVarChange", REAL, true, 0.05,
     "smallest move limit of a design variable (1.0E-5 for topology and topography)"},
    {"TrustRegionLowRatio", REAL, true, 0.01, "cutting ratio 1 of the trust region method"},
    {"TrustRegionMiddleRatio", REAL, true, 0.25, "cutting ratio 2 of the trust region method"},
    {"TrustRegionHighRatio", REAL, true, 0.7, "cutting ratio 3 of the trust region method"},
    {"FullyStressedRelaxation", REAL, true, 0.9, "relaxation of the fully stressed design"},
    {"FullyStressedMaxCycles", INT, true, 0.0, "number of fully stressed design cycles"},
    {"MaxConstraintViolation", REAL, true, 0.005, "violation still accepted at the optimum"},
    {"ConstraintScale", REAL, true, 0.001, "constraint normalisation factor"},
    {"ViolationTreatment", INT, true, 0.0, "number of gradients computed besides the active and violated constraints"},
    {"OptimizerPrintLevel", INT, true, 0.0, "how much the approximate optimisation prints: 0 to 7"},
    {"DesignVarScaling", INT, true, 0.0, "design variable scaling: -1 none, 0 or more scaling"},
    {"OptimizationMethod", INT, true, 0.0,
     "optimisation method: 0 automatic, 1 to 4, or IJK (strategy, optimiser and search)"},
    {"PrintAnalysisEveryCycle", INT, true, 0.0,
     "1 prints the analysis results at every design cycle (0: at the first and the last only)"},
    {"ObjectiveRelativeToStart", INT, true, 0.0, "1 measures the objective from its value at the start (0 there)"},
    {"OptimizerName", WORD, false, 0.0, "the name of the optimiser to use (blank: the default optimiser)"},
    {"PrintInterval", INT, true, 0.0, "print every n-th design cycle"},
    {"PrintItems", INT, true, 1.0, "items to print: the sum of 1, 2, 4, 8 and 16"},
    {"PrintMaxRetained", INT, false, 0.0, "constraints of all categories to print (default: all)"},
    {"PrintBeadConstraints", INT, false, 0.0, "constraints on beam library dimensions to print (default: all)"},
    {"PrintConstraintCount", INT, false, 0.0, "constraints on connectivity properties to print (default: all)"},
    {"PrintDiscreteVars", INT, false, 0.0, "constraints on dependent design variables to print (default: all)"},
    {"PrintMaterialCount", INT, false, 0.0, "constraints on material properties to print (default: all)"},
    {"PrintPropertyCount", INT, false, 0.0, "constraints on element properties to print (default: all)"},
    {"PrintResponseCount", INT, false, 0.0, "constraints on design responses to print (default: all)"},
    {"PrintResponseSet", INT, false, 0.0,
     "the id of a set of the retained responses to print (default: those of the printed constraints)"},
    {"PrintApproximation", INT, false, 0.0, "1 prints the design variables and properties of a topology optimisation"},
    {"PenaltyFactor", REAL, true, 0.0, "penalty that turns an infeasible approximate problem feasible"},
    {"PropertyLimitViolation", INT, true, 0.0, "nonzero ignores a property limit violation with a warning"},
    {"PropertyTolerance", REAL, true, 1.0e35,
     "tolerance between the property values of the cards and those of the design variables"},
    {"StepScale", REAL, true, 1.0, "scaling factor of the finite difference step of the shape variables"},
    {"TopologyCheckerboard", INT, true, -1.0,
     "topology checkerboarding control: -1 automatic, 0 none, 1 filter, 2 density constraint"},
    {"TopologyMinMember", REAL, false, 0.0, "minimum diameter of members in a topology optimisation"},
    {"TopologyRegion", INT, true, 0.0, "1 turns the trust region method on"},
    {"TrustRegionGrowthFactor", REAL, true, 2.0, "factor that enlarges the trust region after a good approximation"},
    {"TrustRegionShrinkFactor", REAL, true, 0.5, "factor that shrinks the trust region after a poor approximation"},
});

} // namespace

std::span<const DesignOptimParamsInfo> designOptimParamsCatalogue() noexcept
{
    return CATALOGUE;
}

const DesignOptimParamsInfo* findDesignOptimParam(std::string_view name) noexcept
{
    for (const DesignOptimParamsInfo& info : CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

std::string_view designOptimParamsKindName(DesignOptimParamsKind kind) noexcept
{
    switch (kind)
    {
    case DesignOptimParamsKind::Integer:
        return "integer";
    case DesignOptimParamsKind::Real:
        return "real";
    case DesignOptimParamsKind::Word:
        break;
    }
    return "text";
}

bool designOptimParamsValueFits(const DesignOptimParamsInfo& info, const DesignValue& value) noexcept
{
    switch (info.kind)
    {
    case DesignOptimParamsKind::Integer:
        return value.isInteger();
    case DesignOptimParamsKind::Real:
        return value.isInteger() || value.isReal();
    case DesignOptimParamsKind::Word:
        break;
    }
    return value.isWord();
}

} // namespace oor::model
