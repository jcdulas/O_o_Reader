// Checks of the remaining load cards: ACLOAD (the acoustic load of an external program), ACIFPRM (the parameters of the
// acoustic conjugate infinite elements), ACCSSPT (the access points of an external superelement), LOADCLID, LOADCNAM
// and LOADCSUB (the load combinations of a partitioned analysis), IMPCASE (a collection of imperfections), PLOADB3 and
// TEMPB3 (the loads of a CBEAM3), TEMPN1 and SLOADN1 (the temperature and power on the grid points of heat shell
// elements), RADBND and RADMT (the wave bands of the radiation and the tables of a RADM). Per card: the fields in their
// format reference positions, the defaults, the references the card lists, the renumbering of the spaces it owns, the
// refused values and the round trip through a model file.

#include "oor/model/cards/dynamics/heatShellNodalLoad/HeatShellNodalLoadTable.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"
#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsCatalogue.hpp"
#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsTable.hpp"
#include "oor/model/cards/loads/acousticPressureLoadMatrices/AcousticPressureLoadMatricesTable.hpp"
#include "oor/model/cards/loads/beam3DistributedLoad/Beam3DistributedLoadTable.hpp"
#include "oor/model/cards/loads/beam3TemperatureLoad/Beam3TemperatureLoadTable.hpp"
#include "oor/model/cards/loads/heatShellInitialTemperature/HeatShellInitialTemperatureTable.hpp"
#include "oor/model/cards/loads/imperfectionCase/ImperfectionCaseTable.hpp"
#include "oor/model/cards/loads/loadCaseIdLabel/LoadCaseIdLabelTable.hpp"
#include "oor/model/cards/loads/loadCaseNameLabel/LoadCaseNameLabelTable.hpp"
#include "oor/model/cards/loads/loadCaseSubcaseLabel/LoadCaseSubcaseLabelTable.hpp"
#include "oor/model/cards/superelements/superelementAccessPoints/SuperelementAccessPointsTable.hpp"
#include "oor/model/cards/thermal/radiationBands/RadiationBandsTable.hpp"
#include "oor/model/cards/thermal/radiationPropTemperature/RadiationPropTemperatureTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
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

// Records what a card names: the target and the ID, in the order of the card.
struct IdVisitor final : ReferenceVisitor
{
    std::vector<std::pair<ReferenceTarget, base::EntityId>> seen;
    base::EntityId visit(std::string_view, ReferenceTarget target, base::EntityId id) override
    {
        seen.emplace_back(target, id);
        return id;
    }
};

using Seen = std::vector<std::pair<ReferenceTarget, base::EntityId>>;

// The first field of a line of the small format, blank: where a deck written in that format starts a short name.
const std::string BLANK_NAME_FIELD(8, ' ');

