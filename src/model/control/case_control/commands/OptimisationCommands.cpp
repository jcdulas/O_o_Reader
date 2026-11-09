#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference, the commands of design optimisation (SOL 200) and of adaptive meshing.
void defineOptimisationCommands(CommandCatalogue& commands)
{
    // a DCONSTR or DCONADD set
    commands.integer("GlobalDesignConstrSelection").refersTo(ReferenceTarget::DesignConstraintSet);
    commands.text("DesignModelName");
    commands.integer("DesignObjectiveSelection")
        .variant("MinimumObjective")
        .variant("MaximumObjective")
        .refersTo(ReferenceTarget::DesignResponse); // DESOBJ(MIN) = n minimises the response n
    commands.integer("SubcaseDesignConstrSelection").refersTo(ReferenceTarget::DesignConstraintSet);
    commands.integer("DesignVarSelection");
    commands.integer("DesignResponseSpanSelection");
    commands.output("DesignSensitivityOutput"); // DSAPRT(FORMATTED,EXPORT,END=SENS) = ALL
    commands.integer("MeshAdaptivitySelection").refersTo(ReferenceTarget::AdaptivitySet); // a HADAPTL entry
}

} // namespace oor::model
