#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Data Selection": static and dynamic loads, constraints, thermal fields, static solution conditions.
void defineLoadAndConstraintCommands(CommandCatalogue& commands)
{
    // Static and dynamic load selection.
    // DEFORM, LOADSET and NONLINEAR select typed cards: the DEFORM set, the LSEQ set (an LSEQ is a card of the Load
    // category) and the NOLIN1 to NOLIN4 set (the DynamicLoad category).
    commands.integer("DeformationLoadSelection").refersTo(ReferenceTarget::LoadCombinationSet);
    commands.integer("SuperelementLoadCombinationSelection")
        .refersTo(ReferenceTarget::LoadCombinationSet); // a CLOAD (category Load), selected by its CID
    commands.integer("StaticLoadSelection").refersTo(ReferenceTarget::LoadCombinationSet);
    commands.text("LoadNameText");
    commands.integer("DynamicLoadSelection").refersTo(ReferenceTarget::DynamicLoadSet);
    commands.integer("LoadSequenceSelection").refersTo(ReferenceTarget::LoadCombinationSet);
    commands.integer("NonlinearLoadSelection").refersTo(ReferenceTarget::DynamicLoadSet);
    commands.keyword("AxisymmetricBoundary");

    // Constraint selection. AUTOSPC(NOPRINT) = YES keeps its option in the entry's options.
    commands.keyword("AutoSingularityConstrSelection");
    commands.integer("BoundaryConditionSelection");
    commands.keyword("DihedralSymmetry");
    commands.integer("MultiPointConstrSelection").refersTo(ReferenceTarget::ConstraintSet);
    commands.integer("SinglePointConstrSelection").refersTo(ReferenceTarget::ConstraintSet);
    // the SID of the SUPORT1 cards
    commands.integer("FreeBodySupportSelection").refersTo(ReferenceTarget::ConstraintSet);

    // Static solution conditions (the SUPORT1 of a deck may be written SUPORT: the abbreviation rule covers it).
    commands.integer("StaticSubcaseSelection");

    // format reference: TEMPERATURE(INITIAL), (LOAD), (MATERIAL) and (BOTH) select a temperature set for different
    // uses: each is an attribute of its own. All of them name a set of TEMP cards.
    commands.integer("TemperatureSelection")
        .variant("InitialTemperature")
        .variant("TemperatureLoadPart")
        .variant("MaterialTemperature")
        .variant("BothLoadAndInitial")
        .refersTo(ReferenceTarget::LoadCombinationSet);
    commands.integer("StructuralTemperatureSelection");
}

} // namespace oor::model
