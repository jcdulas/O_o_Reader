#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference, the output requests of the physical and the solution sets: NAME(options) = ALL | NONE | set ID. A
// deck may repeat a request with other options (DISP(PLOT) = ALL and DISP(PRINT) = 5); each is its own request.
void defineOutputRequestCommands(CommandCatalogue& commands)
{
    // Nodal results. format reference: DISPLACEMENT, VECTOR and PRESSURE are one command under three names.
    commands.output("DisplacementOutput");
    commands.output("VelocityOutput");
    commands.output("AccelOutput");
    commands.output("SolutionSetDisplacementOutput");
    commands.output("SolutionSetVelocityOutput");
    commands.output("SolutionSetAccelOutput");
    commands.output("SolutionSetEigenvectorOutput");
    commands.output("TotalDisplacementOutput");
    commands.output("TotalVelocityOutput");
    commands.output("TotalAccelOutput");
    commands.output("NodeVectorOutput");
    commands.output("NormalVelocityOutput");
    commands.output("RelativeDisplacementOutput");

    // Element results. format reference: FORCE is also ELFORCE, STRESS is also ELSTRESS, STRAIN is also ELSTRAIN.
    commands.output("ElemForceOutput");
    commands.output("StressOutput");
    commands.output("StrainOutput");
    commands.output("ElemAcousticForceOutput");
    commands.output("ElemSummaryOutput");
    commands.output("ElemStressDiscontinuityOutput");
    commands.output("ElemSensitivityOutput");
    commands.output("StressFieldOutput");
    commands.output("NonlinearStressOutput");
    commands.output("PlasticStrainOutput");
    commands.output("ThermalStrainOutput");
    commands.output("ShellThicknessOutput");
    commands.output("FatigueOutput");
    commands.output("RadiatedPowerOutput");

    // Grid point results.
    commands.output("NodeForceOutput");
    commands.output("NodeStressOutput");
    commands.output("NodeStrainOutput");
    commands.output("NodeKineticEnergyOutput");
    commands.output("NodeStressDiscontinuityOutput");
    commands.output("GasketResultsOutput");
    commands.output("ViewNodeOutput");
    commands.output("AcousticTransferVectorOutput");

    // Energy results.
    commands.output("ElemStrainEnergyOutput");
    commands.output("ElemKineticEnergyOutput");
    commands.output("ElemDampingEnergyOutput");
    commands.output("ModalKineticEnergyOutput");
    commands.output("ModalStrainEnergyOutput");
    commands.output("RotorEnergyOutput");
    commands.output("ComponentModeEnergyOutput");

    // Reactions, applied loads and equilibrium.
    commands.output("SinglePointConstrForceOutput");
    commands.output("MultiPointConstrForceOutput");
    commands.output("AppliedLoadOutput");
    commands.output("NonlinearLoadOutput");
    commands.output("EquilibriumOutput");
    commands.output("ModalEffectiveMassOutput");
    commands.output("ModalContributionOutput");
    commands.output("EnthalpyRateOutput");
    commands.output("MonitorPointOutput");
    commands.output("PeakResponseOutput");

    // Thermal, aerodynamic, acoustic and contact results.
    commands.output("ThermalOutput");
    commands.output("HeatFluxOutput");
    commands.output("EnthalpyOutput");
    commands.output("HeatFlowOutput");
    commands.output("TemperatureOutput");
    commands.output("AeroForceOutput");
    commands.output("AeroPressureOutput");
    commands.output("TrimLoadOutput");
    commands.output("AcousticPowerOutput");
    commands.output("AcousticFieldPointOutput");
    commands.output("AcousticIntensityOutput");
    commands.output("VectorIntensityOutput");
    commands.output("WettedPressureOutput");
    commands.output("SlidelineContactOutput");
    commands.output("ContactResultsOutput");
    commands.output("GlueResultsOutput");
    commands.output("BoltResultOutput");
    commands.output("HarmonicOutput");
    commands.output("CyclicSegmentOutput");
    commands.output("CrossSpectrumOutput");
    commands.output("RandomResponseOutput");
    commands.output("ParticleAccelOutput");
    commands.output("BendingLoadOutput");
    commands.output("InterfaceForceOutput");

    // Panel and grid participation, model checkout.
    commands.output("NodeParticipationOutput");
    commands.output("ModeParticipationOutput");
    commands.output("PanelParticipationOutput");
    commands.output("ParticipationResultsOutput");
    commands.output("GroundCheckOutput");
    commands.output("WeightCheckOutput");
    commands.output("ResultFileModulesOutput");
    commands.output("ExternalSuperelementOutput");
}

} // namespace oor::model
