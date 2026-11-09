#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Data Selection": the dynamic and nonlinear solution conditions (eigenvalue extraction, frequencies,
// time steps, damping, random analysis, nonlinear parameters).
void defineSolutionCommands(CommandCatalogue& commands)
{
    // Eigenvalue extraction, frequencies and time steps.
    commands.integer("EigenMethodSelection").refersTo(ReferenceTarget::Method);
    commands.integer("ComplexEigenMethodSelection").refersTo(ReferenceTarget::ComplexMethod);
    commands.integer("FlutterMethodSelection").refersTo(ReferenceTarget::AeroFlutterSet);
    commands.integer("IterativeSolverSelection").refersTo(ReferenceTarget::IterativeSolver); // an ITER entry
    commands.integer("RotorMethodSelection");
    commands.integer("FreqSetSelection").refersTo(ReferenceTarget::FrequencySet);
    // FRF and FRF(GEN | ASM | GENASM, COMPID=..., COMPNAME=..., CONNPTS=..., ...): a list of describers in parentheses
    // and no value (format reference FRF); the describers are kept as written.
    commands.flag("FrfGeneration");
    commands.integer("TimeStepSelection").refersTo(ReferenceTarget::TimeStepSet); // a TSTEP or a TSTEPNL
    commands.integer("InitialConditionSelection").refersTo(ReferenceTarget::InitialConditionSet);
    commands.text("ModeSelectionList");
    commands.text("ModeMapping");
    commands.integer("ModeSelectionSet");
    commands.integer("ModuleSetSelection");
    commands.integer("ModeTrackingSelection");
    commands.keyword("ResidualVectorOption");
    commands.integer("RotordynamicSelection");
    commands.text("ModalFreqSelection");
    commands.text("NonlinearHarmonicIterations");
    commands.text("OperatingShapeFreqs");

    // Damping, mass, random analysis and harmonics.
    // a TABDMP1, or a TABLEDi
    commands.integer("ModalDampingSelection").refersTo(ReferenceTarget::HybridDampingRotorOptionsTable);
    // a DAMPING entry
    commands.integer("RotorDampingSelection").refersTo(ReferenceTarget::HybridDampingRotorOptionsSpecification);
    commands.integer("NonStructMassSelection");
    commands.integer("RandomAnalysisSelection").refersTo(ReferenceTarget::RandomSet);
    commands.integer("RandomVariableSelection");
    commands.integerOrKeyword("HarmonicsSelection");
    commands.integer("DataRecoverySelection");
    commands.integer("OutputRecoverySelection");
    commands.keyword("SequenceDependentLoads");

    // Nonlinear analysis.
    // an NLPARM and its NLPCI
    commands.integer("NonlinearStaticParamsSelection").refersTo(ReferenceTarget::NonlinearSet);
    commands.integer("NonlinearStepSelection").refersTo(ReferenceTarget::NonlinearStep);
    commands.integer("NonlinearInitialConditionSelection");
    commands.integer("NonlinearHarmonicSelection").refersTo(ReferenceTarget::NonlinearHarmonic);
    commands.integer("NonlinearBucklingSelection");
    commands.integer("NonlinearControlSelection");
    commands.integer("NonlinearTimeStepSelection").refersTo(ReferenceTarget::TimeStepSet);
    commands.text("NonlinearOutputParams");
    commands.text("NonlinearRestart");
    commands.integer("StepEnd");
    commands.real("StepEndTime");
    commands.integer("AdaptivitySelection");
    commands.integer("GeneralizedAeroForceSelection");
}

} // namespace oor::model