// A TEMPB3 with point temperatures, some blank, and a list of elements that ends on a second line with a BY.
const std::string BEAM3_TEMPERATURE_FULL =
    "Beam3TemperatureLoad;4601;4623;52.5;12.5;0.;2.25;18.5;4.75;;1.5;62.;84.;36.;;57.5;70.;24.;;41.5;;;38.5;;4009;4012;"
    "Through;4030;4041;4053;4065;4080;Through;4120;By;4";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// ACLOAD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ACLOAD has the format reference defaults and is a dynamic load that a DLOAD names",
          "[model][cards][acload]")
{
    Model model = readBulk(
        "AcousticPressureLoadMatrices;7201;61;62;1.75\nAcousticPressureLoadMatrices;102;7;"
        "8\nAcousticPressureLoadMatrices;103;61;62;2.5;-0.5;3");
    const AcousticPressureLoadMatricesCard sampleCard = cardAt<AcousticPressureLoadMatricesTable>(model, 0);
    REQUIRE((sampleCard.id == 7201 && sampleCard.dataFileUnit == 61 && sampleCard.propertyFileUnit == 62 &&
             sampleCard.scaleReal == 1.75));
    REQUIRE((sampleCard.scaleImaginary == 0.0 && sampleCard.fileLoadCaseId == 1));
    const AcousticPressureLoadMatricesCard bare = cardAt<AcousticPressureLoadMatricesTable>(model, 1);
    REQUIRE((bare.scaleReal == 1.0 && bare.scaleImaginary == 0.0 &&
             bare.fileLoadCaseId == 1)); // the three defaults of the blank fields
    const AcousticPressureLoadMatricesCard full = cardAt<AcousticPressureLoadMatricesTable>(model, 2);
    REQUIRE((full.scaleReal == 2.5 && full.scaleImaginary == -0.5 && full.fileLoadCaseId == 3));
    REQUIRE((AcousticPressureLoadMatricesTable::INFO.category == CardCategory::DynamicLoad &&
             AcousticPressureLoadMatricesTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // A default is a blank field, so the card of defaults is its three numbers.

    // the writer drops the zero before the point

    // The units are Fortran units, not cards: nothing is named.
    RecordingVisitor visitor;
    model.table<AcousticPressureLoadMatricesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    // The dynamic load numbers are one space: a DLOAD finds the ACLOAD, and a renumbering of them moves both.
    Model dynamic = readBulk("DynamicLoadCombination;10;1.;1.;7201\nAcousticPressureLoadMatrices;7201;61;62");
    REQUIRE(unresolvedIn(std::move(dynamic)) == 0);
    REQUIRE(unresolvedIn(readBulk("DynamicLoadCombination;10;1.;1.;7202\nAcousticPressureLoadMatrices;7201;61;62")) ==
            1);
    Model renumbered = readBulk("DynamicLoadCombination;10;1.;1.;7201\nAcousticPressureLoadMatrices;7201;61;62");
    Renumbering::apply(renumbered, RenumberSpace::DynamicLoadSet, Renumbering::offset(1000));
    REQUIRE(cardAt<AcousticPressureLoadMatricesTable>(renumbered).id == 8201);
    REQUIRE(unresolvedIn(std::move(renumbered)) == 0);

    expectInvalid("AcousticPressureLoadMatrices;0;41;42", "id");
    expectInvalid("AcousticPressureLoadMatrices;1;0;42", "dataFileUnit");
    expectInvalid("AcousticPressureLoadMatrices;1;41;0", "propertyFileUnit");
    expectInvalid("AcousticPressureLoadMatrices;1;41", "propertyFileUnit"); // required
    expectInvalid("AcousticPressureLoadMatrices;1;41;42;xx", "scaleReal");
    expectInvalid("AcousticPressureLoadMatrices;1;41;42;;xx", "scaleImaginary");
    expectInvalid("AcousticPressureLoadMatrices;1;41;42;;;0", "fileLoadCaseId");
}

// ---------------------------------------------------------------------------------------------------------------------
// ACIFPRM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ACIFPRM keeps its pairs of name and value and checks the ones the table lists", "[model][cards][acifprm]")
{
    Model model =
        readBulk("AcousticInfiniteParams;FieldPointInsideTolerance;0.005;FirstExtraPointId;320000000;FUTURE;2.5");
    const AcousticInfiniteParamsCard card = cardAt<AcousticInfiniteParamsTable>(model);
    REQUIRE(card.parameters.size() == 3);
    REQUIRE(card.parameters[0].name == "FieldPointInsideTolerance"); // the neutral name of the dictionary
    REQUIRE((!card.parameters[0].value.isInteger() && card.parameters[0].value.asReal() == 0.005));
    REQUIRE(card.parameters[1].name == "FirstExtraPointId");
    REQUIRE(
        (card.parameters[1].value.isInteger() && std::get<std::int32_t>(card.parameters[1].value.number) == 320000000));
    REQUIRE((card.parameters[2].name == "FUTURE" && card.parameters[2].value.asReal() == 2.5)); // not in the table
    REQUIRE((AcousticInfiniteParamsTable::INFO.category == CardCategory::Other &&
             AcousticInfiniteParamsTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The pairs go on from field 9, and a pair of blank fields is padding.
    const AcousticInfiniteParamsCard second = cardAt<AcousticInfiniteParamsTable>(readBulk(
        "AcousticInfiniteParams;FieldPointInsideTolerance;0.02;FirstExtraPointId;250000000;AAA;1;BBB;2.5;CCC;3"));
    REQUIRE(second.parameters.size() == 5);
    REQUIRE((second.parameters[4].name == "CCC" && second.parameters[4].value.isInteger()));
    REQUIRE(cardAt<AcousticInfiniteParamsTable>(
                readBulk("AcousticInfiniteParams;FieldPointInsideTolerance;0.01;;;FirstExtraPointId;3"))
                .parameters.size() == 2);

    // An integer suits the real parameter and is kept as an integer.
    const AcousticInfiniteParamsCard whole =
        cardAt<AcousticInfiniteParamsTable>(readBulk("AcousticInfiniteParams;FieldPointInsideTolerance;1"));
    REQUIRE(whole.parameters[0].value.isInteger());

    // The table of the format reference: the kind, the default and what each value must be.
    const AcousticInfiniteParamsInfo* tolerance = findAcousticInfiniteParam("FieldPointInsideTolerance");
    REQUIRE(tolerance != nullptr);
    REQUIRE((tolerance->kind == AcousticInfiniteParamsKind::Real && tolerance->defaultValue == 0.001));
    const AcousticInfiniteParamsInfo* firstPoint = findAcousticInfiniteParam("FirstExtraPointId");
    REQUIRE(firstPoint != nullptr);
    REQUIRE((firstPoint->kind == AcousticInfiniteParamsKind::Integer && firstPoint->defaultValue == 200000000.0));
    REQUIRE(findAcousticInfiniteParam("OTHER") == nullptr);
    REQUIRE(acousticInfiniteParamsValueProblem(*tolerance, AcousticInfiniteParamsValue{std::int32_t{1}}).empty());
    REQUIRE_FALSE(acousticInfiniteParamsValueProblem(*tolerance, AcousticInfiniteParamsValue{0.0}).empty());
    REQUIRE_FALSE(acousticInfiniteParamsValueProblem(*firstPoint, AcousticInfiniteParamsValue{2.5}).empty());

    // Only one ACIFPRM may be in a deck: the second is reported, the first is kept.
    const Model twice = readModelText(
        "[model]\nAcousticInfiniteParams;FieldPointInsideTolerance;0.01\nAcousticInfiniteParams;"
        "FieldPointInsideTolerance;0.02\n",
        false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-acoustic-infinite-params");

    expectInvalid("AcousticInfiniteParams;FieldPointInsideTolerance;0.", "parameterValue");
    expectInvalid("AcousticInfiniteParams;FieldPointInsideTolerance;-1.", "parameterValue");
    expectInvalid("AcousticInfiniteParams;FirstExtraPointId;2.5", "parameterValue"); // an integer is expected
    expectInvalid("AcousticInfiniteParams;FirstExtraPointId;0", "parameterValue");
    expectInvalid("AcousticInfiniteParams;FieldPointInsideTolerance", "parameterValue");
    expectInvalid("AcousticInfiniteParams;;5.", "parameterName");
    expectInvalid("AcousticInfiniteParams;1TOL;5.", "parameterName");
    oor::testing::requireModelFileRoundTrip(readModelText("AcousticInfiniteParams;TOO_LONG_NAME;1.", true));
    expectInvalid("AcousticInfiniteParams;FieldPointInsideTolerance;abc", "parameterValue");
}

// ---------------------------------------------------------------------------------------------------------------------
// ACCSSPT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ACCSSPT lists grid points and follows a renumbering of the nodes", "[model][cards][accsspt]")
{
    Model model = readBulk(
        "SuperelementAccessPoints;4117;4119\nSuperelementAccessPoints;1;2;3;4;5;6;7;8;9;10;"
        "11\nSuperelementAccessPoints;5;;7");
    REQUIRE((cardAt<SuperelementAccessPointsTable>(model, 0).nodeIds == std::vector<base::EntityId>{4117, 4119}));
    REQUIRE((cardAt<SuperelementAccessPointsTable>(model, 1).nodeIds ==
             std::vector<base::EntityId>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11})); // the continuation line
    REQUIRE((cardAt<SuperelementAccessPointsTable>(model, 2).nodeIds ==
             std::vector<base::EntityId>{5, 7})); // a blank field is padding
    REQUIRE((SuperelementAccessPointsTable::INFO.category == CardCategory::Other &&
             SuperelementAccessPointsTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // Each point is a grid point of the model of the card.
    RecordingVisitor visitor;
    model.table<SuperelementAccessPointsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeId", "nodeId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    REQUIRE(unresolvedIn(readBulk("SuperelementAccessPoints;4117;4119")) == 2);
    Model grids = readBulk("Node;4117\nNode;4119\nSuperelementAccessPoints;4117;4119");
    REQUIRE(unresolvedIn(std::move(grids)) == 0);

    Model renumbered = readBulk("Node;4117\nNode;4119\nSuperelementAccessPoints;4117;4119");
    Renumbering::apply(renumbered, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<SuperelementAccessPointsTable>(renumbered).nodeIds == std::vector<base::EntityId>{5117, 5119}));

    // The card has no ID, so a deck may repeat it (the format says the points are unique, a rule between cards).
    REQUIRE_NOTHROW(readBulk("SuperelementAccessPoints;1\nSuperelementAccessPoints;1"));

    expectInvalid("SuperelementAccessPoints", "nodeId1");
    expectInvalid("SuperelementAccessPoints;0", "nodeId1");
    expectInvalid("SuperelementAccessPoints;5;-3", "nodeId1");
    expectInvalid("SuperelementAccessPoints;5;xx", "nodeId");
    expectInvalid("SuperelementAccessPoints;5;THRU;9", "nodeId"); // the format has no THRU
}

// ---------------------------------------------------------------------------------------------------------------------
// LOADCLID, LOADCNAM and LOADCSUB
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a LOADCLID has an item to every two lines, with the name of the part over the whole second",
          "[model][cards][loadclid]")
{
    Model model = readBulk(
        "LoadCaseIdLabel;4202;1.25;0.75;4101;left_wing_panel;1.5;4301;right_wing_panel\n"
        "LoadCaseIdLabel;4203;-2.;0.5;7;Wing");
    const LoadCaseIdLabelCard sampleCard = cardAt<LoadCaseIdLabelTable>(model, 0);
    REQUIRE((sampleCard.id == 4202 && sampleCard.scale == 1.25 && sampleCard.items.size() == 2));
    REQUIRE((sampleCard.items[0] == LoadCaseIdLabelItem{0.75, 4101, "left_wing_panel"}));
    REQUIRE((sampleCard.items[1] == LoadCaseIdLabelItem{1.5, 4301, "right_wing_panel"}));
    const LoadCaseIdLabelCard negative = cardAt<LoadCaseIdLabelTable>(model, 1);
    REQUIRE((negative.scale == -2.0 && negative.items[0] == LoadCaseIdLabelItem{0.5, 7, "Wing"})); // the case is kept
    REQUIRE((LoadCaseIdLabelTable::INFO.category == CardCategory::Load &&
             LoadCaseIdLabelTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // Long names remain whole in a model record.
    const std::string longest(64, 'n');
    const std::string named = "LoadCaseIdLabel;204;1.5;2.;5;left_wing_panel;3.;6;" + longest;
    const LoadCaseIdLabelCard small = cardAt<LoadCaseIdLabelTable>(readBulk(named));
    REQUIRE((small.id == 204 && small.scale == 1.5 && small.items.size() == 2));
    REQUIRE((small.items[0] == LoadCaseIdLabelItem{2.0, 5, "left_wing_panel"}));
    REQUIRE((small.items[1] == LoadCaseIdLabelItem{3.0, 6, longest}));
    requireRoundTrip(readBulk(named));

    // A name of more than eight characters makes the card leave the small format.

    // A short name stays where the format reference draws it, at the start of the line of its own.

    // The LID is a load set that the Case Control LOAD names, and shares the number with the other loads.
    REQUIRE(unresolvedIn(readModelText("[case]\nStaticLoadSelection;4202\n[model]\nLoadCaseIdLabel;4202;1.;1.;5;wing\n",
                                       true)) == 0);
    REQUIRE(unresolvedIn(readModelText("[case]\nStaticLoadSelection;4203\n[model]\nLoadCaseIdLabel;4202;1.;1.;5;wing\n",
                                       true)) == 1);
    Model renumbered = readBulk("LoadCaseIdLabel;4202;1.;1.;5;wing");
    Renumbering::apply(renumbered, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE(cardAt<LoadCaseIdLabelTable>(renumbered).id == 5202);
    REQUIRE(cardAt<LoadCaseIdLabelTable>(renumbered).items[0].loadCombinationId ==
            5); // the load ID of the part is not a number of the model
    RecordingVisitor visitor;
    model.table<LoadCaseIdLabelTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("LoadCaseIdLabel;0;1.;1.;5;wing", "id");
    expectInvalid("LoadCaseIdLabel;1;;1.;5;wing", "scaleFactor");
    expectInvalid("LoadCaseIdLabel;1;0.;1.;5;wing", "scaleFactor");
    expectInvalid("LoadCaseIdLabel;1;1.", "scaleFactor1");
    expectInvalid("LoadCaseIdLabel;1;1.;0.;5;wing", "scaleFactors");
    expectInvalid("LoadCaseIdLabel;1;1.;1.;0;wing", "loadIds");
    expectInvalid("LoadCaseIdLabel;1;1.;1.;;wing", "loadIds");
    expectInvalid("LoadCaseIdLabel;1;1.;1.;5", "partNames");
    oor::testing::requireModelFileRoundTrip(
        readModelText("LoadCaseIdLabel;1;1.;1.;5;" + std::string(65, 'n'), true)); // 64 at most
    oor::testing::requireModelFileRoundTrip(readModelText("LoadCaseIdLabel;1;1.;1.;5;wi'ng", true));
}

TEST_CASE("a LOADCNAM has an item to every three lines: the factor, the name of the load and the name of the part",
          "[model][cards][loadcnam]")
{
    Model model = readBulk(
        "LoadCaseNameLabel;4301;1.2;1.75;lateral_gust;inboard_left_flap_track_fairing_assembly;0.6;lateral_gust_on_the_"
        "tail_cone;aft_fuselage_panel\n"
        "LoadCaseNameLabel;4302;3.;-1.5;G;P");
    const LoadCaseNameLabelCard sampleCard = cardAt<LoadCaseNameLabelTable>(model, 0);
    REQUIRE((sampleCard.id == 4301 && sampleCard.scale == 1.2 && sampleCard.items.size() == 2));
    REQUIRE((sampleCard.items[0] ==
             LoadCaseNameLabelItem{1.75, "lateral_gust", "inboard_left_flap_track_fairing_assembly"}));
    REQUIRE((sampleCard.items[1] == LoadCaseNameLabelItem{0.6, "lateral_gust_on_the_tail_cone", "aft_fuselage_panel"}));
    REQUIRE((cardAt<LoadCaseNameLabelTable>(model, 1).items == std::vector<LoadCaseNameLabelItem>{{-1.5, "G", "P"}}));
    REQUIRE((LoadCaseNameLabelTable::INFO.category == CardCategory::Load &&
             LoadCaseNameLabelTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    REQUIRE(unresolvedIn(readModelText("[case]\nStaticLoadSelection;4301\n[model]\nLoadCaseNameLabel;4301;1.;1.;a;b\n",
                                       true)) == 0);
    RecordingVisitor visitor;
    model.table<LoadCaseNameLabelTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("LoadCaseNameLabel;0;1.;1.;grav;wing", "id");
    expectInvalid("LoadCaseNameLabel;1;0.;1.;grav;wing", "scaleFactor");
    expectInvalid("LoadCaseNameLabel;1;1.", "scaleFactor1");
    expectInvalid("LoadCaseNameLabel;1;1.;0.;grav;wing", "scaleFactors");
    expectInvalid("LoadCaseNameLabel;1;1.;1.;;wing", "loadNames");
    expectInvalid("LoadCaseNameLabel;1;1.;1.;grav", "partNames");
    oor::testing::requireModelFileRoundTrip(readModelText("LoadCaseNameLabel;1;1.;1.;gra'v;wing", true));
}

TEST_CASE("a LOADCSUB names the subcases of the parts", "[model][cards][loadcsub]")
{
    Model model = readBulk(
        "LoadCaseSubcaseLabel;4401;1.;0.8;2;nose_gear_bay_from_subcase_2;1.4;5;main_gear_bay_from_subcase_5\n"
        "LoadCaseSubcaseLabel;4402;-1.;0.5;2;a;1.5;3;b;2.5;4;c");
    const LoadCaseSubcaseLabelCard sampleCard = cardAt<LoadCaseSubcaseLabelTable>(model, 0);
    REQUIRE((sampleCard.id == 4401 && sampleCard.scale == 1.0));
    REQUIRE((sampleCard.items == std::vector<LoadCaseSubcaseLabelItem>{{0.8, 2, "nose_gear_bay_from_subcase_2"},
                                                                       {1.4, 5, "main_gear_bay_from_subcase_5"}}));
    REQUIRE((cardAt<LoadCaseSubcaseLabelTable>(model, 1).items ==
             std::vector<LoadCaseSubcaseLabelItem>{{0.5, 2, "a"}, {1.5, 3, "b"}, {2.5, 4, "c"}}));
    REQUIRE((LoadCaseSubcaseLabelTable::INFO.category == CardCategory::Load &&
             LoadCaseSubcaseLabelTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // The LOADCLID, LOADCNAM and LOADCSUB are loads of one number space: a LOAD command finds any of them.
    REQUIRE(unresolvedIn(readModelText(
                "[case]\nStaticLoadSelection;4401\n[model]\nLoadCaseSubcaseLabel;4401;1.;1.;1;a\n", true)) == 0);

    expectInvalid("LoadCaseSubcaseLabel;0;1.;1.;1;wing", "id");
    expectInvalid("LoadCaseSubcaseLabel;1;0.;1.;1;wing", "scaleFactor");
    expectInvalid("LoadCaseSubcaseLabel;1;1.", "scaleFactor1");
    expectInvalid("LoadCaseSubcaseLabel;1;1.;0.;1;wing", "scaleFactors");
    expectInvalid("LoadCaseSubcaseLabel;1;1.;1.;0;wing", "subcases");
    expectInvalid("LoadCaseSubcaseLabel;1;1.;1.;1.5;wing", "subcases");
    expectInvalid("LoadCaseSubcaseLabel;1;1.;1.;1", "partNames");
}

TEST_CASE("the names of a part and the components of a heat shell point follow the rules of their helpers",
          "[model][cards][loadclosure]")
{
    REQUIRE_FALSE(checkPaaName("wing", "NAME").has_value());
    REQUIRE_FALSE(checkPaaName(std::string(64, 'x'), "NAME").has_value());
    REQUIRE_FALSE(checkPaaName(std::string(65, 'x'), "partName").has_value());
    REQUIRE(checkPaaName("", "NAME").has_value());
    REQUIRE_FALSE(checkPaaName("two words", "partName").has_value());
    REQUIRE_FALSE(checkPaaName("a,b", "partName").has_value());
    REQUIRE_FALSE(checkPaaName("a$b", "partName").has_value());

    REQUIRE((isShellComponentSet(1) && isShellComponentSet(23) && isShellComponentSet(321)));
    REQUIRE_FALSE((isShellComponentSet(0) || isShellComponentSet(4) || isShellComponentSet(11) ||
                   isShellComponentSet(1234) || isShellComponentSet(-1)));
    REQUIRE((sortedShellComponents(321) == 123 && sortedShellComponents(31) == 13));
    REQUIRE(sortedShellComponents(11) == 11); // not a set: left for the check to refuse
    REQUIRE(checkShellComponents(11, "C1").has_value());
}

// ---------------------------------------------------------------------------------------------------------------------
// IMPCASE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an IMPCASE lists IMPGEOM IDs with THRU, kept as written", "[model][cards][impcase]")
{
    Model model = readBulk(
        "ImperfectionCase;5;1;2\nImperfectionCase;6;10;Through;20;30\nImperfectionCase;7;1;2;3;4;5;6;7;8;9;Through;15");
    REQUIRE((cardAt<ImperfectionCaseTable>(model, 0).imperfections == std::vector<IdRun>{{1, 1}, {2, 2}}));
    REQUIRE((cardAt<ImperfectionCaseTable>(model, 1).imperfections == std::vector<IdRun>{{10, 20}, {30, 30}}));
    REQUIRE((cardAt<ImperfectionCaseTable>(model, 2).imperfections ==
             std::vector<IdRun>{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 15}}));
    REQUIRE((ImperfectionCaseTable::INFO.category == CardCategory::ImperfectionCase &&
             ImperfectionCaseTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The IDs are IMPGEOM entries, and the two ends of a range are looked at alike: this deck has no IMPGEOM, so they
    // are unresolved (the IMPGEOM tests of FreeBodyLoadsCardsTest.cpp have one).
    IdVisitor visitor;
    model.table<ImperfectionCaseTable>().visitReferences(1, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::ImperfectionGeometry, 10},
                                 {ReferenceTarget::ImperfectionGeometry, 20},
                                 {ReferenceTarget::ImperfectionGeometry, 30}});
    REQUIRE(unresolvedIn(readBulk("ImperfectionCase;5;1;2")) == 2);

    // The ID is the IMPCASE's own number: its renumbering moves it and leaves the list.
    REQUIRE(Renumbering::apply(model, RenumberSpace::ImperfectionCase, Renumbering::offset(100)) == 3);
    REQUIRE((cardAt<ImperfectionCaseTable>(model, 0).id == 105 && cardAt<ImperfectionCaseTable>(model, 2).id == 107));
    REQUIRE((cardAt<ImperfectionCaseTable>(model, 1).imperfections == std::vector<IdRun>{{10, 20}, {30, 30}}));
    // Two cards of the same number are refused: it is one space for the type.
    REQUIRE_THROWS_AS(readBulk("ImperfectionCase;5;1\nImperfectionCase;5;2"), base::DeckError);

    expectInvalid("ImperfectionCase;0;1", "id");
    expectInvalid("ImperfectionCase;5", "imperfectionIds");
    expectInvalid("ImperfectionCase;5;0", "imperfectionIds");
    expectInvalid("ImperfectionCase;5;xx", "imperfectionIds");
    expectInvalid("ImperfectionCase;5;3;Through;1", "imperfectionIds");
    expectInvalid("ImperfectionCase;5;Through;9", "Through");
    expectInvalid("ImperfectionCase;5;1;Through", "Through");
}

// ---------------------------------------------------------------------------------------------------------------------
// PLOADB3
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PLOADB3 takes its system as a word or a number and requires its type", "[model][cards][ploadb3]")
{
    Model model = readBulk(
        "Beam3DistributedLoad;4510;4502;LocalFrame;1.;;;Moment;;125.;95.;60.\n"
        "Beam3DistributedLoad;11;5;;0.;1.;0.;Force\n"
        "Beam3DistributedLoad;12;6;12;0.;0.;-1.;WarpingBimoment;2.;1.;2.;3.\n"
        "Beam3DistributedLoad;13;7;Element;1.;1.;1.;Force\n"
        "Beam3DistributedLoad;14;8;0;1.;;;Force\n"
        "Beam3DistributedLoad;15;9;BasicFrame;1.;;;Force");
    const Beam3DistributedLoadCard sampleCard = cardAt<Beam3DistributedLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 4510 && sampleCard.elemId == 4502 &&
             sampleCard.frame == Beam3DistributedLoadFrame::Local && sampleCard.frameId == 0));
    REQUIRE((sampleCard.n == std::array<double, 3>{1.0, 0.0, 0.0} &&
             sampleCard.kind == Beam3DistributedLoadType::Moment && sampleCard.scaleFactor == 1.0));
    REQUIRE(sampleCard.p == std::array<double, 3>{125.0, 95.0, 60.0});
    const Beam3DistributedLoadCard blank = cardAt<Beam3DistributedLoadTable>(model, 1);
    REQUIRE((blank.frame == Beam3DistributedLoadFrame::Basic && blank.scaleFactor == 1.0 &&
             blank.p == std::array<double, 3>{0.0, 0.0, 0.0}));
    const Beam3DistributedLoadCard system = cardAt<Beam3DistributedLoadTable>(model, 2);
    REQUIRE((system.frame == Beam3DistributedLoadFrame::Coordinate && system.frameId == 12 &&
             system.kind == Beam3DistributedLoadType::Bimoment));
    REQUIRE((system.n == std::array<double, 3>{0.0, 0.0, -1.0} && system.scaleFactor == 2.0));
    REQUIRE(system.p == std::array<double, 3>{1.0, 2.0, 3.0});
    const Beam3DistributedLoadCard element = cardAt<Beam3DistributedLoadTable>(model, 3);
    REQUIRE((element.frame == Beam3DistributedLoadFrame::Element &&
             element.kind == Beam3DistributedLoadType::Force)); // any case
    REQUIRE(cardAt<Beam3DistributedLoadTable>(model, 4).frame ==
            Beam3DistributedLoadFrame::Basic); // 0 is the basic system
    REQUIRE(cardAt<Beam3DistributedLoadTable>(model, 5).frame == Beam3DistributedLoadFrame::Basic); // so is the word
    REQUIRE((Beam3DistributedLoadTable::INFO.category == CardCategory::Load &&
             Beam3DistributedLoadTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // BASIC is the default and is a blank field; written in full when the defaults are not omitted.

    // The element is a reference, and the system when it is a number.
    IdVisitor visitor;
    model.table<Beam3DistributedLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Element, 4502}});
    visitor = IdVisitor{};
    model.table<Beam3DistributedLoadTable>().visitReferences(2, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Element, 6}, {ReferenceTarget::Coordinate, 12}});
    REQUIRE(unresolvedIn(readBulk("Beam3DistributedLoad;12;6;12;0.;0.;-1.;WarpingBimoment")) == 2);
    REQUIRE(unresolvedIn(readBulk("Beam3DistributedLoad;10;6;LocalFrame;1.;0.;0.;Force")) == 1);

    // They follow a renumbering of the elements and of the coordinate systems.
    Model resolved = readBulk(
        "Node;1\n"
        "Node;2;;1.\n"
        "Rod2;6;1;1;2\n"
        "RectFrame2;12;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "Beam3DistributedLoad;12;6;12;0.;0.;-1.;WarpingBimoment");
    Renumbering::apply(resolved, RenumberSpace::Element, Renumbering::offset(1000));
    Renumbering::apply(resolved, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE((cardAt<Beam3DistributedLoadTable>(resolved).elemId == 1006 &&
             cardAt<Beam3DistributedLoadTable>(resolved).frameId == 112));

    expectInvalid("Beam3DistributedLoad;0;5;;1.;0.;0.;Force", "id");
    expectInvalid("Beam3DistributedLoad;1;0;;1.;0.;0.;Force", "elemId");
    expectInvalid("Beam3DistributedLoad;1;5;FRAME;1.;0.;0.;Force", "frameId");
    expectInvalid("Beam3DistributedLoad;1;5;-2;1.;0.;0.;Force", "frameId");
    expectInvalid("Beam3DistributedLoad;1;5;;0.;0.;0.;Force", "directionX"); // at least one component is not zero
    expectInvalid("Beam3DistributedLoad;1;5;;xx;0.;0.;Force", "directionX");
    expectInvalid("Beam3DistributedLoad;1;5;;1.;0.;0.", "kind"); // required
    expectInvalid("Beam3DistributedLoad;1;5;;1.;0.;0.;TORQUE", "kind");
    expectInvalid("Beam3DistributedLoad;1;5;;1.;0.;0.;Force;xx", "scaleFactor");
    expectInvalid("Beam3DistributedLoad;1;5;;1.;0.;0.;Force;;xx", "pressureA");
    oor::testing::requireModelFileRoundTrip(model, false);
}

// ---------------------------------------------------------------------------------------------------------------------
// TEMPB3
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TEMPB3 keeps twelve temperatures by station and a list of elements with THRU and BY",
          "[model][cards][tempb3]")
{
    Model model = readBulk(
        BEAM3_TEMPERATURE_FULL +
        "\nBeam3TemperatureLoad;102;5;10.;;;;;;;;;;;;;;;;;;;;;7\nBeam3TemperatureLoad;103;7;;;;;;;;;;5.;;;;;;;;;;;;8");
    const Beam3TemperatureLoadCard sampleCard = cardAt<Beam3TemperatureLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 4601 && sampleCard.elemId == 4623));
    REQUIRE((sampleCard.temperature == std::array<double, 3>{52.5, 12.5, 0.0}));
    REQUIRE((sampleCard.gradientY == std::array<double, 3>{2.25, 4.75, 1.5} &&
             sampleCard.gradientZ == std::array<double, 3>{18.5, 0.0, 62.0}));
    // The point temperatures: blank ones are 0.0 once some are given.
    REQUIRE((sampleCard.temperatureC == std::array<double, 3>{84.0, 70.0, 0.0} &&
             sampleCard.temperatureD == std::array<double, 3>{36.0, 24.0, 0.0}));
    REQUIRE((sampleCard.temperatureE == std::array<double, 3>{0.0, 0.0, 38.5} &&
             sampleCard.temperatureF == std::array<double, 3>{57.5, 41.5, 0.0}));
    REQUIRE(sampleCard.hasPointTemperatures());
    // The list: 4009, 4012 THRU 4030, 4041, 4053, 4065 and 4080 THRU 4120 BY 4 (the THRU is the first field of a
    // line).
    REQUIRE(
        (sampleCard.elements ==
         std::vector<ThermalRun>{
             {4009, 4009, 1}, {4012, 4030, 1}, {4041, 4041, 1}, {4053, 4053, 1}, {4065, 4065, 1}, {4080, 4120, 4}}));

    // With no point temperature the card has none: the format reference says linear gradients are assumed.
    const Beam3TemperatureLoadCard bare = cardAt<Beam3TemperatureLoadTable>(model, 1);
    REQUIRE((bare.id == 102 && bare.elemId == 5 && bare.temperature == std::array<double, 3>{10.0, 0.0, 0.0}));
    REQUIRE_FALSE(bare.hasPointTemperatures());
    REQUIRE((std::isnan(bare.temperatureC[0]) && std::isnan(bare.temperatureF[2])));
    REQUIRE((bare.elements == std::vector<ThermalRun>{{7, 7, 1}}));
    // One point temperature given: the eleven others are 0.0.
    const Beam3TemperatureLoadCard one = cardAt<Beam3TemperatureLoadTable>(model, 2);
    REQUIRE(one.hasPointTemperatures());
    REQUIRE((one.temperatureC == std::array<double, 3>{5.0, 0.0, 0.0} &&
             one.temperatureD == std::array<double, 3>{0.0, 0.0, 0.0}));
    REQUIRE((Beam3TemperatureLoadTable::INFO.category == CardCategory::Load &&
             Beam3TemperatureLoadTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // The two forms are written differently: the card with no point temperature has no TC to TF at all.

    REQUIRE(cardAt<Beam3TemperatureLoadTable>(
                readModelText(
                    writeModelText(readModelText("Beam3TemperatureLoad;102;5;10.;;;;;;;;;;;;;;;;;;;;;7", true)), true))
                .hasPointTemperatures() == false);

    // The element of the EID and every element of the list are elements; a THRU range names its two ends, which may not
    // exist.
    IdVisitor visitor;
    model.table<Beam3TemperatureLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Element, 4623},
                                 {ReferenceTarget::Element, 4009},
                                 {ReferenceTarget::ElementInRange, 4012},
                                 {ReferenceTarget::ElementInRange, 4030},
                                 {ReferenceTarget::Element, 4041},
                                 {ReferenceTarget::Element, 4053},
                                 {ReferenceTarget::Element, 4065},
                                 {ReferenceTarget::ElementInRange, 4080},
                                 {ReferenceTarget::ElementInRange, 4120}});
    REQUIRE(unresolvedIn(readBulk(BEAM3_TEMPERATURE_FULL)) == 5); // the EID and the four single elements
    Model rods = readBulk(
        "Beam3TemperatureLoad;102;5;10.;;;;;;;;;;;;;;;;;;;;;7;Through;9\nRod2;5;1;1;2\nRod2;7;1;1;2\nRod2;9;1;"
        "1;"
        "2\nNode;1\nNode;2");
    Renumbering::apply(rods, RenumberSpace::Element, Renumbering::offset(1000));
    const Beam3TemperatureLoadCard moved = cardAt<Beam3TemperatureLoadTable>(rods);
    REQUIRE((moved.elemId == 1005 && moved.elements == std::vector<ThermalRun>{{1007, 1009, 1}}));
    REQUIRE(moved.id == 102); // the temperature set is not an element number

    expectInvalid("Beam3TemperatureLoad;0;5;;;;;;;;;;;;;;;;;;;;;;7", "id");
    expectInvalid("Beam3TemperatureLoad;1;0;;;;;;;;;;;;;;;;;;;;;;7", "elemId");
    expectInvalid("Beam3TemperatureLoad;1;5;xx;;;;;;;;;;;;;;;;;;;;;7", "temperatureA");
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;xx;;;;;;;;7", "temperatureCB");
    expectInvalid("Beam3TemperatureLoad;1;5", "elemId"); // at least one element in the list
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;;;;;;;;;0", "elemId");
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;;;;;;;;;9;Through;3", "elemId");
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;;;;;;;;;9;Through", "Through");
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;;;;;;;;;9;By;2", "By");
    expectInvalid("Beam3TemperatureLoad;1;5;;;;;;;;;;;;;;;;;;;;;;9;Through;12;By;0", "By");
}

// ---------------------------------------------------------------------------------------------------------------------
// TEMPN1 and SLOADN1
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TEMPN1 gives the temperature of one or two points, each on the top, the bottom and the middle",
          "[model][cards][tempn1]")
{
    Model model = readBulk(
        "HeatShellInitialTemperature;10;4100;123;875.\nHeatShellInitialTemperature;11;5;;20."
        "5\nHeatShellInitialTemperature;12;1;1;1.;2;23;-2.\nHeatShellInitialTemperature;13;7;321;5.");
    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(model, 0).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{4100, 123, 875.0}}));
    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(model, 1).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{5, 1, 20.5}})); // a blank C is 1 (the top)
    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(model, 2).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{1, 1, 1.0}, {2, 23, -2.0}}));
    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(model, 3).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{7, 123, 5.0}})); // the digits are sorted
    REQUIRE((HeatShellInitialTemperatureTable::INFO.category == CardCategory::Load &&
             HeatShellInitialTemperatureTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    // A default C is a blank field, and a Gi of the second point can follow a blank first point.

    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(readBulk("HeatShellInitialTemperature;14;;;;6;3;4.")).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{6, 3, 4.0}}));

    // The points are grid points, in the node space.
    IdVisitor visitor;
    model.table<HeatShellInitialTemperatureTable>().visitReferences(2, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Node, 1}, {ReferenceTarget::Node, 2}});
    REQUIRE(unresolvedIn(readBulk("HeatShellInitialTemperature;12;1;1;1.;2;23;-2.")) == 2);
    Model grids = readBulk("Node;1\nNode;2\nHeatShellInitialTemperature;12;1;1;1.;2;23;-2.");
    REQUIRE(unresolvedIn(std::move(grids)) == 0);
    Model renumbered = readBulk("Node;1\nNode;2\nHeatShellInitialTemperature;12;1;1;1.;2;23;-2.");
    Renumbering::apply(renumbered, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<HeatShellInitialTemperatureTable>(renumbered).points ==
             std::vector<HeatShellInitialTemperaturePoint>{{101, 1, 1.0}, {102, 23, -2.0}}));

    expectInvalid("HeatShellInitialTemperature;0;10;1;5.", "id");
    expectInvalid("HeatShellInitialTemperature;1", "node1");
    expectInvalid("HeatShellInitialTemperature;1;0;1;5.", "node1");
    expectInvalid("HeatShellInitialTemperature;1;;1;5.", "node1");
    expectInvalid("HeatShellInitialTemperature;1;10;4;5.", "components1");
    expectInvalid("HeatShellInitialTemperature;1;10;0;5.", "components1");
    expectInvalid("HeatShellInitialTemperature;1;10;11;5.", "components1"); // a digit twice
    expectInvalid("HeatShellInitialTemperature;1;10;1", "temperature1");
    expectInvalid("HeatShellInitialTemperature;1;10;1;xx", "temperature1");
    expectInvalid("HeatShellInitialTemperature;1;10;1;5.;20;9;6.", "components2");
}

