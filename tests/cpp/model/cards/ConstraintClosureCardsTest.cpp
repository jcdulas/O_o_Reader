// Checks of the remaining constraint cards: CYSUP (the fictitious supports of a cyclic symmetry model), GMBNDC (a
// geometric boundary along a curve), MPCD and SPCR (the load selectable values of the non-homogeneous constraints and
// of the relative enforced motion), MPCY (the non-homogeneous multipoint constraint), DEFUSET (the names of the sets of
// degrees of freedom), SET2 and SET4 (the lists of grid points over a macro element and of properties), MONCNCM (a
// monitor point of the strips of the CAERO1 panels) and PSHLN1 and PSHLN2 (the nonlinear extensions of the shell and
// plane properties). Per card: the fields in their format reference positions, the defaults, the references the card
// lists, the renumbering of the spaces it owns, the refused values and the round trip through a model file.

#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Table.hpp"
#include "oor/model/cards/constraints/cyclicSupport/CyclicSupportTable.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/geomBoundaryCurve/GeomBoundaryCurveTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrValue/MultiPointConstrValueTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrWithValue/MultiPointConstrWithValueTable.hpp"
#include "oor/model/cards/constraints/relativeEnforcedMotionConstr/RelativeEnforcedMotionConstrTable.hpp"
#include "oor/model/cards/matrices/monitorChordNormalCoeffs/MonitorChordNormalCoeffsTable.hpp"
#include "oor/model/cards/properties/planePropNonlinearExt/PlanePropNonlinearExtTable.hpp"
#include "oor/model/cards/properties/shellPropNonlinear/ShellPropNonlinearTable.hpp"
#include "oor/model/cards/sets/aeroPrismNodeSet2/AeroPrismNodeSet2Table.hpp"
#include "oor/model/cards/sets/dofSetRenames/DofSetRenamesTable.hpp"
#include "oor/model/cards/sets/propertyListSet4/PropertyListSet4Table.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The references that name nothing in a model that was just read (the support function takes a model that stays).
std::size_t unresolvedIn(Model&& model)
{
    return testing::unresolvedIn(model);
}

ShellWords words(ShellBehavior behavior, ShellIntegration integration)
{
    return ShellWords{behavior, integration};
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// CYSUP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CYSUP supports the components of a grid point and follows its renumbering",
          "[model][cards][constraint-closure]")
{
    Model model = readBulk("Node;2316\nNode;2317\nCyclicSupport;2316;6531\nCyclicSupport;2317;4");
    const CyclicSupportCard sampleCard = cardAt<CyclicSupportTable>(model, 0);
    REQUIRE((sampleCard.nodeId == 2316 && sampleCard.components == 1356)); // the digits are kept in ascending order
    REQUIRE(
        (cardAt<CyclicSupportTable>(model, 1).nodeId == 2317 && cardAt<CyclicSupportTable>(model, 1).components == 4));
    REQUIRE((CyclicSupportTable::INFO.category == CardCategory::Constraint &&
             CyclicSupportTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The fixed format of the Card layout.
    REQUIRE(cardAt<CyclicSupportTable>(readBulk("CyclicSupport;2316;1356")).components == 1356);

    RecordingVisitor visitor;
    model.table<CyclicSupportTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("CyclicSupport;2316;12")) == 1); // no grid point 2316

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(
        (cardAt<CyclicSupportTable>(model, 0).nodeId == 2416 && cardAt<CyclicSupportTable>(model, 1).nodeId == 2417));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("CyclicSupport;0;1", "nodeId");
    expectInvalid("CyclicSupport;2316", "components");
    expectInvalid("CyclicSupport;2316;0", "components");  // a support needs a component
    expectInvalid("CyclicSupport;2316;17", "components"); // the digits are 1 to 6
}

