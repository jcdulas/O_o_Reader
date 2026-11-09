#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Aerodynamic Analysis" and "Fluid-Structure Analysis": configuration, symmetry flags, trim, flutter,
// gust.
void defineAeroelasticityCommands(CommandCatalogue& commands)
{
    commands.keyword("AnalysisKind");
    commands.keyword("AeroConfigurationName");
    commands.keyword("AeroConfigurationNameAlt");
    commands.keyword("AeroSymmetryXy").words({"SymmetricFlow", "NoSymmetryFlow", "AntiSymmetricFlow"});
    commands.keyword("AeroSymmetryXz").words({"SymmetricFlow", "NoSymmetryFlow", "AntiSymmetricFlow"});
    commands.integer("AeroReferenceControlVector").refersTo(ReferenceTarget::ControlVector);
    commands.integer("AeroLoadSelection");
    commands.integer("ControlScheduleSelection").refersTo(ReferenceTarget::ControlSchedule);
    commands.integer("DivergenceSelection").refersTo(ReferenceTarget::DivergenceSet);
    commands.integer("GustSelection").refersTo(ReferenceTarget::AeroGustSet);
    commands.integer("TrimSelection").refersTo(ReferenceTarget::AeroTrimSet); // a TRIM or a TRIM2
    commands.text("TrimGroupSelection");
    commands.text("TrimmedMaterialSelection");
    commands.text("VortexLatticeExport");

    commands.text("FluidFrequencySelection");
    commands.text("FluidStructurePanelOutput");
    commands.text("FluidStructureControls");
}

} // namespace oor::model