TEST_CASE("an SLOADN1 gives the power on one or two points of a heat shell element", "[model][cards][sloadn1]")
{
    Model model = readBulk(
        "HeatShellNodalLoad;10;4110;12;640.;4120;2;415.\nHeatShellNodalLoad;11;5;;2.5\nHeatShellNodalLoad;12;7;321;"
        "50.");
    REQUIRE((cardAt<HeatShellNodalLoadTable>(model, 0).loads ==
             std::vector<HeatShellNodalLoadLoad>{{4110, 12, 640.0}, {4120, 2, 415.0}}));
    REQUIRE((cardAt<HeatShellNodalLoadTable>(model, 1).loads ==
             std::vector<HeatShellNodalLoadLoad>{{5, 1, 2.5}})); // a blank C is 1
    REQUIRE((cardAt<HeatShellNodalLoadTable>(model, 2).loads == std::vector<HeatShellNodalLoadLoad>{{7, 123, 50.0}}));
    REQUIRE((HeatShellNodalLoadTable::INFO.category == CardCategory::Load &&
             HeatShellNodalLoadTable::INFO.idSpace == base::IdSpace::Group));
    requireRoundTrip(model);

    IdVisitor visitor;
    model.table<HeatShellNodalLoadTable>().visitReferences(0, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::Node, 4110}, {ReferenceTarget::Node, 4120}});
    Model renumbered = readBulk("Node;4110\nNode;4120\nHeatShellNodalLoad;10;4110;12;640.;4120;2;415.");
    Renumbering::apply(renumbered, RenumberSpace::Node, Renumbering::offset(5));
    REQUIRE((cardAt<HeatShellNodalLoadTable>(renumbered).loads ==
             std::vector<HeatShellNodalLoadLoad>{{4115, 12, 640.0}, {4125, 2, 415.0}}));

    // The load is a load of the set a LOAD command or an EXCITEID names, like the SLOAD.
    REQUIRE(unresolvedIn(readModelText(
                "[case]\nStaticLoadSelection;10\n[model]\nNode;5\nHeatShellNodalLoad;10;5;;2.5\n", true)) == 0);

    expectInvalid("HeatShellNodalLoad;0;10;1;5.", "id");
    expectInvalid("HeatShellNodalLoad;1", "node1");
    expectInvalid("HeatShellNodalLoad;1;0;1;5.", "node1");
    expectInvalid("HeatShellNodalLoad;1;;1;5.", "node1");
    expectInvalid("HeatShellNodalLoad;1;10;4;5.", "components1");
    expectInvalid("HeatShellNodalLoad;1;10;0;5.", "components1");
    expectInvalid("HeatShellNodalLoad;1;10;11;5.", "components1");
    expectInvalid("HeatShellNodalLoad;1;10;1", "heatFlux1");
    expectInvalid("HeatShellNodalLoad;1;10;1;xx", "heatFlux1");
    expectInvalid("HeatShellNodalLoad;1;10;1;5.;20;9;6.", "components2");
}