// ---------------------------------------------------------------------------------------------------------------------
// GMBNDC
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// MPCD and SPCR
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an SPCR keeps the digits of its components and shares the load set of an SPCD",
          "[model][cards][constraint-closure]")
{
    Model model = readBulk(
        "Node;2432\n"
        "Node;2405\n"
        "RelativeEnforcedMotionConstr;2501;2432;253;-1.75;2405;;3.25\n"
        "RelativeEnforcedMotionConstr;2502;1\n"
        "RelativeEnforcedMotionConstr;2503;2432;6321;0.5;2405;45;-0.5\n"
        "EnforcedMotionConstr;2501;2432;1;0.");
    const RelativeEnforcedMotionConstrCard sampleCard = cardAt<RelativeEnforcedMotionConstrTable>(model, 0);
    REQUIRE((sampleCard.id == 2501 && sampleCard.nodes == std::array<base::EntityId, 2>{2432, 2405}));
    REQUIRE((sampleCard.components == std::array<std::int32_t, 2>{235, 0} &&
             sampleCard.enforced == std::array<double, 2>{-1.75, 3.25}));
    REQUIRE(sampleCard.hasSecondGroup());
    const RelativeEnforcedMotionConstrCard minimal = cardAt<RelativeEnforcedMotionConstrTable>(model, 1);
    REQUIRE((minimal.nodes[0] == 1 && minimal.components[0] == 0 && minimal.enforced[0] == 0.0));
    REQUIRE_FALSE(minimal.hasSecondGroup());
    REQUIRE((cardAt<RelativeEnforcedMotionConstrTable>(model, 2).components == std::array<std::int32_t, 2>{1236, 45}));
    REQUIRE((RelativeEnforcedMotionConstrTable::INFO.category == CardCategory::Load &&
             RelativeEnforcedMotionConstrTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // An SPCR and an SPCD may have the same SID: they are cards of one load set.
    REQUIRE_NOTHROW(
        readBulk("RelativeEnforcedMotionConstr;2501;2432;1;0.\nEnforcedMotionConstr;2501;2433;1;0."
                 "\nRelativeEnforcedMotionConstr;2501;2434;1;0."));

    RecordingVisitor visitor;
    model.table<RelativeEnforcedMotionConstrTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::ScalarOrGridPoint));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE((cardAt<RelativeEnforcedMotionConstrTable>(model, 0).id == 3501 &&
             cardAt<EnforcedMotionConstrTable>(model, 0).id == 3501));

    expectInvalid("RelativeEnforcedMotionConstr;0;1", "id");
    expectInvalid("RelativeEnforcedMotionConstr;1", "node1");
    expectInvalid("RelativeEnforcedMotionConstr;1;0", "node1");
    expectInvalid("RelativeEnforcedMotionConstr;1;5;17", "components1");
    expectInvalid("RelativeEnforcedMotionConstr;1;5;1;xyz", "enforcedValue1");
    expectInvalid("RelativeEnforcedMotionConstr;1;5;1;2.;;3", "node2");
    expectInvalid("RelativeEnforcedMotionConstr;1;5;1;2.;6;17", "components2");
}

// ---------------------------------------------------------------------------------------------------------------------
// MPCY
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// DEFUSET
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// SET2
// ---------------------------------------------------------------------------------------------------------------------

namespace
{

// A panel, a spline that names a set of the panel and another that names a set nothing defines.
const std::string PANEL_AND_SPLINES =
    "AeroLiftingPanelProp1;6001\nAeroLiftingPanel1;1000;6001;;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.\n";

} // namespace

