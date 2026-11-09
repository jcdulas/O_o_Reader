// The stored columns of the cards of the family "elements", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/acousticAbsorberProp/AcousticAbsorberPropTable.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid2/AcousticAxisymFluid2Table.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid3/AcousticAxisymFluid3Table.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid4/AcousticAxisymFluid4Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteProp/AcousticInfinitePropTable.hpp"
#include "oor/model/cards/elements/acousticSlot3/AcousticSlot3Table.hpp"
#include "oor/model/cards/elements/acousticSlot4/AcousticSlot4Table.hpp"
#include "oor/model/cards/elements/axisymQuad/AxisymQuadTable.hpp"
#include "oor/model/cards/elements/axisymQuad4/AxisymQuad4Table.hpp"
#include "oor/model/cards/elements/axisymQuad8/AxisymQuad8Table.hpp"
#include "oor/model/cards/elements/shellAxisym/ShellAxisymTable.hpp"
#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"
#include "oor/model/cards/elements/axisymTria6/AxisymTria6Table.hpp"
#include "oor/model/cards/elements/barArbitrarySectionProp/BarArbitrarySectionPropTable.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsTable.hpp"
#include "oor/model/cards/elements/bar2Stations/Bar2StationsTable.hpp"
#include "oor/model/cards/elements/beamArbitrarySectionProp/BeamArbitrarySectionPropTable.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/beam3/Beam3Table.hpp"
#include "oor/model/cards/elements/beam2Defaults/Beam2DefaultsTable.hpp"
#include "oor/model/cards/elements/beamPropNonlinear/BeamPropNonlinearTable.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"
#include "oor/model/cards/elements/bushing2D/Bushing2DTable.hpp"
#include "oor/model/cards/elements/cohesiveAxisymQuad/CohesiveAxisymQuadTable.hpp"
#include "oor/model/cards/elements/cohesiveHexa/CohesiveHexaTable.hpp"
#include "oor/model/cards/elements/cohesiveMat/CohesiveMatTable.hpp"
#include "oor/model/cards/elements/cohesivePenta/CohesivePentaTable.hpp"
#include "oor/model/cards/elements/cohesiveProp/CohesivePropTable.hpp"
#include "oor/model/cards/elements/cohesiveQuad/CohesiveQuadTable.hpp"
#include "oor/model/cards/elements/shellConical2/ShellConical2Table.hpp"
#include "oor/model/cards/elements/conicalShellProp/ConicalShellPropTable.hpp"
#include "oor/model/cards/elements/crackTip2D/CrackTip2DTable.hpp"
#include "oor/model/cards/elements/crackTip3D/CrackTip3DTable.hpp"
#include "oor/model/cards/elements/crackTipProp2D/CrackTipProp2DTable.hpp"
#include "oor/model/cards/elements/crackTipProp3D/CrackTipProp3DTable.hpp"
#include "oor/model/cards/elements/curvedBeam2/CurvedBeam2Table.hpp"
#include "oor/model/cards/elements/damperDofProperty/DamperDofPropertyTable.hpp"
#include "oor/model/cards/elements/damperDofPropertyExplicit/DamperDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/damperDofInline/DamperDofInlineTable.hpp"
#include "oor/model/cards/elements/damperDofInlineExplicit/DamperDofInlineExplicitTable.hpp"
#include "oor/model/cards/elements/damperScalarProperty/DamperScalarPropertyTable.hpp"
#include "oor/model/cards/elements/damperScalarInline/DamperScalarInlineTable.hpp"
#include "oor/model/cards/elements/elemGeneration/ElemGenerationTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/gap2/Gap2Table.hpp"
#include "oor/model/cards/elements/generalMatrix/GeneralMatrixTable.hpp"
#include "oor/model/cards/elements/heatCapacity2/HeatCapacity2Table.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/lineInterface/LineInterfaceTable.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/massScalarProperty/MassScalarPropertyTable.hpp"
#include "oor/model/cards/elements/massScalarInline/MassScalarInlineTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Table.hpp"
#include "oor/model/cards/elements/planeStrainTria6/PlaneStrainTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad8/PlaneStrainQuad8Table.hpp"
#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Table.hpp"
#include "oor/model/cards/elements/planeStressQuad4/PlaneStressQuad4Table.hpp"
#include "oor/model/cards/elements/planeStressTria6/PlaneStressTria6Table.hpp"
#include "oor/model/cards/elements/planeStressQuad8/PlaneStressQuad8Table.hpp"
#include "oor/model/cards/elements/plotLine2/PlotLine2Table.hpp"
#include "oor/model/cards/elements/plotQuad4/PlotQuad4Table.hpp"
#include "oor/model/cards/elements/plotSurface/PlotSurfaceTable.hpp"
#include "oor/model/cards/elements/plotTria3/PlotTria3Table.hpp"
#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixTable.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/seam/SeamTable.hpp"
#include "oor/model/cards/elements/seamProp/SeamPropTable.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/elements/solidPropNonlinear/SolidPropNonlinearTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofPropertyExplicit/SpringDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/elements/springDofInlineExplicit/SpringDofInlineExplicitTable.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitTable.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/elements/userDefined1/UserDefined1Table.hpp"
#include "oor/model/cards/elements/userDefined2/UserDefined2Table.hpp"
#include "oor/model/cards/elements/userDefined3/UserDefined3Table.hpp"
#include "oor/model/cards/elements/userDefined4/UserDefined4Table.hpp"
#include "oor/model/cards/elements/userDefined5/UserDefined5Table.hpp"
#include "oor/model/cards/elements/userDefined6/UserDefined6Table.hpp"
#include "oor/model/cards/elements/userDefined7/UserDefined7Table.hpp"
#include "oor/model/cards/elements/userDefined8/UserDefined8Table.hpp"
#include "oor/model/cards/elements/userDefined9/UserDefined9Table.hpp"
#include "oor/model/cards/elements/userDefined1Attributes/UserDefined1AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined2Attributes/UserDefined2AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined3Attributes/UserDefined3AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined4Attributes/UserDefined4AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined5Attributes/UserDefined5AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined6Attributes/UserDefined6AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined7Attributes/UserDefined7AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined8Attributes/UserDefined8AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined9Attributes/UserDefined9AttributesTable.hpp"
#include "oor/model/cards/elements/userDefined1Prop/UserDefined1PropTable.hpp"
#include "oor/model/cards/elements/userDefined2Prop/UserDefined2PropTable.hpp"
#include "oor/model/cards/elements/userDefined3Prop/UserDefined3PropTable.hpp"
#include "oor/model/cards/elements/userDefined4Prop/UserDefined4PropTable.hpp"
#include "oor/model/cards/elements/userDefined5Prop/UserDefined5PropTable.hpp"
#include "oor/model/cards/elements/userDefined6Prop/UserDefined6PropTable.hpp"
#include "oor/model/cards/elements/userDefined7Prop/UserDefined7PropTable.hpp"
#include "oor/model/cards/elements/userDefined8Prop/UserDefined8PropTable.hpp"
#include "oor/model/cards/elements/userDefined9Prop/UserDefined9PropTable.hpp"
#include "oor/model/cards/elements/viscousDamper2/ViscousDamper2Table.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerElementsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::UserDefined1AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined1AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined1AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined1AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined1AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined1AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined1AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined2AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined2AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined2AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined2AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined2AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined2AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined2AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined3AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined3AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined3AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined3AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined3AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined3AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined3AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined4AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined4AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined4AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined4AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined4AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined4AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined4AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined5AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined5AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined5AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined5AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined5AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined5AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined5AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined6AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined6AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined6AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined6AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined6AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined6AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined6AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined7AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined7AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined7AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined7AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined7AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined7AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined7AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined8AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined8AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined8AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined8AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined8AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined8AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined8AttributesTable>().elementNameColumn()); });
    }
    {
        using model::UserDefined9AttributesTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined9AttributesTable>();
        registerColumn(catalogue, type, "nodeCount", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined9AttributesTable>().nodeCountColumn()); });
        registerColumn(
            catalogue, type, "connectionFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined9AttributesTable>().connectionFieldCountColumn()); });
        registerColumn(
            catalogue, type, "propertyFieldCount", [](const auto& model)
            { return exportColumn(model->template table<UserDefined9AttributesTable>().propertyFieldCountColumn()); });
        registerColumn(catalogue, type, "displacementComponentCount",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<UserDefined9AttributesTable>().displacementComponentCountColumn());
                       });
        registerColumn(
            catalogue, type, "elementName", [](const auto& model)
            { return exportTextColumn(model->template table<UserDefined9AttributesTable>().elementNameColumn()); });
    }
    {
        using model::Bar2DefaultsTable;
        const std::uint16_t type = model::typeIndexOf<Bar2DefaultsTable>();
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Bar2DefaultsTable>().propIdColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Bar2DefaultsTable>().orientationColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<Bar2DefaultsTable>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "offsetKind", [](const auto& model)
                       { return exportTextColumn(model->template table<Bar2DefaultsTable>().offsetKindColumn()); });
    }
    {
        using model::Beam2DefaultsTable;
        const std::uint16_t type = model::typeIndexOf<Beam2DefaultsTable>();
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Beam2DefaultsTable>().propIdColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Beam2DefaultsTable>().orientationColumn()); });
        registerColumn(
            catalogue, type, "orientationNode", [](const auto& model)
            { return exportColumn(model->template table<Beam2DefaultsTable>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "offsetKind", [](const auto& model)
                       { return exportTextColumn(model->template table<Beam2DefaultsTable>().offsetKindColumn()); });
    }
    {
        using model::AcousticInfiniteTria3Table;
        const std::uint16_t type = model::typeIndexOf<AcousticInfiniteTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteTria3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteTria3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteTria3Table>().nodesColumn()); });
    }
    {
        using model::AcousticInfiniteQuad4Table;
        const std::uint16_t type = model::typeIndexOf<AcousticInfiniteQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteQuad4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfiniteQuad4Table>().nodesColumn()); });
    }
    {
        using model::AcousticAxisymFluid2Table;
        const std::uint16_t type = model::typeIndexOf<AcousticAxisymFluid2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAxisymFluid2Table>().idColumn()); });
    }
    {
        using model::AcousticAxisymFluid3Table;
        const std::uint16_t type = model::typeIndexOf<AcousticAxisymFluid3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAxisymFluid3Table>().idColumn()); });
    }
    {
        using model::AcousticAxisymFluid4Table;
        const std::uint16_t type = model::typeIndexOf<AcousticAxisymFluid4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAxisymFluid4Table>().idColumn()); });
    }
    {
        using model::ShellAxisymTable;
        const std::uint16_t type = model::typeIndexOf<ShellAxisymTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellAxisymTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellAxisymTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellAxisymTable>().nodesColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellAxisymTable>().offsetColumn()); });
    }
    {
        using model::Bar2Table;
        const std::uint16_t type = model::typeIndexOf<Bar2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().orientationColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "offsetKind", [](const auto& model)
                       { return exportTextColumn(model->template table<Bar2Table>().offsetKindColumn()); });
        registerColumn(catalogue, type, "pinFlagsA", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().pinFlagsAColumn()); });
        registerColumn(catalogue, type, "pinFlagsB", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().pinFlagsBColumn()); });
        registerColumn(catalogue, type, "offsetA", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().offsetAColumn()); });
        registerColumn(catalogue, type, "offsetB", [](const auto& model)
                       { return exportColumn(model->template table<Bar2Table>().offsetBColumn()); });
    }
    {
        using model::Bar2StationsTable;
        const std::uint16_t type = model::typeIndexOf<Bar2StationsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Bar2StationsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stationScale", [](const auto& model)
            { return exportTextColumn(model->template table<Bar2StationsTable>().stationScaleColumn()); });
        registerColumn(
            catalogue, type, "listedStations", [](const auto& model)
            { return exportVariableColumn(model->template table<Bar2StationsTable>().listedStationsColumn()); });
        registerColumn(catalogue, type, "stationCount", [](const auto& model)
                       { return exportColumn(model->template table<Bar2StationsTable>().stationCountColumn()); });
        registerColumn(catalogue, type, "station1", [](const auto& model)
                       { return exportColumn(model->template table<Bar2StationsTable>().firstColumn()); });
        registerColumn(catalogue, type, "stationSpacing", [](const auto& model)
                       { return exportColumn(model->template table<Bar2StationsTable>().stationSpacingColumn()); });
    }
    {
        using model::Beam2Table;
        const std::uint16_t type = model::typeIndexOf<Beam2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().orientationColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "offsetKind", [](const auto& model)
                       { return exportTextColumn(model->template table<Beam2Table>().offsetKindColumn()); });
        registerColumn(catalogue, type, "pinFlagsA", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().pinFlagsAColumn()); });
        registerColumn(catalogue, type, "pinFlagsB", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().pinFlagsBColumn()); });
        registerColumn(catalogue, type, "offsetA", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().offsetAColumn()); });
        registerColumn(catalogue, type, "offsetB", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().offsetBColumn()); });
        registerColumn(catalogue, type, "warpingPointA", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().warpingPointAColumn()); });
        registerColumn(catalogue, type, "warpingPointB", [](const auto& model)
                       { return exportColumn(model->template table<Beam2Table>().warpingPointBColumn()); });
    }
    {
        using model::Beam3Table;
        const std::uint16_t type = model::typeIndexOf<Beam3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "nodeC", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().nodeCColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().orientationColumn()); });
        registerColumn(catalogue, type, "offsetA", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().offsetAColumn()); });
        registerColumn(catalogue, type, "offsetB", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().offsetBColumn()); });
        registerColumn(catalogue, type, "offsetC", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().offsetCColumn()); });
        registerColumn(catalogue, type, "twist", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().twistColumn()); });
        registerColumn(catalogue, type, "warpingPointA", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().warpingPointAColumn()); });
        registerColumn(catalogue, type, "warpingPointB", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().warpingPointBColumn()); });
        registerColumn(catalogue, type, "warpingPointC", [](const auto& model)
                       { return exportColumn(model->template table<Beam3Table>().warpingPointCColumn()); });
    }
    {
        using model::CurvedBeam2Table;
        const std::uint16_t type = model::typeIndexOf<CurvedBeam2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().orientationColumn()); });
        registerColumn(catalogue, type, "geometryOption", [](const auto& model)
                       { return exportColumn(model->template table<CurvedBeam2Table>().geometryOptionColumn()); });
    }
    {
        using model::BushingTable;
        const std::uint16_t type = model::typeIndexOf<BushingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().orientationColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "offsetFraction", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().offsetFractionColumn()); });
        registerColumn(catalogue, type, "offsetFrameId", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().offsetFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<BushingTable>().offsetColumn()); });
    }
    {
        using model::Bushing1DTable;
        const std::uint16_t type = model::typeIndexOf<Bushing1DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Bushing1DTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Bushing1DTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<Bushing1DTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<Bushing1DTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<Bushing1DTable>().frameIdColumn()); });
    }
    {
        using model::Bushing2DTable;
        const std::uint16_t type = model::typeIndexOf<Bushing2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Bushing2DTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Bushing2DTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<Bushing2DTable>().nodesColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<Bushing2DTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "plane", [](const auto& model)
                       { return exportTextColumn(model->template table<Bushing2DTable>().planeColumn()); });
    }
    {
        using model::ShellConical2Table;
        const std::uint16_t type = model::typeIndexOf<ShellConical2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellConical2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellConical2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "ringIdA", [](const auto& model)
                       { return exportColumn(model->template table<ShellConical2Table>().ringIdAColumn()); });
        registerColumn(catalogue, type, "ringIdB", [](const auto& model)
                       { return exportColumn(model->template table<ShellConical2Table>().ringIdBColumn()); });
    }
    {
        using model::DamperDofPropertyTable;
        const std::uint16_t type = model::typeIndexOf<DamperDofPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyTable>().components2Column()); });
    }
    {
        using model::DamperDofPropertyExplicitTable;
        const std::uint16_t type = model::typeIndexOf<DamperDofPropertyExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().components2Column()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "follow", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofPropertyExplicitTable>().followColumn()); });
    }
    {
        using model::DamperDofInlineTable;
        const std::uint16_t type = model::typeIndexOf<DamperDofInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().dampingColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineTable>().components2Column()); });
    }
    {
        using model::DamperDofInlineExplicitTable;
        const std::uint16_t type = model::typeIndexOf<DamperDofInlineExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().idColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().dampingColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().components2Column()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "follow", [](const auto& model)
                       { return exportColumn(model->template table<DamperDofInlineExplicitTable>().followColumn()); });
    }
    {
        using model::DamperScalarPropertyTable;
        const std::uint16_t type = model::typeIndexOf<DamperScalarPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarPropertyTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarPropertyTable>().scalarPoint2Column()); });
    }
    {
        using model::DamperScalarInlineTable;
        const std::uint16_t type = model::typeIndexOf<DamperScalarInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarInlineTable>().dampingColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarInlineTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<DamperScalarInlineTable>().scalarPoint2Column()); });
    }
    {
        using model::HeatCapacity2Table;
        const std::uint16_t type = model::typeIndexOf<HeatCapacity2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacity2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacity2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacity2Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<HeatCapacity2Table>().node2Column()); });
    }
    {
        using model::UserDefined1Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined1Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined1Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined1Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined1Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined1Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined2Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined2Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined2Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined2Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined3Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined3Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined3Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined3Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined4Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined4Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined4Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined4Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined5Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined5Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined5Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined5Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined5Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined5Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined6Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined6Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined6Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined6Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined6Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined6Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined7Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined7Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined7Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined7Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined7Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined7Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined7Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined8Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined8Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined8Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined8Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined8Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined8Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined8Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::UserDefined9Table;
        const std::uint16_t type = model::typeIndexOf<UserDefined9Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined9Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined9Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportVariableColumn(model->template table<UserDefined9Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined9Table>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined9Table>().fieldColumns().numberColumn()); });
    }
    {
        using model::SpringDofPropertyTable;
        const std::uint16_t type = model::typeIndexOf<SpringDofPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyTable>().components2Column()); });
    }
    {
        using model::SpringDofPropertyExplicitTable;
        const std::uint16_t type = model::typeIndexOf<SpringDofPropertyExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().components2Column()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "follow", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofPropertyExplicitTable>().followColumn()); });
    }
    {
        using model::SpringDofInlineTable;
        const std::uint16_t type = model::typeIndexOf<SpringDofInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().components2Column()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "stressCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineTable>().stressCoefficientColumn()); });
    }
    {
        using model::SpringDofInlineExplicitTable;
        const std::uint16_t type = model::typeIndexOf<SpringDofInlineExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().components2Column()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "follow", [](const auto& model)
                       { return exportColumn(model->template table<SpringDofInlineExplicitTable>().followColumn()); });
    }
    {
        using model::SpringScalarPropertyTable;
        const std::uint16_t type = model::typeIndexOf<SpringScalarPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarPropertyTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarPropertyTable>().scalarPoint2Column()); });
    }
    {
        using model::SpringScalarInlineTable;
        const std::uint16_t type = model::typeIndexOf<SpringScalarInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarInlineTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarInlineTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<SpringScalarInlineTable>().scalarPoint2Column()); });
    }
    {
        using model::FastenerTable;
        const std::uint16_t type = model::typeIndexOf<FastenerTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().propIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<FastenerTable>().kindColumn()); });
        registerColumn(catalogue, type, "patchIdA", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().patchIdAColumn()); });
        registerColumn(catalogue, type, "patchIdB", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().patchIdBColumn()); });
        registerColumn(catalogue, type, "spotNode", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().spotNodeColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "location", [](const auto& model)
                       { return exportColumn(model->template table<FastenerTable>().locationColumn()); });
    }
    {
        using model::Gap2Table;
        const std::uint16_t type = model::typeIndexOf<Gap2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "orientationNode", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().orientationNodeColumn()); });
        registerColumn(catalogue, type, "orientation", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().orientationColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<Gap2Table>().frameIdColumn()); });
    }
    {
        using model::ElemGenerationTable;
        const std::uint16_t type = model::typeIndexOf<ElemGenerationTable>();
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<ElemGenerationTable>().kindColumn()); });
        registerColumn(catalogue, type, "firstElemId", [](const auto& model)
                       { return exportColumn(model->template table<ElemGenerationTable>().firstElemIdColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ElemGenerationTable>().propIdColumn()); });
        registerColumn(catalogue, type, "gridFieldId", [](const auto& model)
                       { return exportColumn(model->template table<ElemGenerationTable>().gridFieldIdColumn()); });
    }
    {
        using model::SolidHexaTable;
        const std::uint16_t type = model::typeIndexOf<SolidHexaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidHexaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SolidHexaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SolidHexaTable>().nodesColumn()); });
    }
    {
        using model::CohesiveHexaTable;
        const std::uint16_t type = model::typeIndexOf<CohesiveHexaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveHexaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveHexaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveHexaTable>().nodesColumn()); });
    }
    {
        using model::CohesivePentaTable;
        const std::uint16_t type = model::typeIndexOf<CohesivePentaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesivePentaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CohesivePentaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CohesivePentaTable>().nodesColumn()); });
    }
    {
        using model::CohesiveAxisymQuadTable;
        const std::uint16_t type = model::typeIndexOf<CohesiveAxisymQuadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveAxisymQuadTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveAxisymQuadTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveAxisymQuadTable>().nodesColumn()); });
    }
    {
        using model::CohesiveQuadTable;
        const std::uint16_t type = model::typeIndexOf<CohesiveQuadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveQuadTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveQuadTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveQuadTable>().nodesColumn()); });
    }
    {
        using model::LineInterfaceTable;
        const std::uint16_t type = model::typeIndexOf<LineInterfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LineInterfaceTable>().idColumn()); });
    }
    {
        using model::MassDofPropertyTable;
        const std::uint16_t type = model::typeIndexOf<MassDofPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<MassDofPropertyTable>().components2Column()); });
    }
    {
        using model::MassDofInlineTable;
        const std::uint16_t type = model::typeIndexOf<MassDofInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().massColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().node1Column()); });
        registerColumn(catalogue, type, "components1", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().components1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().node2Column()); });
        registerColumn(catalogue, type, "components2", [](const auto& model)
                       { return exportColumn(model->template table<MassDofInlineTable>().components2Column()); });
    }
    {
        using model::MassScalarPropertyTable;
        const std::uint16_t type = model::typeIndexOf<MassScalarPropertyTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarPropertyTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarPropertyTable>().propIdColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarPropertyTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarPropertyTable>().scalarPoint2Column()); });
    }
    {
        using model::MassScalarInlineTable;
        const std::uint16_t type = model::typeIndexOf<MassScalarInlineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarInlineTable>().idColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarInlineTable>().massColumn()); });
        registerColumn(catalogue, type, "scalarPoint1", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarInlineTable>().scalarPoint1Column()); });
        registerColumn(catalogue, type, "scalarPoint2", [](const auto& model)
                       { return exportColumn(model->template table<MassScalarInlineTable>().scalarPoint2Column()); });
    }
    {
        using model::PointMassMatrixTable;
        const std::uint16_t type = model::typeIndexOf<PointMassMatrixTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointMassMatrixTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointMassMatrixTable>().nodeColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<PointMassMatrixTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "massMatrix", [](const auto& model)
                       { return exportColumn(model->template table<PointMassMatrixTable>().massMatrixColumn()); });
    }
    {
        using model::PointMassInertiaTable;
        const std::uint16_t type = model::typeIndexOf<PointMassInertiaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().nodeColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().massColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().offsetColumn()); });
        registerColumn(catalogue, type, "inertia", [](const auto& model)
                       { return exportColumn(model->template table<PointMassInertiaTable>().inertiaColumn()); });
    }
    {
        using model::Rod2InlineSectionTable;
        const std::uint16_t type = model::typeIndexOf<Rod2InlineSectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().nodesColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().matIdColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().areaColumn()); });
        registerColumn(catalogue, type, "torsionConstant", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().torsionConstantColumn()); });
        registerColumn(
            catalogue, type, "torsionStressCoefficient", [](const auto& model)
            { return exportColumn(model->template table<Rod2InlineSectionTable>().torsionStressCoefficientColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<Rod2InlineSectionTable>().nonStructMassColumn()); });
    }
    {
        using model::SolidPentaTable;
        const std::uint16_t type = model::typeIndexOf<SolidPentaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidPentaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SolidPentaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SolidPentaTable>().nodesColumn()); });
    }
    {
        using model::PlaneStrainTria3Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStrainTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria3Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria3Table>().thetaInDegreesColumn()); });
    }
    {
        using model::PlaneStrainQuad4Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStrainQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad4Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad4Table>().thetaInDegreesColumn()); });
    }
    {
        using model::PlaneStrainTria6Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStrainTria6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria6Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria6Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria6Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainTria6Table>().thetaInDegreesColumn()); });
    }
    {
        using model::PlaneStrainQuad8Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStrainQuad8Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad8Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad8Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad8Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuad8Table>().thetaInDegreesColumn()); });
    }
    {
        using model::PlaneStressTria3Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStressTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria3Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria3Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria3Table>().thicknessFlagColumn()); });
        registerColumn(
            catalogue, type, "cornerThickness", [](const auto& model)
            { return exportColumn(model->template table<PlaneStressTria3Table>().cornerThicknessColumn()); });
    }
    {
        using model::PlaneStressQuad4Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStressQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad4Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad4Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad4Table>().thicknessFlagColumn()); });
        registerColumn(
            catalogue, type, "cornerThickness", [](const auto& model)
            { return exportColumn(model->template table<PlaneStressQuad4Table>().cornerThicknessColumn()); });
    }
    {
        using model::PlaneStressTria6Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStressTria6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria6Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria6Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria6Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressTria6Table>().thetaInDegreesColumn()); });
    }
    {
        using model::PlaneStressQuad8Table;
        const std::uint16_t type = model::typeIndexOf<PlaneStressQuad8Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad8Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad8Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad8Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStressQuad8Table>().thetaInDegreesColumn()); });
    }
    {
        using model::SolidPyramidTable;
        const std::uint16_t type = model::typeIndexOf<SolidPyramidTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidPyramidTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SolidPyramidTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SolidPyramidTable>().nodesColumn()); });
    }
    {
        using model::PlaneStrainQuadTable;
        const std::uint16_t type = model::typeIndexOf<PlaneStrainQuadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuadTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuadTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuadTable>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuadTable>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<PlaneStrainQuadTable>().matFrameIdColumn()); });
    }
    {
        using model::ShellQuad4Table;
        const std::uint16_t type = model::typeIndexOf<ShellQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().thicknessFlagColumn()); });
        registerColumn(catalogue, type, "cornerThickness", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4Table>().cornerThicknessColumn()); });
    }
    {
        using model::ShellQuad8Table;
        const std::uint16_t type = model::typeIndexOf<ShellQuad8Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().thicknessFlagColumn()); });
        registerColumn(catalogue, type, "cornerThickness", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad8Table>().cornerThicknessColumn()); });
    }
    {
        using model::ShellQuad4DrillingTable;
        const std::uint16_t type = model::typeIndexOf<ShellQuad4DrillingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellQuad4DrillingTable>().thicknessFlagColumn()); });
        registerColumn(
            catalogue, type, "cornerThickness", [](const auto& model)
            { return exportColumn(model->template table<ShellQuad4DrillingTable>().cornerThicknessColumn()); });
    }
    {
        using model::AxisymQuadTable;
        const std::uint16_t type = model::typeIndexOf<AxisymQuadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuadTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuadTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuadTable>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuadTable>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuadTable>().matFrameIdColumn()); });
    }
    {
        using model::AxisymQuad4Table;
        const std::uint16_t type = model::typeIndexOf<AxisymQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad4Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad4Table>().thetaInDegreesColumn()); });
    }
    {
        using model::AxisymQuad8Table;
        const std::uint16_t type = model::typeIndexOf<AxisymQuad8Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad8Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad8Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad8Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymQuad8Table>().thetaInDegreesColumn()); });
    }
    {
        using model::CrackTip2DTable;
        const std::uint16_t type = model::typeIndexOf<CrackTip2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip2DTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip2DTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip2DTable>().nodesColumn()); });
    }
    {
        using model::CrackTip3DTable;
        const std::uint16_t type = model::typeIndexOf<CrackTip3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip3DTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip3DTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CrackTip3DTable>().nodesColumn()); });
    }
    {
        using model::Rod2Table;
        const std::uint16_t type = model::typeIndexOf<Rod2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Rod2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Rod2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<Rod2Table>().nodesColumn()); });
    }
    {
        using model::SeamTable;
        const std::uint16_t type = model::typeIndexOf<SeamTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().propIdColumn()); });
        registerColumn(catalogue, type, "seamLine", [](const auto& model)
                       { return exportTextColumn(model->template table<SeamTable>().seamLineColumn()); });
        registerColumn(catalogue, type, "patchKind", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().patchKindColumn()); });
        registerColumn(catalogue, type, "patchAStart", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().patchAStartColumn()); });
        registerColumn(catalogue, type, "patchBStart", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().patchBStartColumn()); });
        registerColumn(catalogue, type, "patchAEnd", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().patchAEndColumn()); });
        registerColumn(catalogue, type, "patchBEnd", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().patchBEndColumn()); });
        registerColumn(catalogue, type, "ends", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().endsColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().nodesColumn()); });
        registerColumn(catalogue, type, "start", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().startColumn()); });
        registerColumn(catalogue, type, "end", [](const auto& model)
                       { return exportColumn(model->template table<SeamTable>().endColumn()); });
    }
    {
        using model::ShearPanel4Table;
        const std::uint16_t type = model::typeIndexOf<ShearPanel4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanel4Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanel4Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShearPanel4Table>().nodesColumn()); });
    }
    {
        using model::AcousticSlot3Table;
        const std::uint16_t type = model::typeIndexOf<AcousticSlot3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot3Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot3Table>().nodesColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot3Table>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot3Table>().bulkModulusColumn()); });
        registerColumn(catalogue, type, "slotCount", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot3Table>().slotCountColumn()); });
    }
    {
        using model::AcousticSlot4Table;
        const std::uint16_t type = model::typeIndexOf<AcousticSlot4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot4Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot4Table>().nodesColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot4Table>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot4Table>().bulkModulusColumn()); });
        registerColumn(catalogue, type, "slotCount", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSlot4Table>().slotCountColumn()); });
    }
    {
        using model::SpringOffsetExplicitTable;
        const std::uint16_t type = model::typeIndexOf<SpringOffsetExplicitTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetExplicitTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetExplicitTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SpringOffsetExplicitTable>().nodesColumn()); });
    }
    {
        using model::SolidTetraTable;
        const std::uint16_t type = model::typeIndexOf<SolidTetraTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidTetraTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<SolidTetraTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SolidTetraTable>().nodesColumn()); });
    }
    {
        using model::ShellTria3Table;
        const std::uint16_t type = model::typeIndexOf<ShellTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().thicknessFlagColumn()); });
        registerColumn(catalogue, type, "cornerThickness", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3Table>().cornerThicknessColumn()); });
    }
    {
        using model::ShellTria6Table;
        const std::uint16_t type = model::typeIndexOf<ShellTria6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().thicknessFlagColumn()); });
        registerColumn(catalogue, type, "cornerThickness", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria6Table>().cornerThicknessColumn()); });
    }
    {
        using model::ShellTria3DrillingTable;
        const std::uint16_t type = model::typeIndexOf<ShellTria3DrillingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "offset", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().offsetColumn()); });
        registerColumn(catalogue, type, "thicknessFlag", [](const auto& model)
                       { return exportColumn(model->template table<ShellTria3DrillingTable>().thicknessFlagColumn()); });
        registerColumn(
            catalogue, type, "cornerThickness", [](const auto& model)
            { return exportColumn(model->template table<ShellTria3DrillingTable>().cornerThicknessColumn()); });
    }
    {
        using model::AxisymTriaTable;
        const std::uint16_t type = model::typeIndexOf<AxisymTriaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTriaTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTriaTable>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTriaTable>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTriaTable>().thetaInDegreesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTriaTable>().matFrameIdColumn()); });
    }
    {
        using model::AxisymTria6Table;
        const std::uint16_t type = model::typeIndexOf<AxisymTria6Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTria6Table>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTria6Table>().matIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTria6Table>().nodesColumn()); });
        registerColumn(catalogue, type, "materialAngle", [](const auto& model)
                       { return exportColumn(model->template table<AxisymTria6Table>().thetaInDegreesColumn()); });
    }
    {
        using model::Tube2Table;
        const std::uint16_t type = model::typeIndexOf<Tube2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<Tube2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<Tube2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<Tube2Table>().nodesColumn()); });
    }
    {
        using model::ViscousDamper2Table;
        const std::uint16_t type = model::typeIndexOf<ViscousDamper2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamper2Table>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamper2Table>().propIdColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamper2Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<ViscousDamper2Table>().node2Column()); });
    }
    {
        using model::WeldTable;
        const std::uint16_t type = model::typeIndexOf<WeldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().propIdColumn()); });
        registerColumn(catalogue, type, "spotNode", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().spotNodeColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<WeldTable>().kindColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "patchShapes", [](const auto& model)
                       { return exportTextColumn(model->template table<WeldTable>().patchShapesColumn()); });
        registerColumn(catalogue, type, "matFrameId", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().matFrameIdColumn()); });
        registerColumn(catalogue, type, "patchIdA", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().patchIdAColumn()); });
        registerColumn(catalogue, type, "patchIdB", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().patchIdBColumn()); });
        registerColumn(catalogue, type, "location", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().locationColumn()); });
        registerColumn(catalogue, type, "patchNodesA", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().patchNodesAColumn()); });
        registerColumn(catalogue, type, "patchNodesB", [](const auto& model)
                       { return exportColumn(model->template table<WeldTable>().patchNodesBColumn()); });
    }
    {
        using model::GeneralMatrixTable;
        const std::uint16_t type = model::typeIndexOf<GeneralMatrixTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GeneralMatrixTable>().idColumn()); });
        registerColumn(
            catalogue, type, "independentDofs", [](const auto& model)
            { return exportVariableColumn(model->template table<GeneralMatrixTable>().independentDofsColumn()); });
        registerColumn(
            catalogue, type, "dependentDofs", [](const auto& model)
            { return exportVariableColumn(model->template table<GeneralMatrixTable>().dependentDofsColumn()); });
        registerColumn(catalogue, type, "flexibility", [](const auto& model)
                       { return exportColumn(model->template table<GeneralMatrixTable>().flexibilityColumn()); });
        registerColumn(
            catalogue, type, "stiffnessMatrix", [](const auto& model)
            { return exportVariableColumn(model->template table<GeneralMatrixTable>().stiffnessMatrixColumn()); });
        registerColumn(
            catalogue, type, "dependencyMatrix", [](const auto& model)
            { return exportVariableColumn(model->template table<GeneralMatrixTable>().dependencyMatrixColumn()); });
    }
    {
        using model::CohesiveMatTable;
        const std::uint16_t type = model::typeIndexOf<CohesiveMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesiveMatTable>().idColumn()); });
    }
    {
        using model::AcousticAbsorberPropTable;
        const std::uint16_t type = model::typeIndexOf<AcousticAbsorberPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberPropTable>().idColumn()); });
        registerColumn(
            catalogue, type, "resistanceTableId", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberPropTable>().resistanceTableIdColumn()); });
        registerColumn(
            catalogue, type, "reactanceTableId", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberPropTable>().reactanceTableIdColumn()); });
        registerColumn(
            catalogue, type, "impedanceScale", [](const auto& model)
            { return exportColumn(model->template table<AcousticAbsorberPropTable>().impedanceScaleColumn()); });
        registerColumn(catalogue, type, "area", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberPropTable>().areaColumn()); });
        registerColumn(catalogue, type, "damping", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberPropTable>().dampingColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<AcousticAbsorberPropTable>().stiffnessColumn()); });
        registerColumn(
            catalogue, type, "characteristicImpedance",
            [](const auto& model)
            {
                return exportColumn(model->template table<AcousticAbsorberPropTable>().characteristicImpedanceColumn());
            });
    }
    {
        using model::AcousticInfinitePropTable;
        const std::uint16_t type = model::typeIndexOf<AcousticInfinitePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfinitePropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfinitePropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "radialOrder", [](const auto& model)
            { return exportColumn(model->template table<AcousticInfinitePropTable>().radialOrderColumn()); });
        registerColumn(catalogue, type, "poleX", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfinitePropTable>().poleXColumn()); });
        registerColumn(catalogue, type, "poleY", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfinitePropTable>().poleYColumn()); });
        registerColumn(catalogue, type, "poleZ", [](const auto& model)
                       { return exportColumn(model->template table<AcousticInfinitePropTable>().poleZColumn()); });
    }
    {
        using model::BeamPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<BeamPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamPropNonlinearTable>().idColumn()); });
    }
    {
        using model::BeamArbitrarySectionPropTable;
        const std::uint16_t type = model::typeIndexOf<BeamArbitrarySectionPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BeamArbitrarySectionPropTable>().idColumn()); });
    }
    {
        using model::BarArbitrarySectionPropTable;
        const std::uint16_t type = model::typeIndexOf<BarArbitrarySectionPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BarArbitrarySectionPropTable>().idColumn()); });
    }
    {
        using model::CohesivePropTable;
        const std::uint16_t type = model::typeIndexOf<CohesivePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CohesivePropTable>().idColumn()); });
    }
    {
        using model::ConicalShellPropTable;
        const std::uint16_t type = model::typeIndexOf<ConicalShellPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId1", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().matId1Column()); });
        registerColumn(catalogue, type, "thickness1", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().thickness1Column()); });
        registerColumn(catalogue, type, "matId2", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().matId2Column()); });
        registerColumn(catalogue, type, "inertia", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().inertiaColumn()); });
        registerColumn(catalogue, type, "matId3", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().matId3Column()); });
        registerColumn(catalogue, type, "thickness2", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().thickness2Column()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "fiberDistance1", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().fiberDistance1Column()); });
        registerColumn(catalogue, type, "fiberDistance2", [](const auto& model)
                       { return exportColumn(model->template table<ConicalShellPropTable>().fiberDistance2Column()); });
        registerColumn(
            catalogue, type, "phiAngle", [](const auto& model)
            { return exportVariableColumn(model->template table<ConicalShellPropTable>().phiAngleColumn()); });
    }
    {
        using model::UserDefined1PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined1PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined1PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined1PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined1PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined1PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined2PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined2PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined2PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined2PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined2PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined2PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined3PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined3PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined3PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined3PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined3PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined3PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined4PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined4PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined4PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined4PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined4PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined4PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined5PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined5PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined5PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined5PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined5PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined5PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined6PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined6PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined6PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined6PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined6PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined6PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined7PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined7PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined7PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined7PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined7PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined7PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined8PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined8PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined8PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined8PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined8PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined8PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::UserDefined9PropTable;
        const std::uint16_t type = model::typeIndexOf<UserDefined9PropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined9PropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<UserDefined9PropTable>().matIdColumn()); });
        registerColumn(
            catalogue, type, "fieldKinds", [](const auto& model)
            { return exportVariableColumn(model->template table<UserDefined9PropTable>().fieldColumns().kindColumn()); });
        registerColumn(
            catalogue, type, "fieldValues",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<UserDefined9PropTable>().fieldColumns().numberColumn());
            });
    }
    {
        using model::PlotLine2Table;
        const std::uint16_t type = model::typeIndexOf<PlotLine2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlotLine2Table>().idColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PlotLine2Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PlotLine2Table>().node2Column()); });
    }
    {
        using model::PlotTria3Table;
        const std::uint16_t type = model::typeIndexOf<PlotTria3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlotTria3Table>().idColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PlotTria3Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PlotTria3Table>().node2Column()); });
        registerColumn(catalogue, type, "node3", [](const auto& model)
                       { return exportColumn(model->template table<PlotTria3Table>().node3Column()); });
    }
    {
        using model::PlotQuad4Table;
        const std::uint16_t type = model::typeIndexOf<PlotQuad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlotQuad4Table>().idColumn()); });
        registerColumn(catalogue, type, "node1", [](const auto& model)
                       { return exportColumn(model->template table<PlotQuad4Table>().node1Column()); });
        registerColumn(catalogue, type, "node2", [](const auto& model)
                       { return exportColumn(model->template table<PlotQuad4Table>().node2Column()); });
        registerColumn(catalogue, type, "node3", [](const auto& model)
                       { return exportColumn(model->template table<PlotQuad4Table>().node3Column()); });
        registerColumn(catalogue, type, "node4", [](const auto& model)
                       { return exportColumn(model->template table<PlotQuad4Table>().node4Column()); });
    }
    {
        using model::PlotSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<PlotSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PlotSurfaceTable>().idColumn()); });
        registerColumn(catalogue, type, "grids", [](const auto& model)
                       { return exportColumn(model->template table<PlotSurfaceTable>().gridsColumn()); });
    }
    {
        using model::CrackTipProp2DTable;
        const std::uint16_t type = model::typeIndexOf<CrackTipProp2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().matIdColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "planeCondition", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().planeConditionColumn()); });
        registerColumn(catalogue, type, "nonStructMass", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().nonStructMassColumn()); });
        registerColumn(catalogue, type, "gammaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().gammaCoefficientColumn()); });
        registerColumn(catalogue, type, "phiAngle", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp2DTable>().phiAngleColumn()); });
    }
    {
        using model::CrackTipProp3DTable;
        const std::uint16_t type = model::typeIndexOf<CrackTipProp3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp3DTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp3DTable>().matIdColumn()); });
        registerColumn(catalogue, type, "gammaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp3DTable>().gammaCoefficientColumn()); });
        registerColumn(catalogue, type, "phiAngle", [](const auto& model)
                       { return exportColumn(model->template table<CrackTipProp3DTable>().phiAngleColumn()); });
    }
    {
        using model::SeamPropTable;
        const std::uint16_t type = model::typeIndexOf<SeamPropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SeamPropTable>().idColumn()); });
        registerColumn(catalogue, type, "matId", [](const auto& model)
                       { return exportColumn(model->template table<SeamPropTable>().matIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportTextColumn(model->template table<SeamPropTable>().kindColumn()); });
        registerColumn(catalogue, type, "width", [](const auto& model)
                       { return exportColumn(model->template table<SeamPropTable>().widthColumn()); });
        registerColumn(catalogue, type, "thickness", [](const auto& model)
                       { return exportColumn(model->template table<SeamPropTable>().thicknessColumn()); });
        registerColumn(catalogue, type, "integration", [](const auto& model)
                       { return exportColumn(model->template table<SeamPropTable>().integrationColumn()); });
    }
    {
        using model::SolidPropNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<SolidPropNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SolidPropNonlinearTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
