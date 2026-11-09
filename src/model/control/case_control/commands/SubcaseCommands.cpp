#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Subcase Definition": the commands that open a subcase or an output block, and the ones that
// belong to the structure of the section rather than to a subcase.
void defineSubcaseCommands(CommandCatalogue& commands)
{
    commands.subcaseHeader("SubcaseStart", SubcaseKind::Subcase);
    commands.subcaseHeader("SubcaseCombination", SubcaseKind::SubcaseCombination);
    commands.subcaseHeader("SymmetrySubcase", SubcaseKind::SymmetrySubcase);
    commands.subcaseHeader("SymmetryCombination", SubcaseKind::SymmetryCombination);
    commands.subcaseHeader("RepeatSubcase", SubcaseKind::RepeatSubcase);
    commands.subcaseHeader("StepStart", SubcaseKind::Step);
    commands.subcaseHeader("SubstepStart", SubcaseKind::Substep);
    // format reference AUXCASE (SOL 200): the Case Control of an auxiliary model starts here, after the primary one,
    // and runs to the next AUXCASE or BEGIN BULK; it has no number.
    commands.subcaseHeader("AuxiliaryCase", SubcaseKind::AuxiliaryCase);

    // format reference: SUBSEQ belongs to a SUBCOM and SYMSEQ to a SYMCOM; they give the coefficients of the
    // combination. They are commands of the subcase, not subcase kinds of their own.
    commands.realList("SubcaseCombinationFactors");
    commands.realList("SubcaseCombinationFactors1");
    commands.realList("SymmetryCombinationFactors");

    commands.integer("ModeRepetition");
    commands.flag("MasterSubcase");

    // SET n = members, PARAM,NAME,value and the output blocks have a structure of their own (sets, parameters, blocks).
    commands.setDefinition("SetDefinition");
    commands.parameter("SubcaseParameter");
    commands.blockHeader("OutputBlock", "");
    commands.blockHeader("SetsBlock", "Postprocessing"); // SETS DEFINITION opens the grid point stress block
}

} // namespace oor::model