TEST_CASE("a SET2 names a CAERO and a SPLINEi finds it as a set", "[model][cards][constraint-closure]")
{
    Model model =
        readBulk(PANEL_AND_SPLINES + "AeroPrismNodeSet2;5;1000;0.;1.;0.;1.\nAeroSurfaceSpline1;3;1000;1001;1002;5");
    RecordingVisitor visitor;
    model.table<AeroPrismNodeSet2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroElement});
    // The spline resolves its structural set among the SET1, SET2, SET3 and SET4 cards.
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk(PANEL_AND_SPLINES +
                                  "AeroPrismNodeSet2;5;1000;0.;1.;0.;1.\nAeroSurfaceSpline1;3;1000;1001;1002;6")) == 1);
    REQUIRE(unresolvedIn(readBulk(PANEL_AND_SPLINES + "AeroPrismNodeSet2;5;999;0.;1.;0.;1.")) == 1); // no CAERO 999

    // The set space renumbers the SID and the spline that names it; the aerodynamic elements the macro.
    REQUIRE(Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10)) == 1);
    REQUIRE((cardAt<AeroPrismNodeSet2Table>(model, 0).id == 15 &&
             cardAt<AeroSurfaceSpline1Table>(model, 0).nodeSetId == 15));
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(10000));
    REQUIRE((cardAt<AeroPrismNodeSet2Table>(model, 0).aeroPanelId == 11000 &&
             cardAt<AeroSurfaceSpline1Table>(model, 0).aeroPanelId == 11000));
    REQUIRE(unresolvedIn(model) == 0);

    // The SID may be shared with a SET1 or a SET3: they are cards of one set space.
    REQUIRE_NOTHROW(readBulk("IdListSet1;5;1;2\nAeroPrismNodeSet2;5;1000;0.;1.;0.;1.\nTypedListSet3;5;Node;3"));
}

// ---------------------------------------------------------------------------------------------------------------------
// SET4
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// MONCNCM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MONCNCM has its label over fields 2 to 8, MREF and the CAERO1 list on the second line",
          "[model][cards][constraint-closure]")
{
    Model model = readBulk(PANEL_AND_SPLINES +
                           "AeroLiftingPanel1;3001;6001;;2;1;;;1;0.;0.;0.;1.;0.;1.;0.;1.\n"
                           "MonitorChordNormalCoeffs;G87LEFT;Strips of the left wing;;2001;3001\n"
                           "MonitorChordNormalCoeffs;G87FULL;Right wing;0.3;1000;3001\n"
                           "MonitorChordNormalCoeffs;G87ALL;All strips;;All");
    const MonitorChordNormalCoeffsCard sampleCard = cardAt<MonitorChordNormalCoeffsTable>(model, 0);
    REQUIRE((sampleCard.name == "G87LEFT" && sampleCard.label == "Strips of the left wing"));
    REQUIRE((sampleCard.momentChordFraction == 0.25 && !sampleCard.allStrips &&
             sampleCard.aeroPanelIds == std::vector<base::EntityId>{2001, 3001}));
    const MonitorChordNormalCoeffsCard full = cardAt<MonitorChordNormalCoeffsTable>(model, 1);
    REQUIRE((full.label == "Right wing" && full.momentChordFraction == 0.3 &&
             full.aeroPanelIds == std::vector<base::EntityId>{1000, 3001}));
    const MonitorChordNormalCoeffsCard all = cardAt<MonitorChordNormalCoeffsTable>(model, 2);
    REQUIRE((all.allStrips && all.aeroPanelIds.empty() && all.momentChordFraction == 0.25));
    REQUIRE((MonitorChordNormalCoeffsTable::INFO.category == CardCategory::MonitorPoint &&
             MonitorChordNormalCoeffsTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The small format puts the label in the fields 2 to 8 and MREF in the first field of the second line.

    // The CAERO1 elements are references to aerodynamic elements; ALL lists none.
    RecordingVisitor visitor;
    model.table<MonitorChordNormalCoeffsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroPanelIds", "aeroPanelIds"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::AeroElement));
    visitor = RecordingVisitor{};
    model.table<MonitorChordNormalCoeffsTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 1); // there is no CAERO 2001
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(10000));
    REQUIRE(cardAt<MonitorChordNormalCoeffsTable>(model, 1).aeroPanelIds == std::vector<base::EntityId>{11000, 13001});

    // The name is unique among the MONCNCM cards.
    const Model twice = readModelText("MonitorChordNormalCoeffs;A;;;1\nMonitorChordNormalCoeffs;A;;;2\n");
    REQUIRE(twice.diagnostics().hasErrors());
    CHECK(twice.diagnostics().firstError()->code == "deck.duplicate-monitor-chord-normal-coeffs");

    expectInvalid("MonitorChordNormalCoeffs;;label;;5", "entryName");
    REQUIRE_NOTHROW(readBulk("MonitorChordNormalCoeffs;TOOLONGNAME;label;;5"));
    expectInvalid("MonitorChordNormalCoeffs;A;label;1.5;5", "momentChordFraction");
    expectInvalid("MonitorChordNormalCoeffs;A;label;-0.1;5", "momentChordFraction");
    expectInvalid("MonitorChordNormalCoeffs;A;label", "aeroPanelIds"); // a list or ALL is required
    expectInvalid("MonitorChordNormalCoeffs;A;label;;0", "aeroPanelIds");
    expectInvalid("MonitorChordNormalCoeffs;A;label;;xyz", "aeroPanelIds");
    expectInvalid("MonitorChordNormalCoeffs;A;label;;All;5", "aeroPanelIds");
    expectInvalid("MonitorChordNormalCoeffs;A;label;;5;All", "aeroPanelIds");
}

