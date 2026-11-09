#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Contact", "Module Control", "Miscellaneous", "Monte-Carlo simulation" and the commands whose
// syntax the format reference gives as a list of options: the value is kept as text (the library reads and writes it,
// and does not interpret it).
void defineContactAndModuleCommands(CommandCatalogue& commands)
{
    // Contact.
    // BCONTACT = n (a BCTABL1, BCTABLE or BCONECT), ALLBODY, NONE, ALLELE, or AUTO with the type of the contact it
    // generates (format reference BCONTACT: TOUCH by default, PGLUE, GGLUE, SGLUE); a record writes the neutral names
    // (AutoContact,PermanentGlue).
    commands.integerOrKeyword("ContactSelection")
        .refersTo(ReferenceTarget::ContactSelection)
        .qualifiedWord("AutoContact", {"TouchOnly", "PermanentGlue", "GeneralGlue", "StandardGlue"});
    commands.integer("ContactSetSelection");
    commands.integer("GlueSetSelection");
    commands.integer("ContactBodyChangeSelection");
    commands.integer("ContactBodyMoveSelection");
    commands.integer("BrakeSquealSelection").refersTo(ReferenceTarget::BrakeSqueal);
    commands.integer("UnglueSelection");
    commands.integer("BoltLoadSelection");
    commands.integer("DeactivatedElemsSelection").refersTo(ReferenceTarget::DeactivatedElementSet);
    commands.text("ContactCheck");
    commands.text("SolidSkinOutput");
    commands.text("CrackClosureSelection");

    // Module control, the SOL 700 and the interfaces to other programs. AUXCASE opens a section (SubcaseCommands);
    // AUXMODEL = n names the part BEGIN AUXMODEL = n of the auxiliary model a section after AUXCASE analyses.
    commands.integer("AuxiliaryModelSelection");
    commands.flag("ModuleEnd");
    commands.text("FlexibleBodyExport");
    commands.text("CampbellDiagramSelection");
    commands.text("CosimulationServiceSelection");
    commands.text("DatabaseSave");
    commands.text("DynamicStiffnessOption");
    commands.text("LoadExport");
    commands.text("FreeBodyLoadSelection");
    commands.text("ModelCheck");
    commands.integer("MonteCarloSelection").refersTo(ReferenceTarget::MonteCarloSet);
    commands.integer("ModuleSolutionSelection");

    // Miscellaneous: rigid body handling, imperfections, rotor dynamics, tires, trim of the stochastic analysis.
    commands.keyword("RigidElemMethod");
    commands.integer("ImperfectionSelection").refersTo(ReferenceTarget::ImperfectionCase); // an IMPCASE entry
    commands.output("ImperfectionOutput");
    commands.integer("InertiaReliefLoadSelection");
    commands.text("RotorBendSelection");
    commands.text("PartSetDefinition");
    commands.text("StochasticSelection");
    commands.text("TireSelectionRequest");
    commands.integer("TransferPathSelection").refersTo(ReferenceTarget::TransferPathSet);
    commands.text("WettedSensitivity");
    commands.integer("SplineOutputRequest");
    commands.integer("MassCombinationSelection").refersTo(ReferenceTarget::MassCombination); // a MASSSET card
}

} // namespace oor::model
