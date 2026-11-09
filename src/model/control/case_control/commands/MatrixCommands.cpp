#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference, the commands that select direct input matrices: each command names DMIG/DMIAX/DMI matrices, with an
// optional scale factor ("K2GG = 0.5*KAA, KBB"). The names are kept as text.
void defineMatrixCommands(CommandCatalogue& commands)
{
    commands.text("DirectInputDampingMatrix");
    commands.text("DirectInputViscousMatrix");
    commands.text("DirectInputViscousMatrixP");
    commands.text("DirectInputStiffnessMatrix");
    commands.text("DirectInputStiffnessMatrixP");
    commands.text("DirectInputStructuralDampingMatrix");
    commands.text("DirectInputMassMatrix");
    commands.text("DirectInputMassMatrixP");
    commands.text("DirectInputLoadMatrix");
    commands.text("TransferFunctionSelection");
    commands.integer("FluidVirtualMassSelection");
}

} // namespace oor::model
