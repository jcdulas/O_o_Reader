#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Superelement Control": which superelements a subcase processes, generates, reduces or recovers
// (ALL, NONE or a superelement ID). The number is the one of a superelement, or the one of a Case Control SET of
// superelement numbers (the reference is skipped when the number names a SET: CaseControl.cpp), and 0 is the residual
// structure. SUPER may add a load sequence number: SUPER = 17, 3.
void defineSuperelementCommands(CommandCatalogue& commands)
{
    commands.integerOrKeyword("SuperelementSelection").refersTo(ReferenceTarget::Superelement).withLoadSequence();
    commands.integerOrKeyword("SuperelementAllSteps").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementDataRecovery").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementDesignVars").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementExclusion").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementStiffnessReduction").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementLoadGeneration").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementLoadReduction").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementMatrixGeneration").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementMassReduction").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementResponseSensitivity").refersTo(ReferenceTarget::Superelement);
    commands.integerOrKeyword("SuperelementDampingSelection")
        .refersTo(ReferenceTarget::HybridDampingRotorOptionsSpecification); // a DAMPING entry, not a superelement
    commands.integer("SuperelementFinal").refersTo(ReferenceTarget::Superelement);

    commands.text("ExternalDataRecoveryInput");
    commands.text("ExternalDataRecoveryOutputRequest");
    commands.text("ExternalModuleOutput");
}

} // namespace oor::model