// ---------------------------------------------------------------------------------------------------------------------
// PSHLN1
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PSHLN1 gives each keyword line the defaults of its keyword and keeps the words it is given",
          "[model][cards][constraint-closure]")
{
    Model model = readBulk(
        "IsoMat;4350;2.1E+11;;0.3\n"
        "IsoMat;4351;2.1E+11;;0.3\n"
        "ShellProp;4311;4350;0.1\n"
        "ShellProp;4312;4350;0.1\n"
        "ShellProp;4313;4350;0.1\n"
        "ShellProp;4314;4350;0.1\n"
        "ShellProp;4315;4350;0.1\n"
        "ShellPropNonlinear;4311;4350;4351;ImplicitStructural;ThreeNodeElems;Membrane;Linear;Membrane;Linear;"
        "FourNodeElems;ThinShell;LinearDiscreteKirchhoff;;;EightNodeElems;Membrane\n"
        "ShellPropNonlinear;4312\n"
        "ShellPropNonlinear;4313;;;;ThreeNodeElems;ThinShell\n"
        "ShellPropNonlinear;4314;;;ImplicitHeat;FourNodeElems;Membrane;;Membrane\n"
        "ShellPropNonlinear;4315;;;;FourNodeElems;ThickShell;LinearReducedHourglass;;;EightNodeElems;ThickShell;"
        "QuadraticReduced;ThickShell;Quadratic");
    const ShellPropNonlinearCard full = cardAt<ShellPropNonlinearTable>(model, 0);
    REQUIRE((full.id == 4311 && full.matId1 == 4350 && full.matId2 == 4351 &&
             full.analysisType == PropertyAnalysis::Structural));
    REQUIRE(full.lines[SHELL_PROP_NONLINEAR_C3] == ShellLine{words(ShellBehavior::Membrane, ShellIntegration::Linear),
                                                             words(ShellBehavior::Membrane, ShellIntegration::Linear)});
    // The heat words of the C4 line are blank: the defaults of the keyword.
    REQUIRE(full.lines[SHELL_PROP_NONLINEAR_C4] ==
            ShellLine{words(ShellBehavior::ThinShell, ShellIntegration::LinearDiscreteKirchhoff),
                      words(ShellBehavior::ThickShell, ShellIntegration::Linear)});
    // A line that is not there has the defaults, and a blank integration is the one of the table for the behaviour:
    // MB on the eight-node quadrilateral has Q, where the default of the line (DCT) has QRI.
    REQUIRE(full.lines[SHELL_PROP_NONLINEAR_C6] == defaultLine(SHELL_PROP_NONLINEAR_LINES[SHELL_PROP_NONLINEAR_C6]));
    REQUIRE(full.lines[SHELL_PROP_NONLINEAR_C8] ==
            ShellLine{words(ShellBehavior::Membrane, ShellIntegration::Quadratic),
                      words(ShellBehavior::ThickShell, ShellIntegration::Quadratic)});

    const ShellPropNonlinearCard bare = cardAt<ShellPropNonlinearTable>(model, 1);
    REQUIRE((bare.matId1 == 0 && bare.matId2 == 0 && bare.analysisType == PropertyAnalysis::StructuralAndHeat));
    REQUIRE(bare.lines == defaultLines(SHELL_PROP_NONLINEAR_LINES));
    REQUIRE(bare.lines[SHELL_PROP_NONLINEAR_C3].structural ==
            words(ShellBehavior::ThinShell, ShellIntegration::LinearDiscreteKirchhoff)); // DCTN with LDK
    REQUIRE(bare.lines[SHELL_PROP_NONLINEAR_C4].structural ==
            words(ShellBehavior::ThickShell, ShellIntegration::Linear));
    REQUIRE(bare.lines[SHELL_PROP_NONLINEAR_C6].structural ==
            words(ShellBehavior::Membrane, ShellIntegration::Quadratic));
    REQUIRE(bare.lines[SHELL_PROP_NONLINEAR_C8].structural ==
            words(ShellBehavior::ThickShell, ShellIntegration::QuadraticReduced));
    REQUIRE(cardAt<ShellPropNonlinearTable>(model, 2).lines == bare.lines); // DCTN alone is the line it was
    const ShellPropNonlinearCard membrane = cardAt<ShellPropNonlinearTable>(model, 3);
    REQUIRE(membrane.analysisType == PropertyAnalysis::Heat);
    REQUIRE(membrane.lines[SHELL_PROP_NONLINEAR_C4] ==
            ShellLine{words(ShellBehavior::Membrane, ShellIntegration::Linear),
                      words(ShellBehavior::Membrane, ShellIntegration::Linear)});
    const ShellPropNonlinearCard thick = cardAt<ShellPropNonlinearTable>(model, 4);
    REQUIRE(thick.lines[SHELL_PROP_NONLINEAR_C4].structural ==
            words(ShellBehavior::ThickShell, ShellIntegration::LinearReducedHourglass));
    REQUIRE(thick.lines[SHELL_PROP_NONLINEAR_C8] ==
            defaultLine(SHELL_PROP_NONLINEAR_LINES[SHELL_PROP_NONLINEAR_C8])); // given as it is, so it is the default
    REQUIRE((ShellPropNonlinearTable::INFO.category == CardCategory::Property &&
             ShellPropNonlinearTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The lines are written when a word is not the default, with all their words; the default ones are left out, and a
    // writer that keeps defaults writes every line.
    REQUIRE(writeModelText(model, false).size() > writeModelText(model).size());
    oor::testing::requireModelFileRoundTrip(model, false);
    // The keyword lines are in any order.
    const ShellPropNonlinearCard shuffled = cardAt<ShellPropNonlinearTable>(
        readBulk("ShellPropNonlinear;22;;;;EightNodeElems;Membrane;;;;ThreeNodeElems;Membrane;Linear;Membrane;Linear"));
    REQUIRE((shuffled.lines[SHELL_PROP_NONLINEAR_C3].structural.behavior == ShellBehavior::Membrane &&
             shuffled.lines[SHELL_PROP_NONLINEAR_C8].structural.behavior == ShellBehavior::Membrane));

    // The ID is the one of the property it extends, in a space of its own; the materials are references.
    REQUIRE_NOTHROW(readBulk("ShellProp;5;1;0.1\nShellPropNonlinear;5"));
    REQUIRE_THROWS_AS(readBulk("ShellPropNonlinear;5\nShellPropNonlinear;5"), base::DeckError);
    RecordingVisitor visitor;
    model.table<ShellPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId1", "matId2"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material,
                                                            ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<ShellPropNonlinearTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("ShellPropNonlinear;4311;4350;4351")) == 3);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<ShellPropNonlinearTable>(model, 0).matId1 == 4450 &&
             cardAt<ShellPropNonlinearTable>(model, 0).matId2 == 4451));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<ShellPropNonlinearTable>(model, 0).id == 5311 && cardAt<ShellPropNonlinearTable>(model, 4).id == 5315));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("ShellPropNonlinear;0", "id");
    expectInvalid("ShellPropNonlinear;5;-1", "matId1");
    expectInvalid("ShellPropNonlinear;5;;-1", "matId2");
    expectInvalid("ShellPropNonlinear;5;;;XX", "analysisType");
    expectInvalid("ShellPropNonlinear;5;;;;ThreeNodeElems;Membrane;;;;ThreeNodeElems;Membrane",
                  "ThreeNodeElems"); // a keyword once
    expectInvalid("ShellPropNonlinear;5;;;;ThreeNodeElems;ThickShell",
                  "behavior3"); // no thick shell on the triangle with three grids
    expectInvalid("ShellPropNonlinear;5;;;;ThreeNodeElems;;Quadratic",
                  "integration3"); // no quadratic scheme on it either
    expectInvalid("ShellPropNonlinear;5;;;;FourNodeElems;ThinShell;LinearDiscreteKirchhoff;XX",
                  "heatBehavior4"); // a word the format has not
    expectInvalid("ShellPropNonlinear;5;;;;FourNodeElems;ThickShell;Linear;ThinShell",
                  "heatBehavior4"); // no thin shell for the heat
    expectInvalid("ShellPropNonlinear;5;;;;SixNodeElems;ThickShell",
                  "behavior6"); // the six-node triangle is a membrane
}

