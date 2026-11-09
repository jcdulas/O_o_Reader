#include "ParameterCatalogueData.hpp"

namespace oor::model
{

namespace
{

constexpr ParamDefault noDefault() noexcept
{
    return {};
}

constexpr ParamDefault integerDefault(std::int32_t value) noexcept
{
    ParamDefault result;
    result.kind = ParameterKind::Integer;
    result.integerValue = value;
    return result;
}

constexpr ParamDefault realDefault(double value) noexcept
{
    ParamDefault result;
    result.kind = ParameterKind::Real;
    result.realValue = value;
    return result;
}

constexpr ParamDefault textDefault(std::string_view value) noexcept
{
    ParamDefault result;
    result.kind = ParameterKind::Text;
    result.textValue = value;
    return result;
}

constexpr ParamDefault complexDefault(double real, double imaginary) noexcept
{
    ParamDefault result;
    result.kind = ParameterKind::Complex;
    result.realValue = real;
    result.imaginaryValue = imaginary;
    return result;
}

} // namespace

// The names of the chapter "Parameters" of the format reference, in its order, typed by hand
// from the chapter. Per name: the kind of value, the default, and the options when the entry lists them as the
// complete set.
//
// Conventions of the typing:
// - The kind is the one the entry states; when it states none, the one its default and its description show.
//   `ParamType::Number` is a value the format reference writes as a number without saying integer or real;
//   `ParamType::Any` is a parameter the format reference only says was replaced by a Case Control command, or whose
//   value kind depends on the use.
// - A heading that lists several names ("W3, W4, W3FL, W4FL") gives one row each, with its own default. The heading
//   "ESLRCF, user_rc_file" names a placeholder as its second item: it has no row. In "XYUNIT, n" and "SDUNIT, n" the n
//   is the unit number given as the value. The two defaults of "ALPHA1, ALPHA2" are the real and imaginary parts of
//   each (see the rows).
// - `noDefault()` is a default the format reference does not give as a value (computed by the program, equal to another
// parameter,
//   a blank string).
// - When the default depends on the solution or the element type, the row has the general default and a comment
//   starting with AMBIGUOUS says what differs. The other AMBIGUOUS comments are entries whose kind or options the text
//   does not settle.
// - The options are filled only when the entry lists them as the complete set (a table of values, or the pair YES and
//   NO with both words in the entry); an entry that only gives a range, "see another parameter" or values that add up
//   has none.
const std::vector<ParamInfo>& parameterCatalogueEntries()
{
    static const std::vector<ParamInfo> ENTRIES = {
        {"AcousticExteriorMethod", ParamType::Text, noDefault(), "Iterative|Krylov"},
        {"AcousticExteriorSet", ParamType::Integer,
         noDefault()}, // AMBIGUOUS: kind not stated, inferred (id of an ITER entry)
        {"AcousticModalParticipation", ParamType::Integer, integerDefault(-1)},
        {"AcousticOutputKind", ParamType::Text, textDefault("Peak"), "Peak|RootMeanSquare"},
        {"AcousticWeakCoupling", ParamType::Text, textDefault("No")},
        {"AcousticSymmetricForm", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"TrimComponentDamping", ParamType::Integer, integerDefault(0)},
        {"TrimComponentThreads", ParamType::Integer, integerDefault(0)},
        {"TrimComponentMemory", ParamType::Integer, integerDefault(0)},
        {"AdjointMethod", ParamType::Integer, integerDefault(0), "0|1|2"},
        {"AdmittanceExternalUnit", ParamType::Integer, integerDefault(0)},
        {"AdmittancePostOutput", ParamType::Integer, integerDefault(0), "0|1|2"},
        {"AdaptiveConvergence", ParamType::Real, realDefault(1.0)},
        {"StaticDifferentialStiffness", ParamType::Text, textDefault("Yes")},
        {"AeroDofOffset", ParamType::Integer, integerDefault(35)},
        {"AeroStaticDiscardTolerance", ParamType::Real, realDefault(1.0e-8)},
        {"AeroStaticMaxIterations", ParamType::Integer, integerDefault(15)},
        {"AeroStaticMethod", ParamType::Text, textDefault("Select"), "Select|Automatic|Direct|RitzVectors|Iterative"},
        {"AeroStaticRandomSeed", ParamType::Integer, integerDefault(2)},
        {"AeroStaticTolerance", ParamType::Real, realDefault(1.0e-10)},
        {"AggregateRotations", ParamType::Text, textDefault("Yes"), "Yes|No"},
        // The two defaults of "ALPHA1, ALPHA2" are the real and imaginary part of each: a deck gives
        // PARAM,ALPHA1,0.0,0.0 and PARAM,ALPHA2,0.0,0.0 (the FL pair is read the same way, by its likeness)
        {"RayleighMassDamping", ParamType::Complex, complexDefault(0.0, 0.0)},
        {"RayleighStiffnessDamping", ParamType::Complex, complexDefault(0.0, 0.0)},
        {"RayleighMassDampingFluid", ParamType::Complex,
         complexDefault(0.0, 0.0)}, // AMBIGUOUS: read like ALPHA1, no deck shows it
        {"RayleighStiffnessDampingFluid", ParamType::Complex,
         complexDefault(0.0, 0.0)}, // AMBIGUOUS: read like ALPHA2, no deck shows it
        {"AlternateReduction", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ArbitrarySectionAspectRatio", ParamType::Number,
         integerDefault(2)}, // AMBIGUOUS: an aspect ratio whose default is written as the integer 2
        {"ArbitrarySectionByElements", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"ArbitrarySectionImmediate", ParamType::Integer, integerDefault(0), "0|1"},
        {"ArbitrarySectionPrint", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"ArbitrarySectionShear", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ArbitrarySectionShearKind", ParamType::Text, textDefault("TimoshenkoBeam"), "TimoshenkoBeam|TimoshenkoForce"},
        {"ReductionFactorFluid", ParamType::Real, realDefault(0.95)},
        {"ReductionFactorStructure", ParamType::Real, realDefault(0.95)},
        {"AcousticStructureCoupling", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"AutoSingularityAction", ParamType::Integer, integerDefault(0), "-1|0"},
        {"AccelUnitsFactor", ParamType::Real, realDefault(1.0)},
        {"AutoAdjointChoice", ParamType::Text, textDefault("Yes"), "Yes|No|NoStaticAdjust|NoFrequencyAdjust"},
        {"AutoGlueOutput", ParamType::Text, textDefault("No"), "Yes|No"},
        {"AutoDependentSet", ParamType::Text, textDefault("No"),
         "No|Yes|LowerUpperSolution|Automatic"}, // AMBIGUOUS: default differs in SOL 400 statics
        {"AutoGeneralizedSet", ParamType::Text, textDefault("No"), "Yes|No"},
        {"AutoSingularityConstr", ParamType::Text, textDefault("Yes")},
        {"AutoSingularityConstrNonlinear", ParamType::Text, textDefault("No"), "Yes|No"},
        {"AutoFreeBodySupport", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"SingularityBailOut", ParamType::Integer, integerDefault(0)},
        {"BeamBendingAreaRatio", ParamType::Real, realDefault(1000.0)},
        {"BucklingEigenReduction", ParamType::Text, textDefault("Yes")},
        {"LargeEnergyThreshold", ParamType::Real, realDefault(0.0)},
        {"LargeEnergyThreshold1", ParamType::Real, realDefault(0.0)},
        {"LargeEnergyThreshold2", ParamType::Real, realDefault(0.0)},
        {"LargeEnergyThreshold3", ParamType::Real, realDefault(0.0)},
        {"BucklingRestart", ParamType::Integer, integerDefault(-1), "-1|1|2"},
        {"BushNormalization", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"BandwidthReorder", ParamType::Text, textDefault("No"), "Yes|No"},
        {"IterativeAcousticMassEstimate", ParamType::Text, textDefault("No"), "Yes|No"},
        {"IterativeAcousticMemoryEstimate", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"IterativeAcousticPivot", ParamType::Real, realDefault(1.0e-10)},
        {"MassScaleFactor", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"DampingScaleFactor", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"CentralDifferenceSensitivity", ParamType::Text, textDefault("Yes"),
         "Yes|No"}, // AMBIGUOUS: default depends on the optimization type
        {"ConstraintIterations", ParamType::Integer, integerDefault(0)},
        {"ConstraintPunch", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ConstraintPrint", ParamType::Text, textDefault("Yes")},
        {"FastenerDiagnosticPrint", ParamType::Text, textDefault("No"), "Yes|No"},
        {"FastenerRandomDelete", ParamType::Real, realDefault(0.0)},
        {"ModelCheckout", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ConditionCheck", ParamType::Any,
         textDefault("Yes")}, // AMBIGUOUS: default is YES but the text also speaks of a numeric threshold
        {"StiffnessScale1", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"StiffnessScale2", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"StiffnessScale3", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: kind inferred from the (re, im) default
        {"CleanVectors", ParamType::Text, textDefault("No"), "Yes|No"},
        {"CleanVectorTolerance", ParamType::Real, realDefault(0.01)},
        {"ClosenessRatio", ParamType::Real, realDefault(1.0)},
        {"MassScale1", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"MassScale2", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"CompositeMatTemperature", ParamType::Text, textDefault("No"), "No|Yes|NonSmeared"},
        {"ConnectionTolerance", ParamType::Real, realDefault(1.0e-5)},
        {"CouplingSubcycles", ParamType::Integer, integerDefault(1)},
        {"CouplingSubcyclesMax", ParamType::Integer, integerDefault(0)},
        {"CoupledMass", ParamType::Integer, integerDefault(-1)},
        {"LoadScale1", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"LoadScale2", ParamType::Complex,
         complexDefault(1.0, 0.0)}, // AMBIGUOUS: (1.0, 0.0) read as one complex default per name
        {"CompleteQuadraticCombination", ParamType::Integer, integerDefault(-1)},
        {"CurvatureStress", ParamType::Integer, integerDefault(-1)},
        {"CurvePlotRequest", ParamType::Integer, integerDefault(-1)},
        {"WeldDiagnosticPrint", ParamType::Text, textDefault("No"), "Yes|No"},
        {"WeldRandomDelete", ParamType::Real, realDefault(0.0)},
        {"DampingTolerance", ParamType::Real, realDefault(1.0e8)},
        {"DatabaseAllSets", ParamType::Text, textDefault("DBALL")},
        {"DatabaseConversion", ParamType::Text, textDefault("XL")},
        {"DatabaseConversionDiagnostic", ParamType::Integer, integerDefault(0)},
        {"DatabaseConversionOverwrite", ParamType::Text, textDefault("Yes")},
        {"DatabaseDictionaryPrint", ParamType::Integer, integerDefault(-1)},
        {"DatabaseDownstream", ParamType::Text, noDefault()},
        {"DatabaseRecoveryProject", ParamType::Integer, integerDefault(0)},
        {"DatabaseRecoveryVersion", ParamType::Integer, integerDefault(0)},
        {"DatabaseExternal", ParamType::Text, noDefault()},
        {"DatabaseRecovery", ParamType::Text, noDefault()},
        {"DatabaseUpstream", ParamType::Text, noDefault()},
        {"ModalRecoveryMatrixMethod", ParamType::Integer, integerDefault(0), "-1|0"},
        {"ClumpDeltaFactor", ParamType::Real, realDefault(0.5)},
        {"DesignPunchOutput", ParamType::Integer, integerDefault(0)},
        {"DesignPunchOutput1", ParamType::Integer, integerDefault(6)},
        {"FreqDuplicateTolerance", ParamType::Real, realDefault(1.0e-5)},
        {"DirectOutputFile", ParamType::Text, textDefault("No")},
        {"DesignOptimOutput", ParamType::Integer, integerDefault(0), "0|1|2|3|4"},
        {"DesignPerturbationEpsilon", ParamType::Real, realDefault(1.0e-4)},
        {"DesignPhaseFlag", ParamType::Integer, integerDefault(0), "0|1|2|3"},
        {"SensitivityWithoutDifferentialStiffness", ParamType::Real, realDefault(0.0)},
        {"SensitivityOutput", ParamType::Text, noDefault()},
        {"SensitivityZero", ParamType::Real, realDefault(0.0)},
        {"DesignThirdPass", ParamType::Integer, integerDefault(1)},
        {"DynamicConstraintForces", ParamType::Text, textDefault("New"), "New|Old"},
        {"EigenFilter", ParamType::Real, realDefault(1.0e-13)},
        {"EnforcedMotionMethod", ParamType::Text, textDefault("Total"),
         "Total|Absolute|Relative"}, // AMBIGUOUS: default given as TOTAL or ABS (same meaning)
        {"EnforcedMotionKind", ParamType::Text, textDefault("Total"),
         "Total|Absolute|Relative"}, // AMBIGUOUS: default given as TOTAL or ABS (same meaning)
        {"ElemPlotPrint", ParamType::Real, realDefault(1.0e-8)},
        {"EpsilonTemperature", ParamType::Text, textDefault("Secant"), "Secant|Integral"},
        {"PivotZeroThreshold", ParamType::Real, realDefault(1.0e-8)},
        {"RadiatedPowerSpeedOfSound", ParamType::Real, realDefault(1.0)},
        {"RadiatedPowerReferenceDecibel", ParamType::Real, realDefault(1.0)},
        {"RadiatedPowerDensity", ParamType::Real, realDefault(1.0)},
        {"RadiatedPowerLossFactor", ParamType::Real, realDefault(1.0)},
        {"ErrorHandling", ParamType::Integer, integerDefault(-1), "-1|0"},
        {"EquivalentLoadSaveFiles", ParamType::Text, textDefault("No"), "Yes|No"},
        {"EquivalentLoadMoveLimit", ParamType::Integer, integerDefault(0), "0|1"},
        {"EquivalentLoadCombination", ParamType::Text, textDefault("No"), "Yes|No"},
        {"EquivalentLoadConstraintOption", ParamType::Integer, integerDefault(0)},
        {"EquivalentLoadExit", ParamType::Integer, integerDefault(0), "0|1|2|3"},
        {"EquivalentLoadPrint", ParamType::Integer, integerDefault(0)},
        {"EquivalentLoadPrint1", ParamType::Integer, integerDefault(7)},
        {"EquivalentLoadResponseFactor", ParamType::Text,
         noDefault()}, // AMBIGUOUS: default is blank; heading also lists the placeholder user_rc_file, no row for it
        {"EquivalentLoadTopologyConvergence", ParamType::Integer, integerDefault(80)},
        {"EquivalentLoadUnit2", ParamType::Integer, integerDefault(54)},
        {"EquivalentLoadUnit1", ParamType::Integer, integerDefault(53)},
        {"ElemStrainEnergyThreshold", ParamType::Any,
         noDefault()}, // AMBIGUOUS: no kind or default, entry only says it was replaced by a Case Control command
        {"EulerBoundary", ParamType::Text, textDefault("Element"), "Extrapolate|Element"},
        {"EulerStress", ParamType::Text, textDefault("Volume"), "Volume|Mass"},
        {"EulerSubcycles", ParamType::Integer, integerDefault(1)},
        {"ExcludedSuperelements", ParamType::Integer, integerDefault(0)},
        {"ExternalDataRecovery", ParamType::Any, noDefault()},       // AMBIGUOUS: deprecated, no kind or default given
        {"ExternalDataRecoveryOutput", ParamType::Any, noDefault()}, // AMBIGUOUS: deprecated, no kind or default given
        {"ExternalDataRecoveryUnit", ParamType::Any, noDefault()},   // AMBIGUOUS: deprecated, no kind or default given
        {"ExternalSuperelementOutput", ParamType::Any, noDefault()}, // AMBIGUOUS: deprecated, no kind or default given
        {"ExternalRecovery", ParamType::Any, noDefault()},           // AMBIGUOUS: deprecated, no kind or default given
        {"ExternalUnit", ParamType::Integer, integerDefault(30)},
        {"SensitivityPerturbation", ParamType::Real, realDefault(0.0)},
        {"KeyFactor", ParamType::Integer, integerDefault(10000)},
        {"FastFreqResponse", ParamType::Text, textDefault("Automatic"), "Automatic|Yes|No"},
        {"FrfAssemblyTolerance", ParamType::Real, realDefault(1.0e-5)},
        {"FrfBlending", ParamType::Real, realDefault(0.6667)},
        {"FrfDataRecoveryLoads", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"FirstIterationStiffness", ParamType::Integer, noDefault()},
        {"FixedBoundaryOnly", ParamType::Integer,
         integerDefault(0)}, // AMBIGUOUS: format text mixes a 0-to-1 blending fraction with a -2/-1/1 option list; one
                             // option label is garbled, so the list is left open
        {"FollowerStiffnessSymmetryFactor", ParamType::Real, realDefault(0.024)},
        {"FlexibleIncrements", ParamType::Text, textDefault("No"), "Yes|No"},
        {"FluidModesRequested", ParamType::Any,
         noDefault()}, // AMBIGUOUS: replaced by a Case Control command, no kind or default given
        {"FluidNodeExtract", ParamType::Integer, integerDefault(500)},
        {"FluidSuperelement", ParamType::Integer, integerDefault(0)},
        {"FluidMultipleModes", ParamType::Real, realDefault(0.1)},
        {"FollowerStiffness", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"FreqDependentOutput", ParamType::Text, textDefault("No"), "Yes|No"},
        {"FreqTolerance", ParamType::Real, realDefault(1.0e-8)},
        {"FullSuperelementRecovery", ParamType::Text, textDefault("No"), "Yes|No"},
        {"FreqZeroThreshold", ParamType::Real, realDefault(1.0)},
        {"UniformStructuralDamping", ParamType::Real, realDefault(0.0)},
        {"FluidStructuralDamping", ParamType::Real, realDefault(0.0)},
        {"GeometryUnit", ParamType::Integer, integerDefault(40)},
        {"NodeElemConnectivity", ParamType::Integer, integerDefault(-1), "-1|1"},
        {"GradientMesh", ParamType::Text, textDefault("Off"), "Off|MinimumVolume|ElemNumbering"},
        {"GravitySet", ParamType::Integer, integerDefault(0)},
        {"WeightReferenceNode", ParamType::Integer, integerDefault(-1)},
        {"GustAeroOption", ParamType::Integer, integerDefault(1)},
        {"GyroscopicAveraging", ParamType::Integer, integerDefault(0)},
        {"GyroscopicFatalCheck", ParamType::Integer, integerDefault(-1)},
        {"HeatStaticAnalysis", ParamType::Text, textDefault("No"), "Yes|No"},
        {"HighFreqLimit", ParamType::Real, realDefault(1.0e30)},
        {"HighFreqLimitFluid", ParamType::Real, realDefault(1.0e30)},
        {"HeatCouplingIterations", ParamType::Integer, integerDefault(20)},
        {"HeatCouplingPrint", ParamType::Text, textDefault("No"), "Yes|No"},
        {"HeatCouplingTolerance", ParamType::Real, realDefault(1.0e-2)},
        {"HeatSymmetry", ParamType::Integer, integerDefault(0), "0|1"},
        {"InitialConditionOption", ParamType::Integer, integerDefault(1)},
        {"InputProcessorDebug", ParamType::Text,
         noDefault()}, // AMBIGUOUS: default is the value of DBALL, kind inferred from that
        {"InertiaRelief", ParamType::Integer, integerDefault(0), "-4|-2|-1|0"},
        {"InertiaReliefMethod", ParamType::Any,
         noDefault()}, // AMBIGUOUS: replaced by a Case Control keyword, no kind or default given
        {"ResidualOutput", ParamType::Integer, integerDefault(-1)},
        {"InitialTape", ParamType::Integer, integerDefault(-1)},
        {"InputUnit", ParamType::Integer, integerDefault(11)},
        {"EigenvalueLock", ParamType::Integer, integerDefault(0), "0|1"},
        {"JonesWilkinsLeeDetonation", ParamType::Text, textDefault("NoLink"), "NoLink|Link"},
        {"ModalDampingAsStiffness", ParamType::Integer, integerDefault(1)},
        {"ModalDampingAsStiffnessFluid", ParamType::Integer, integerDefault(1)},
        {"StiffnessDiagonalAdd", ParamType::Real,
         realDefault(-1.0)}, // AMBIGUOUS: default depends on solution (0.0 for SOL 400 with contact)
        {"ModalDampingFilter", ParamType::Integer, integerDefault(0), "0|1|2"},
        {"RitzStructuralDamping", ParamType::Integer, integerDefault(0)},
        {"ShellDrillingStiffness", ParamType::Real, realDefault(100.0)},
        {"LargeRotationMethod", ParamType::Integer, integerDefault(1), "1|2"},
        {"LoadSummary", ParamType::Integer, integerDefault(0), "0|1|2|3|4"},
        {"LowFreqLimit", ParamType::Real, realDefault(0.0)},
        {"LowFreqLimitFluid", ParamType::Real, realDefault(0.0)},
        {"LargeDisplacements", ParamType::Integer,
         integerDefault(-1)}, // AMBIGUOUS: option list includes a "> 0" case, so the list is left open
        {"LagrangeScaleFactor", ParamType::Real,
         realDefault(1.0e5)}, // AMBIGUOUS: computed by the program for SOL 400 and SOL 101 contact
        {"LowestModesCount", ParamType::Integer, integerDefault(0)},
        {"LowestModesCountFluid", ParamType::Integer, integerDefault(0)},
        {"LoadUnit", ParamType::Integer, integerDefault(-1)},
        {"LoopIdentifier", ParamType::Integer, integerDefault(0)},
        {"LayerStrainOutput", ParamType::Any,
         noDefault()}, // AMBIGUOUS: replaced by a Case Control command, no kind or default given
        {"MachNumber", ParamType::Real, realDefault(0.0)},
        {"MaxAcceptedRatio", ParamType::Real, realDefault(1.0e9)},
        {"MaxAppliedLoads", ParamType::Integer, integerDefault(1000)},
        {"MaxConditionNumber", ParamType::Real, realDefault(1.0e11)},
        {"MaxDampingRatio", ParamType::Integer, integerDefault(1000)},
        {"MaxLoops", ParamType::Integer, integerDefault(5)},
        {"MaxMechanisms", ParamType::Integer, integerDefault(24)},
        {"MaxDiagonalRatio", ParamType::Real, realDefault(1.0e7)},
        {"ModuleStructuralDampingOption", ParamType::Integer, integerDefault(1)},
        {"ModuleOption14", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"ModuleOutputTransferMatrix", ParamType::Text, textDefault("Automatic"), "Automatic|Yes|No"},
        {"ModuleOutputTransferFactor", ParamType::Integer, integerDefault(20)},
        {"ModuleReductionOption", ParamType::Integer, integerDefault(-1)},
        {"MechanismFilter", ParamType::Real, realDefault(1.0e-6)},
        {"MechanismFix", ParamType::Text, textDefault("Automatic"), "Automatic|Yes|No"},
        {"MechanismOutput", ParamType::Text, textDefault("Yes")},
        {"MechanismPrint", ParamType::Text, textDefault("No"), "Yes|No"},
        {"MeshPlot", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ResidualModeMethod", ParamType::Integer, integerDefault(0)},
        {"HarmonicReduction", ParamType::Text, textDefault("Yes")},
        {"MicroSeconds", ParamType::Integer, integerDefault(10)},
        {"MidNodeAdjust", ParamType::Integer, integerDefault(0), "0|1|2"},
        {"MinimumOmittedSet", ParamType::Text, textDefault("No"), "Yes|No"},
        {"MinOrthogonality", ParamType::Real, realDefault(0.9)},
        {"MinOrthogonalityRatio", ParamType::Real, realDefault(0.7)},
        {"ModalMassFilter", ParamType::Real, realDefault(1.0e-10)},
        {"ModalMassMethod", ParamType::Text, textDefault("Beta"), "Beta|New|Old"},
        {"ModeAcceleration", ParamType::Integer,
         integerDefault(-1)}, // AMBIGUOUS: option list includes a "> 0" case, so the list is left open
        {"ModelDisplay", ParamType::Integer, integerDefault(0)},
        {"ModulePartSet", ParamType::Text, textDefault("GlobalDofSet"),
         "FreeDofSet|GlobalDofSet"}, // AMBIGUOUS: default depends on solution (FreeDofSet in SOL 144)
        {"ConstraintExpansion", ParamType::Integer, integerDefault(0)},
        {"MonitorPointUnit", ParamType::Integer, integerDefault(0)},
        {"SuppressedMessage10625", ParamType::Integer, integerDefault(0), "0|1"},
        {"NonlinearHarmonicMaxNonConvergedSteps", ParamType::Integer, integerDefault(5)},
        {"MaxLagrangeMultipliers", ParamType::Real, realDefault(0.0)},
        {"DesignPrintFrequency", ParamType::Integer, integerDefault(0)},
        {"NumericalDamping", ParamType::Real,
         realDefault(
             0.01)}, // AMBIGUOUS: default depends on solution (-0.05 for SOL 400 non-contact, 0.0 for SOL 400 contact)
        {"NumericalDampingModal", ParamType::Real,
         realDefault(0.0)}, // AMBIGUOUS: default depends on solution (1.0 for SOL 400 contact)
        {"NewmarkIntegration", ParamType::Text, textDefault("No")},
        {"FiniteDifferenceOptions", ParamType::Integer, integerDefault(2), "1|2"},
        {"NonlinearHarmonicExtraSamplingPoints", ParamType::Integer, integerDefault(20)},
        {"InterpolationPoints", ParamType::Integer, integerDefault(10)},
        {"LayerCount", ParamType::Integer,
         integerDefault(5)}, // AMBIGUOUS: default depends on element type (7 for CQUADR, 3 for CAXISYM)
        {"NonlinearHeatLineSearch", ParamType::Integer, integerDefault(0)},
        {"NonlinearHarmonicTolerance", ParamType::Real, realDefault(1.0e-5)},
        {"NonlinearHarmonicLineSearchStepFactor", ParamType::Real, realDefault(1.1)},
        {"NonlinearMaxLoad", ParamType::Integer, integerDefault(60)},
        {"NonlinearMinLoad", ParamType::Integer, integerDefault(10)},
        {"NonlinearPacking", ParamType::Integer, integerDefault(100)},
        {"NonlinearTolerance", ParamType::Integer, integerDefault(2),
         "0|1|2|3"}, // AMBIGUOUS: default depends on solution (0 for SOL 153)
        {"NormalModesLoop", ParamType::Integer, integerDefault(0)},
        {"NormalModesNonlinearFreq", ParamType::Real, realDefault(0.0)},
        {"NoCompositeStress", ParamType::Integer, integerDefault(1), "-1|0|1"},
        {"NoDecomposition", ParamType::Integer, integerDefault(0), "0|1"},
        {"NoElemForceOutput", ParamType::Integer, integerDefault(-1)},
        {"NoElemLoadOutput", ParamType::Integer, integerDefault(-1)},
        {"NoNodeForceOutput", ParamType::Integer, integerDefault(1)},
        {"NonCoupledDofs", ParamType::Integer, integerDefault(-1)},
        {"GeneralizedCount", ParamType::Integer, integerDefault(0)},
        {"OutputCount", ParamType::Integer,
         noDefault()}, // AMBIGUOUS: only refers to S1, S1G, S1M; no kind or default stated, kind guessed
        {"OutputCount1", ParamType::Integer, noDefault()}, // AMBIGUOUS: same as NUMOUT
        {"OutputCount2", ParamType::Integer, noDefault()}, // AMBIGUOUS: same as NUMOUT
        {"OutputCount3", ParamType::Integer, noDefault()}, // AMBIGUOUS: same as NUMOUT
        {"OutputElemOption", ParamType::Integer, integerDefault(3), "0|1|2|3"},
        {"OutputElemSet", ParamType::Integer, integerDefault(0)},
        {"OutputGeometry", ParamType::Integer, integerDefault(0)},
        {"OutputGeometryTables", ParamType::Text, textDefault("Yes")},
        {"OutputNodeOption", ParamType::Integer, integerDefault(1), "0|1|2|3"},
        {"OutputNodeSet", ParamType::Integer, integerDefault(0)},
        {"OldResequencing", ParamType::Integer, integerDefault(-1),
         "-1|1|2|3|4|5|6|8|9|10|11"}, // AMBIGUOUS: default depends on the run type (5 iterative, 6 with SUPER=2); a
                                      // table also names option 7 that is not in the list
        {"OldWeldFormulation", ParamType::Text, textDefault("No"), "Yes|No"},
        {"OutputMachPrint", ParamType::Text, textDefault("No")},
        {"OutputMaxRatio", ParamType::Integer,
         noDefault()}, // AMBIGUOUS: default is twice the machine-dependent BUFFSIZE
        {"OutputMaterialFrame", ParamType::Text, textDefault("No"), "No|Yes|Both"},
        {"OutputMessageLevel", ParamType::Integer, integerDefault(0), "0|1"},
        {"OutputGeometryTables34", ParamType::Text, textDefault("True"), "True|False"},
        {"MatrixFileDigits", ParamType::Integer, integerDefault(15)},
        {"OutputPunchSet", ParamType::Integer, integerDefault(0)},
        {"OutputPlotGeometry", ParamType::Integer, integerDefault(-1)},
        {"OutputDifferentialStiffness", ParamType::Integer, integerDefault(-1)},
        {"OutputBoundaryModes", ParamType::Integer, integerDefault(-1)},
        {"OutputAnalysisModes", ParamType::Integer, integerDefault(-1)},
        {"OptimExit", ParamType::Integer, integerDefault(0), "-4|0|1|2|3|4|5|6|7"},
        {"OptionChoice", ParamType::Text, textDefault("Absolute")},
        {"OutputSetElems", ParamType::Integer, integerDefault(2)},
        {"OutputSetNodes", ParamType::Integer, integerDefault(1)},
        {"OutputWelds", ParamType::Integer, integerDefault(-1)},
        {"OutputWeldPoints", ParamType::Integer, integerDefault(-1)},
        {"OutputDisplacementFrame", ParamType::Any,
         noDefault()}, // AMBIGUOUS: only refers to POST; no kind or default stated
        {"OutputDisplacementSplit", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"OutputUnit1", ParamType::Integer,
         noDefault()}, // AMBIGUOUS: default is the value of OUNIT2, kind inferred from it
        {"OutputUnit2", ParamType::Integer, integerDefault(12)},
        {"OutputOption", ParamType::Integer, integerDefault(0)},
        {"AcousticInfinitePropId", ParamType::Integer, noDefault()},
        {"PanelModesRequested", ParamType::Any,
         noDefault()}, // AMBIGUOUS: replaced by a Case Control keyword, no kind or default given
        {"PostprocessorPlus", ParamType::Text, textDefault("No"), "Yes|No"},
        {"PostprocessorVersion", ParamType::Real, realDefault(3.0)},
        {"CompositeReplace", ParamType::Integer, integerDefault(0), "0|1|2"},
        {"DataRecoveryMessages", ParamType::Integer, integerDefault(1)},
        {"PoroElasticDistributedParallel", ParamType::Text, textDefault("No"), "Yes|No"},
        {"PoroElasticFreqRange", ParamType::Integer, integerDefault(0), "0|1"},
        {"PoroElasticMappingTolerance", ParamType::Integer, integerDefault(0), "0|1"},
        {"PoroElasticMatrixFile", ParamType::Integer, integerDefault(1), "0|1"},
        {"PoroElasticPartitions", ParamType::Integer, integerDefault(1)},
        {"PoroElasticSkin", ParamType::Integer, integerDefault(0), "0|1"},
        {"PenaltyFactor", ParamType::Real, realDefault(1.0e5)},
        {"PercentOutput", ParamType::Number,
         integerDefault(40)}, // AMBIGUOUS: kind not stated, a percentage whose default is written as an integer
        {"PhaseTwoOutput", ParamType::Integer, integerDefault(0), "0|1|3"},
        {"SpectrumPeakCombination", ParamType::Integer, integerDefault(-1)},
        {"PlotMessages", ParamType::Integer, integerDefault(1)},
        {"PostprocessingFile", ParamType::Integer, integerDefault(9999999)},
        {"PostprocessingExternal", ParamType::Text, textDefault("No"), "Yes|No"},
        {"PostprocessingAdjointFile", ParamType::Text, textDefault("No"), "Yes|No"},
        {"PostprocessingUnit", ParamType::Integer, integerDefault(-1)},
        {"ReferencePressureDecibel", ParamType::Real, realDefault(1.0)},
        {"PrintSingularities", ParamType::Text, textDefault("No")},
        {"PrintRequest", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"ProgressOutput", ParamType::Integer, integerDefault(-1)},
        {"PrintAnalysisSet", ParamType::Real, realDefault(1.0e37)},
        {"PrintProjection", ParamType::Real, realDefault(1.0e37)},
        {"PrintModalVectors", ParamType::Real, realDefault(1.0e37)},
        {"PrintMaxima", ParamType::Text, textDefault("No")},
        {"PrintResultants", ParamType::Text, textDefault("Yes")},
        {"SensitivityPunch", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ParticipationThreshold", ParamType::Integer, integerDefault(-1)},
        {"DynamicPressure", ParamType::Real, realDefault(0.0)},
        {"RadiationModel", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"RigidBodySetPrint", ParamType::Integer, integerDefault(0)},
        {"RemoveZeros", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ResultantOption", ParamType::Integer, integerDefault(8)},
        {"ResidualVectors", ParamType::Any,
         noDefault()}, // AMBIGUOUS: obsolete, replaced by a Case Control command; no kind and no default given
        {"RotationalForceSet", ParamType::Integer, integerDefault(0)},
        {"DensityHeatCapacity", ParamType::Real, realDefault(1.0)},
        {"RigidInterpolation", ParamType::Text, textDefault("Linear"), "Linear|Log10"},
        {"RungeKuttaScheme", ParamType::Integer, noDefault()},
        {"RootMeanSquareIntegration", ParamType::Text, textDefault("Linear"), "Linear|LogLog"},
        {"RootMeanSquareParams", ParamType::Text, textDefault("No"), "Yes|No"},
        {"OutputHydrostatic", ParamType::Real,
         noDefault()}, // AMBIGUOUS: default is a multiple of the reference density, two different multiples given
        {"OutputMultiple", ParamType::Real, noDefault()},
        {"OutputStressRotation", ParamType::Real, noDefault()},
        {"ResponseSpectra", ParamType::Integer, integerDefault(-1)},
        {"ResponseSpectraPrint", ParamType::Integer, integerDefault(0)},
        {"RestartTemperature", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ResidualVectorLoadFilter", ParamType::Text, textDefault("No"), "Yes|No"},
        {"StressSign", ParamType::Integer, integerDefault(-1)},
        {"StressSignNode", ParamType::Integer, integerDefault(-1)},
        {"StressSignMode", ParamType::Integer, integerDefault(-1)},
        {"StressSignAggregate", ParamType::Integer, integerDefault(-1)},
        {"StressSignAggregateMode", ParamType::Integer, integerDefault(-1)},
        {"SubspaceForm", ParamType::Text, noDefault(),
         "ModalFormulation|StaticModalFormulation"}, // AMBIGUOUS: the two option words are read from loose MF and SM
                                                     // labels
        {"ScaledResponseSpectra", ParamType::Integer, integerDefault(-1)},
        {"SensitivityTableFile", ParamType::Integer, integerDefault(0)},
        {"SensitivityUnit", ParamType::Integer,
         noDefault()}, // the n of the format reference heading is the unit number given as the value
        {"SuperelementDifferentialStiffness", ParamType::Text, textDefault("ResidualOnly"), "ResidualOnly|All"},
        {"SuperelementMapping", ParamType::Text, textDefault("SuperelementMap"),
         "SuperelementMap|SuperelementMapAll|SuperelementMapConnected|SuperelementMapEstimate|SuperelementMapPunch"},
        {"SuperelementMappingOption", ParamType::Integer, integerDefault(42)},
        {"SuperelementMappingPrint", ParamType::Integer, integerDefault(3)},
        {"SensitivityOmittedSet", ParamType::Text, textDefault("No"), "Yes|No"},
        {"SuperelementMergeOverride", ParamType::Integer, integerDefault(0)},
        {"SequenceOutput", ParamType::Integer, integerDefault(0)},
        {"SuperelementRestart", ParamType::Text, textDefault("Automatic"),
         "Automatic|Manual|Semi|ResidualModes|SuperelementRecovery"},
        {"SuperelementStructuralDamping", ParamType::Text, textDefault("No"), "Yes|No|AugmentedModes"},
        {"SuperelementStrainEnergyFraction", ParamType::Integer, integerDefault(-1), "-1|0|1"},
        {"EigenShift1", ParamType::Real, realDefault(-1.234)},
        {"ShellDampingOption", ParamType::Text, textDefault("Same")},
        {"StefanBoltzmannConstant", ParamType::Real, realDefault(0.0)},
        {"SkinOutput", ParamType::Text, textDefault("None"), "None|Punch|Print|All"},
        {"SkipAmplitude", ParamType::Integer, integerDefault(0)},
        {"SubLoopIdentifier", ParamType::Integer, integerDefault(0)},
        {"SmallDynamicPressure", ParamType::Real, realDefault(0.0)},
        {"ShellNormalAngle", ParamType::Real, realDefault(20.0)},
        {"ShellNormalPrint", ParamType::Integer, integerDefault(-1)},
        {"SoftExitOption", ParamType::Text, textDefault("No"), "Yes|No"},
        {"SolutionAdjacency", ParamType::Integer, integerDefault(0)},
        {"SolidOption", ParamType::Integer, integerDefault(0)},
        {"SparseDataMatrix", ParamType::Text, textDefault("Yes")},
        {"SparseDataRecovery", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"SparsePhase", ParamType::Text, textDefault("Yes"), "Yes|No"},
        {"SparseRecoveryRatio", ParamType::Number,
         integerDefault(75)}, // AMBIGUOUS: kind not stated, a percentage whose default is written as an integer
        {"ConstraintGeneration", ParamType::Text, textDefault("No")},
        {"SparseMatrixRatio", ParamType::Number,
         integerDefault(60)}, // AMBIGUOUS: kind not stated, a percentage whose default is written as an integer
        {"SuperelementGeneralizedSetId", ParamType::Integer, integerDefault(99000001)},
        {"StrengthRatioCompositeStress", ParamType::Text, textDefault("No"), "Yes|No"},
        {"SortElemType", ParamType::Integer, integerDefault(0)},
        {"SortOption", ParamType::Integer, integerDefault(0), "0|1|2|3"},
        {"StartPoint", ParamType::Integer, integerDefault(0)},
        {"StepFactor", ParamType::Real, realDefault(0.666)},
        {"StepFactorLimit", ParamType::Real, realDefault(0.9)},
        {"StartTime", ParamType::Real, realDefault(0.0)},
        {"StructureModesRequested", ParamType::Any,
         noDefault()}, // AMBIGUOUS: replaced by a keyword of a Case Control command; no kind and no default given
        {"SubcaseIdentifier", ParamType::Integer, integerDefault(0)},
        {"SubcaseNumber", ParamType::Integer, integerDefault(1)},
        {"AeroSupport", ParamType::Text, textDefault("SupersonicLiftingSurface"),
         "SupersonicLiftingSurface|PanelMethodCoupling"},
        {"SupportedDofs", ParamType::Integer, integerDefault(0)},
        {"SuperelementProcessing", ParamType::Integer,
         integerDefault(0)}, // AMBIGUOUS: default is 0 for non-superelement runs and -1
                             // for superelement runs; kind inferred from the defaults
        {"TableIdentifier", ParamType::Integer, integerDefault(2)},
        {"AbsoluteTemperatureOffset", ParamType::Real, realDefault(0.0)},
        {"ThermalDamping", ParamType::Real, realDefault(0.0)},
        {"NegativeTermTest", ParamType::Integer, integerDefault(-2),
         "-2|-1|0|1|2|3"}, // AMBIGUOUS: default depends on the method, -2 (Newton) or -1 (arc-length and linear
                           // solutions)
        {"TransferFunctionSymmetryFactor", ParamType::Real, realDefault(1.0e-8)},
        {"TinyStrainEnergy", ParamType::Real, realDefault(1.0e-3)},
        {"ToleranceRescale", ParamType::Real, realDefault(0.05)},
        {"TopologyConvergence", ParamType::Number,
         integerDefault(80)}, // AMBIGUOUS: kind not stated, a percentage whose default is written as an integer
        {"TrimBimodal", ParamType::Text, textDefault("Physical"), "Physical|Modal|DataCheckOnly"},
        {"TransientStatic", ParamType::Integer, integerDefault(-1)},
        {"UniversalGasConstant", ParamType::Real, noDefault()},
        {"UnsymmetricFactor", ParamType::Text, textDefault("No"), "Yes|No"},
        {"UnsymmetricBoundaryStiffness", ParamType::Text, textDefault("No"), "Yes|No"},
        {"UpdateBushes", ParamType::Text, textDefault("No"), "Yes|No"},
        {"DofSetPrint", ParamType::Integer, integerDefault(-1), "-1|0|1|2|10|11|12"},
        {"DofSetPrintSelection", ParamType::Integer, integerDefault(0)},
        {"DofSetPrintSets", ParamType::Text, noDefault(), "",
         true}, // AMBIGUOUS: character parameter whose default is a blank string
        {"VariablePhase", ParamType::Real, realDefault(0.78539816)},
        {"VelocityCutoff", ParamType::Real, realDefault(1.0e-6)},
        {"VirtualMassOption", ParamType::Integer, integerDefault(0)},
        {"ReferenceVelocity", ParamType::Real, realDefault(1.0)},
        {"WhirlOption", ParamType::Text, textDefault("Forward"), "Forward|Backward"},
        {"ModalWeighting", ParamType::Text, textDefault("No"), "Yes|No"},
        {"WriteBeamBranches", ParamType::Integer, integerDefault(0), "-1|0"},
        {"WeightToMass", ParamType::Real, realDefault(1.0)},
        {"DampingReferenceFreq", ParamType::Real, realDefault(0.0)},
        {"ElemDampingReferenceFreq", ParamType::Real, realDefault(0.0)},
        {"DampingReferenceFreqFluid", ParamType::Real, realDefault(0.0)},
        {"ElemDampingReferenceFreqFluid", ParamType::Real, realDefault(0.0)},
        {"RotorDampingReferenceFreq", ParamType::Real, realDefault(0.0)},
        {"RotorElemDampingReferenceFreq", ParamType::Real, realDefault(0.0)},
        {"RotorHysteresisFreq", ParamType::Real, realDefault(0.0)},
        {"ExpansionFactorParam", ParamType::Real, realDefault(0.5)},
        {"ExpansionFactorXParam", ParamType::Real, realDefault(1.0)},
        {"TemperatureLoadFlag", ParamType::Integer, integerDefault(0)},
        {"CurvePunch", ParamType::Text, textDefault("No"), "Yes|No"},
        {"CurveUnit", ParamType::Integer,
         noDefault()}, // the n of the format reference heading is the unit number given as the value
        {"ZeroCouplingMass", ParamType::Text, textDefault("No"), "Yes|No"},
        {"ZeroVectors", ParamType::Real, realDefault(1.0e-6)},
    };
    return ENTRIES;
}

} // namespace oor::model
