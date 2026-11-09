#pragma once

#include "oor/model/control/case_control/CommandCatalogue.hpp"

namespace oor::model
{

// The definitions of the Case Control commands, one function per family of commands. Each
// command is one hand-written statement: its syntax, its aliases, the option variants that are attributes of their own
// and what its number refers to. CommandCatalogue calls them in this order when it is built.
void defineSubcaseCommands(CommandCatalogue& commands);
void defineLoadAndConstraintCommands(CommandCatalogue& commands);
void defineSolutionCommands(CommandCatalogue& commands);
void defineMatrixCommands(CommandCatalogue& commands);
void defineAeroelasticityCommands(CommandCatalogue& commands);
void defineOptimisationCommands(CommandCatalogue& commands);
void defineOutputControlCommands(CommandCatalogue& commands);
void defineOutputRequestCommands(CommandCatalogue& commands);
void defineSuperelementCommands(CommandCatalogue& commands);
void defineContactAndModuleCommands(CommandCatalogue& commands);
void definePlotCommands(CommandCatalogue& commands);
void defineXyPlotCommands(CommandCatalogue& commands);

} // namespace oor::model