// ---------------------------------------------------------------------------------------------------------------------
// PSHLN2
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PSHLN2 has its MID, DIRECT and T before ANAL and takes the plane element tables for its lines",
          "[model][cards][constraint-closure]")
{
    Model model = readBulk(
        "IsoMat;4350;2.1E+11;;0.3\n"
        "PlanePropNonlinear;4311;4350\n"
        "PlanePropNonlinear;4312;4350\n"
        "PlanePropNonlinear;4313;4350\n"
        "PlanePropNonlinear;4314;4350\n"
        "PlanePropNonlinearExt;4311;4350;2;0.75;ImplicitStructural;ThreeNodeElems;AxisymSolid;Linear;AxisymSolid;"
        "Linear;FourNodeElems;PlaneStrainComposite;Linear;AxisymSolid;Linear;SixNodeElems;PlaneStress;Quadratic;"
        "AxisymSolid;Quadratic;EightNodeElems;"
        "AxisymSolid;QuadraticTwist;AxisymSolid;Quadratic\n"
        "PlanePropNonlinearExt;4312;4350\n"
        "PlanePropNonlinearExt;4313;4350;;;;FourNodeElems;PlaneStress;LinearReducedHourglass\n"
        "PlanePropNonlinearExt;4314;4350;;;;ThreeNodeElems;IncompressiblePlaneStrain;;;;EightNodeElems;AxisymSolid");
    const PlanePropNonlinearExtCard full = cardAt<PlanePropNonlinearExtTable>(model, 0);
    REQUIRE((full.id == 4311 && full.matId == 4350 && full.direction == 2 && full.thickness == 0.75));
    REQUIRE(full.analysisType == PropertyAnalysis::Structural);
    REQUIRE(full.lines[PLANE_PROP_NONLINEAR_EXT_C3] ==
            ShellLine{words(ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear),
                      words(ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear)});
    REQUIRE(full.lines[PLANE_PROP_NONLINEAR_EXT_C4].structural ==
            words(ShellBehavior::PlaneStrainComposite, ShellIntegration::Linear));
    REQUIRE(full.lines[PLANE_PROP_NONLINEAR_EXT_C6].structural ==
            words(ShellBehavior::PlaneStress, ShellIntegration::Quadratic));
    REQUIRE(full.lines[PLANE_PROP_NONLINEAR_EXT_C8].structural ==
            words(ShellBehavior::AxisymmetricSolid, ShellIntegration::QuadraticTwist));
    REQUIRE(full.lines[PLANE_PROP_NONLINEAR_EXT_C8].heat ==
            words(ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic));

    // The format reference defaults: DIRECT 1, T 1.0, ANAL ISH, and plane strain with L (L on the lower order elements,
    // Q on the quadratic ones) for every line.
    const PlanePropNonlinearExtCard bare = cardAt<PlanePropNonlinearExtTable>(model, 1);
    REQUIRE((bare.direction == 1 && bare.thickness == 1.0 && bare.analysisType == PropertyAnalysis::StructuralAndHeat));
    REQUIRE(bare.lines == defaultLines(PLANE_PROP_NONLINEAR_EXT_LINES));
    REQUIRE(bare.lines[PLANE_PROP_NONLINEAR_EXT_C3].structural ==
            words(ShellBehavior::PlaneStrain, ShellIntegration::Linear));
    REQUIRE(bare.lines[PLANE_PROP_NONLINEAR_EXT_C4].structural ==
            words(ShellBehavior::PlaneStrain, ShellIntegration::Linear));
    REQUIRE(bare.lines[PLANE_PROP_NONLINEAR_EXT_C6].structural ==
            words(ShellBehavior::PlaneStrain, ShellIntegration::Quadratic));
    REQUIRE(bare.lines[PLANE_PROP_NONLINEAR_EXT_C8].structural ==
            words(ShellBehavior::PlaneStrain, ShellIntegration::Quadratic));
    REQUIRE(bare.lines[PLANE_PROP_NONLINEAR_EXT_C4].heat ==
            words(ShellBehavior::PlaneStrain, ShellIntegration::Linear));
    // A blank integration is the first scheme of the table for the behaviour.
    REQUIRE(cardAt<PlanePropNonlinearExtTable>(model, 2).lines[PLANE_PROP_NONLINEAR_EXT_C4].structural ==
            words(ShellBehavior::PlaneStress, ShellIntegration::LinearReducedHourglass));
    REQUIRE(cardAt<PlanePropNonlinearExtTable>(model, 3).lines[PLANE_PROP_NONLINEAR_EXT_C3].structural ==
            words(ShellBehavior::IncompressiblePlaneStrain, ShellIntegration::Linear));
    REQUIRE(cardAt<PlanePropNonlinearExtTable>(model, 3).lines[PLANE_PROP_NONLINEAR_EXT_C8].structural ==
            words(ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic));
    REQUIRE((PlanePropNonlinearExtTable::INFO.category == CardCategory::Property &&
             PlanePropNonlinearExtTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    (void)0; // the line of the full card

    // The material is required; both it and the PLPLANE are references.
    REQUIRE_NOTHROW(readBulk("PlanePropNonlinear;5;1\nPlanePropNonlinearExt;5;2"));
    REQUIRE_THROWS_AS(readBulk("PlanePropNonlinearExt;5;1\nPlanePropNonlinearExt;5;1"), base::DeckError);
    RecordingVisitor visitor;
    model.table<PlanePropNonlinearExtTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("PlanePropNonlinearExt;4311;4350")) == 2);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<PlanePropNonlinearExtTable>(model, 0).matId == 4450);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<PlanePropNonlinearExtTable>(model, 0).id == 5311 &&
             cardAt<PlanePropNonlinearExtTable>(model, 3).id == 5314));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("PlanePropNonlinearExt;0;5", "id");
    expectInvalid("PlanePropNonlinearExt;5", "matId");
    expectInvalid("PlanePropNonlinearExt;5;0", "matId");
    expectInvalid("PlanePropNonlinearExt;5;9;0", "direction");
    expectInvalid("PlanePropNonlinearExt;5;9;3", "direction");
    expectInvalid("PlanePropNonlinearExt;5;9;;xyz", "thickness");
    expectInvalid("PlanePropNonlinearExt;5;9;;;XX", "analysisType");
    expectInvalid("PlanePropNonlinearExt;5;9;;;;ThreeNodeElems;PlaneStress",
                  "behavior3"); // no plane stress triangle with three grids
    expectInvalid("PlanePropNonlinearExt;5;9;;;;FourNodeElems;PlaneStrainComposite;Quadratic",
                  "integration4"); // the composite has L only
    expectInvalid("PlanePropNonlinearExt;5;9;;;;SixNodeElems;PlaneStrainComposite",
                  "behavior6"); // and only on the four-node quadrilateral
    expectInvalid("PlanePropNonlinearExt;5;9;;;;EightNodeElems;AxisymSolid;LinearTwist",
                  "integration8"); // the linear twist is for four nodes
    expectInvalid("PlanePropNonlinearExt;5;9;;;;FourNodeElems;PlaneStrain;Linear;PlaneStress",
                  "heatBehavior4"); // no plane stress heat
}
