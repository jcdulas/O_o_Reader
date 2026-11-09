#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// Fluid-structure interface parameters. SurfaceCoupling is the default; ScaledSurfaceCoupling uses the same
// tolerance and search settings. ClosedPressure accepts only None or All for interfaceInput and keeps the skin,
// interface tolerance and search settings at their defaults.
//
// fluidSetId and structureSetId name the fluid skin and structural sets. A value of -1 selects the sets defined by
// ModuleFluidSet and ModuleStructureSet; zero leaves the set unspecified. normalTolerance defaults to 1.0
// for DifferenceForm and 0.001 for IdenticalMesh and is stored resolved.
struct FluidStructureInterfaceCard
{
    std::string interfaceKind = "DifferenceForm";
    std::string interfaceInput = "None";
    base::EntityId fluidSetId = 0;
    base::EntityId structureSetId = 0;
    double normalTolerance = 1.0;
    std::string methodName = "SurfaceCoupling";
    double skinTolerance = 0.5;
    double skinToleranceIncrement = 0.75;
    double interfaceTolerance = 0.5;
    std::string wholeStructureSet = "No";
    std::string searchUnit = "Relative";
};

} // namespace oor::model