// ---------------------------------------------------------------------------------------------------------------------
// RADBND and RADMT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a RADBND has one wavelength fewer than its bands, in ascending order", "[model][cards][radbnd]")
{
    Model model = readBulk("RadiationBands;6;14388.;1.5;3.;5.5;9.;15.");
    const RadiationBandsCard sampleCard = cardAt<RadiationBandsTable>(model);
    REQUIRE((sampleCard.bandCount == 6 && sampleCard.planckSecondConstant == 14388.0));
    REQUIRE((sampleCard.wavelengths == std::vector<double>{1.5, 3.0, 5.5, 9.0, 15.0}));
    REQUIRE((RadiationBandsTable::INFO.category == CardCategory::Other &&
             RadiationBandsTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The wavelengths go on from field 9, and a repeated one is a band of zero width.
    const RadiationBandsCard long8 =
        cardAt<RadiationBandsTable>(readBulk("RadiationBands;9;25898.;1.;2.;3.;4.;5.;6.;7.;8."));
    REQUIRE((long8.bandCount == 9 && long8.wavelengths.size() == 8 && long8.wavelengths.back() == 8.0));
    REQUIRE((cardAt<RadiationBandsTable>(readBulk("RadiationBands;4;14388.;2.;2.;5.")).wavelengths ==
             std::vector<double>{2.0, 2.0, 5.0}));
    REQUIRE((cardAt<RadiationBandsTable>(readBulk("RadiationBands;2;14388.;0.")).wavelengths ==
             std::vector<double>{0.0})); // not below zero

    // Only one RADBND may be in a deck: the second is reported, the first is kept.
    const Model twice = readModelText("[model]\nRadiationBands;2;14388.;5.\nRadiationBands;3;14388.;5.;6.\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-radiation-bands");

    RecordingVisitor visitor;
    model.table<RadiationBandsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("RadiationBands", "bandCount");
    expectInvalid("RadiationBands;2", "planckSecondConstant");
    expectInvalid("RadiationBands;1;14388.;5.", "bandCount");
    expectInvalid("RadiationBands;2;0.;5.", "planckSecondConstant");
    expectInvalid("RadiationBands;2;-5.;5.", "planckSecondConstant");
    expectInvalid("RadiationBands;3;14388.;5.", "bandWavelength");    // one wavelength is missing
    expectInvalid("RadiationBands;2;14388.;5.;6.", "bandWavelength"); // one too many
    expectInvalid("RadiationBands;3;14388.;5.;3.", "bandWavelength"); // not ascending
    expectInvalid("RadiationBands;2;14388.;-5.", "bandWavelength");
    expectInvalid("RadiationBands;3;14388.;5.;;6.", "bandWavelength"); // a blank before the last
    expectInvalid("RadiationBands;2;14388.;xx", "bandWavelength");
}

TEST_CASE("a RADMT extends the RADM of its number with one table for each property and each wave band",
          "[model][cards][radmt]")
{
    Model model = readBulk(
        "RadiationPropTemperature;11;;1;2;3;4;5;6\n"
        "RadiationPropTemperature;12\n"
        "RadiationPropTemperature;13;7\n"
        "RadiationPropTemperature;14;7;;8;;9\n"
        "RadiationPropTemperature;15;;1;2;3;4;5;6;7;8;9\n"
        "RadiationPropTemperature;16;;;;3;0;0");
    REQUIRE((cardAt<RadiationPropTemperatureTable>(model, 0).absorptivityTableId == 0 &&
             cardAt<RadiationPropTemperatureTable>(model, 0).emissivityTableIds ==
                 std::vector<base::EntityId>{1, 2, 3, 4, 5, 6}));
    REQUIRE((cardAt<RadiationPropTemperatureTable>(model, 1).absorptivityTableId == 0 &&
             cardAt<RadiationPropTemperatureTable>(model, 1).emissivityTableIds.empty()));
    REQUIRE((cardAt<RadiationPropTemperatureTable>(model, 2).absorptivityTableId == 7 &&
             cardAt<RadiationPropTemperatureTable>(model, 2).emissivityTableIds.empty()));
    // A band without a table keeps its place: the table of the third band is the third value.
    REQUIRE((cardAt<RadiationPropTemperatureTable>(model, 3).emissivityTableIds ==
             std::vector<base::EntityId>{0, 8, 0, 9}));
    REQUIRE((cardAt<RadiationPropTemperatureTable>(model, 4).emissivityTableIds ==
             std::vector<base::EntityId>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
    // The zeros at the end are not kept.
    REQUIRE(
        (cardAt<RadiationPropTemperatureTable>(model, 5).emissivityTableIds == std::vector<base::EntityId>{0, 0, 3}));
    REQUIRE((RadiationPropTemperatureTable::INFO.category == CardCategory::Other &&
             RadiationPropTemperatureTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The ID is the one of a RADM and the tables are TABLEMj cards.
    IdVisitor visitor;
    model.table<RadiationPropTemperatureTable>().visitReferences(3, visitor);
    REQUIRE(visitor.seen == Seen{{ReferenceTarget::RadiationMaterial, 14},
                                 {ReferenceTarget::MaterialTable, 7},
                                 {ReferenceTarget::MaterialTable, 8},
                                 {ReferenceTarget::MaterialTable, 9}});
    const std::string tables =
        "TableMaterial1;1;;;0.;1.;100.;0.5;EndTable\nTableMaterial1;2;;;0.;1.;100.;0.5;EndTable\n";
    REQUIRE(unresolvedIn(readBulk("RadiationProp;11;0.9\n" + tables + "RadiationPropTemperature;11;1;2")) == 0);
    REQUIRE(unresolvedIn(readBulk("RadiationProp;11;0.9\n" + tables + "RadiationPropTemperature;12;1;3")) ==
            2); // the RADM 12 and the table 3

    // A renumbering of the RADM moves the number of the RADMT, and one of the tables moves its tables.
    Model resolved = readBulk("RadiationProp;11;0.9\n" + tables + "RadiationPropTemperature;11;1;2");
    REQUIRE(Renumbering::apply(resolved, RenumberSpace::RadiationMaterial, Renumbering::offset(100)) == 1);
    REQUIRE(cardAt<RadiationPropTemperatureTable>(resolved).id == 111);
    Renumbering::apply(resolved, RenumberSpace::MaterialTable, Renumbering::offset(10));
    REQUIRE((cardAt<RadiationPropTemperatureTable>(resolved).absorptivityTableId == 11 &&
             cardAt<RadiationPropTemperatureTable>(resolved).emissivityTableIds == std::vector<base::EntityId>{12}));
    REQUIRE(unresolvedIn(std::move(resolved)) == 0);
    // The RADMT is the companion: a RADM and a RADMT of one number coexist, two RADMT of one number do not.
    REQUIRE_NOTHROW(readBulk("RadiationProp;11;0.9\nRadiationPropTemperature;11;1"));
    REQUIRE_THROWS_AS(readBulk("RadiationPropTemperature;11;1\nRadiationPropTemperature;11;2"), base::DeckError);

    expectInvalid("RadiationPropTemperature", "id");
    expectInvalid("RadiationPropTemperature;0", "id");
    expectInvalid("RadiationPropTemperature;1;xx", "absorptivityTableId");
    expectInvalid("RadiationPropTemperature;1;-3", "absorptivityTableId");
    expectInvalid("RadiationPropTemperature;1;;xx", "emissivityTableId");
    expectInvalid("RadiationPropTemperature;1;;-3", "emissivityTableId");
}
