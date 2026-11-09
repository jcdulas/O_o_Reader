// Checks of the co-simulation cards of SOL 400 (COSMGRP, COSMINP, COSMOUT, COSMSEL, COSMSRV) and of the fluid-structure
// cards (FSICTRL, WETELME, WETELMG, WETSURF, WETLOAD). Per card: the fields in their format reference positions, the
// defaults and the variants, the references it lists and what each one points to, the ID space and the renumbering of
// the kinds it owns, the cards that edit and delete it, the refused values and the round trip through a model file.
// The generic tests of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/loads/wettedSurfaceLoad/WettedSurfaceLoadTable.hpp"
#include "oor/model/cards/solution_control/cosimulationArea/CosimulationAreaTable.hpp"
#include "oor/model/cards/solution_control/cosimulationInputs/CosimulationInputsTable.hpp"
#include "oor/model/cards/solution_control/cosimulationOutputs/CosimulationOutputsTable.hpp"
#include "oor/model/cards/solution_control/cosimulationSelection/CosimulationSelectionTable.hpp"
#include "oor/model/cards/solution_control/cosimulationService/CosimulationServiceTable.hpp"
#include "oor/model/cards/solution_control/fluidStructureControl/FluidStructureControlTable.hpp"
#include "oor/model/cards/solution_control/wettedElemNodes/WettedElemNodesTable.hpp"
#include "oor/model/cards/solution_control/wettedElemSide/WettedElemSideTable.hpp"
#include "oor/model/cards/solution_control/wettedSurface/WettedSurfaceTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

using Faces = std::vector<CosimulationFace>;
using Runs = std::vector<ContactRun>;
using Ids = std::vector<base::EntityId>;

// A small mesh for the references: six grid points and two quadrilaterals (the elements 1001 and 1002).
const std::string MESH_TEXT =
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;3;;1.;1.;0.\n"
    "Node;4;;0.;1.;0.\n"
    "Node;5;;2.;0.;0.\n"
    "Node;6;;2.;1.;0.\n"
    "IsoMat;1;2.1E+11;;0.3\n"
    "ShellProp;1;1;0.1\n"
    "ShellQuad4;1001;1;1;2;3;4\n"
    "ShellQuad4;1002;1;2;5;6;3\n";

// The cards of one co-simulation service: a group of two element faces, its input and output quantities, the service
// that joins them and the selection of that service.
const std::string COSIMULATION_TEXT =
    "CosimulationArea;7100;Surface;1001;3;1002;6\n"
    "CosimulationInputs;7200;Force\n"
    "CosimulationOutputs;7300;Displacement;Acceleration\n"
    "CosimulationService;7400;flowSvc;Follower;7100;7200;7100;7300\n"
    "CosimulationSelection;5;7400\n";

// The references a card of a table lists.
template <typename TableT>
RecordingVisitor referencesOf(Model& model, std::uint32_t row = 0)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor;
}

// The kind and the ID of every reference a card lists, in the order of its fields.
using Seen = std::vector<std::pair<ReferenceTarget, base::EntityId>>;

struct IdVisitor final : ReferenceVisitor
{
    Seen seen;
    base::EntityId visit(std::string_view, ReferenceTarget target, base::EntityId id) override
    {
        seen.emplace_back(target, id);
        return id;
    }
};

template <typename TableT>
Seen seenBy(Model& model, std::uint32_t row = 0)
{
    IdVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor.seen;
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// COSMGRP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a COSMGRP lists the faces of elements, grid points or elements by its TYPE, and is a group a COSMSRV names",
          "[model][cards][cosimulation]")
{
    REQUIRE((CosimulationAreaTable::INFO.category == CardCategory::CosimulationGroup &&
             CosimulationAreaTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(MESH_TEXT +
                           "CosimulationArea;7102;;1001;3;1002;6\n"
                           "CosimulationArea;7103;Surface;1001;1;;;;;;;1002;2\n"
                           "CosimulationArea;7104;Point;1;2;5;6\n"
                           "CosimulationArea;7105;Point;1;Through;4;6\n"
                           "CosimulationArea;7106;Volume;1001;1002\n"
                           "CosimulationArea;7107;Volume;1001;Through;1002\n");

    const CosimulationAreaCard blank = cardAt<CosimulationAreaTable>(model, 0);
    REQUIRE((blank.id == 7102 && blank.kind == CosimulationAreaType::Surface)); // a blank TYPE is SURFACE
    REQUIRE(blank.faces == Faces{{1001, 3}, {1002, 6}});
    REQUIRE(blank.runs.empty());
    // The second pair of the card is on the second continuation line: the blank pairs between the lines are skipped.
    REQUIRE(cardAt<CosimulationAreaTable>(model, 1).faces == Faces{{1001, 1}, {1002, 2}});
    const CosimulationAreaCard points = cardAt<CosimulationAreaTable>(model, 2);
    REQUIRE((points.kind == CosimulationAreaType::Point && points.faces.empty()));
    REQUIRE(points.runs == Runs{{1, 1, 1}, {2, 2, 1}, {5, 5, 1}, {6, 6, 1}});
    REQUIRE(cardAt<CosimulationAreaTable>(model, 3).runs == Runs{{1, 4, 1}, {6, 6, 1}}); // a THRU run, kept as written
    const CosimulationAreaCard volume = cardAt<CosimulationAreaTable>(model, 4);
    REQUIRE((volume.kind == CosimulationAreaType::Volume && volume.runs == Runs{{1001, 1001, 1}, {1002, 1002, 1}}));
    REQUIRE(cardAt<CosimulationAreaTable>(model, 5).runs == Runs{{1001, 1002, 1}});

    // What each form names: the elements of the faces, the grid points of a POINT area (the ends of a THRU range are
    // looked at as the ends of a range) and the elements of a VOLUME.
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor faces = referencesOf<CosimulationAreaTable>(model, 0);
    REQUIRE(faces.fields == std::vector<std::string>{"elemIds", "elemIds"});
    REQUIRE(faces.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Element));
    const RecordingVisitor grids = referencesOf<CosimulationAreaTable>(model, 3);
    REQUIRE(grids.fields == std::vector<std::string>(3, "nodeIds"));
    REQUIRE(grids.targets == std::vector<ReferenceTarget>{ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange,
                                                          ReferenceTarget::Node});
    REQUIRE(referencesOf<CosimulationAreaTable>(model, 4).targets ==
            std::vector<ReferenceTarget>(2, ReferenceTarget::Element));
    REQUIRE(referencesOf<CosimulationAreaTable>(model, 5).targets ==
            std::vector<ReferenceTarget>(2, ReferenceTarget::ElementInRange));
    // The kind and the ID of each: the face number is not a reference, and a single ID is told from the end of a range.
    REQUIRE(seenBy<CosimulationAreaTable>(model, 0) ==
            Seen{{ReferenceTarget::Element, 1001}, {ReferenceTarget::Element, 1002}});
    REQUIRE(seenBy<CosimulationAreaTable>(model, 3) ==
            Seen{{ReferenceTarget::NodeInRange, 1}, {ReferenceTarget::NodeInRange, 4}, {ReferenceTarget::Node, 6}});
    REQUIRE(seenBy<CosimulationAreaTable>(model, 5) ==
            Seen{{ReferenceTarget::ElementInRange, 1001}, {ReferenceTarget::ElementInRange, 1002}});
    Model lacking = readBulk("CosimulationArea;7102;;1001;3;1002;6\nCosimulationArea;7104;Point;1;2");
    REQUIRE(unresolvedIn(lacking) == 4); // two elements and two grid points
    requireRoundTrip(model);

    // The edit path of Python: the values are checked, then stored in the row.
    CosimulationAreaCard edited = cardAt<CosimulationAreaTable>(model, 0);
    edited.faces.push_back(CosimulationFace{1001, 2});
    REQUIRE_FALSE(CosimulationAreaFormat::check(edited).has_value());
    model.table<CosimulationAreaTable>().store(0, edited);
    REQUIRE(cardAt<CosimulationAreaTable>(model, 0).faces.size() == 3);
    edited.kind = CosimulationAreaType::Point; // a POINT area that still lists faces, then one that lists nothing
    REQUIRE(CosimulationAreaFormat::check(edited).has_value());
    edited.faces.clear();
    REQUIRE(CosimulationAreaFormat::check(edited).has_value());
    edited.runs = {ContactRun{1, 2, 1}};
    REQUIRE_FALSE(CosimulationAreaFormat::check(edited).has_value());
    edited.runs = {ContactRun{4, 2, -1}}; // a list goes upwards
    REQUIRE(CosimulationAreaFormat::check(edited).has_value());

    // The renumbering of the grid points and of the elements follows into the lists, ranges included.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<CosimulationAreaTable>(model, 2).runs ==
            Runs{{101, 101, 1}, {102, 102, 1}, {105, 105, 1}, {106, 106, 1}});
    REQUIRE(cardAt<CosimulationAreaTable>(model, 3).runs == Runs{{101, 104, 1}, {106, 106, 1}});
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<CosimulationAreaTable>(model, 0).faces == Faces{{2001, 3}, {2002, 6}, {2001, 2}}); // the face stays
    REQUIRE(cardAt<CosimulationAreaTable>(model, 4).runs == Runs{{2001, 2001, 1}, {2002, 2002, 1}});
    REQUIRE(cardAt<CosimulationAreaTable>(model, 5).runs == Runs{{2001, 2002, 1}});
    REQUIRE(unresolvedIn(model) == 0);
    // The group number is the card's own: its space renumbers it.
    Renumbering::apply(model, RenumberSpace::CosimulationGroup, Renumbering::offset(5000));
    REQUIRE(
        (cardAt<CosimulationAreaTable>(model, 0).id == 12102 && cardAt<CosimulationAreaTable>(model, 5).id == 12107));
    requireRoundTrip(model);

    // A card that is deleted leaves the others in their place.
    model.remove(*model.locateInType(typeIndexOf<CosimulationAreaTable>(), 12102));
    REQUIRE_FALSE(model.locateInType(typeIndexOf<CosimulationAreaTable>(), 12102).has_value());
    model.compact();
    REQUIRE(rowCount<CosimulationAreaTable>(model) == 5);
    REQUIRE(cardAt<CosimulationAreaTable>(model, 0).id == 12103);
    requireRoundTrip(model);

    // One number per group.
    REQUIRE_THROWS_AS(readBulk("CosimulationArea;7102;;1001;5\nCosimulationArea;7102;Point;1"), base::DeckError);

    expectInvalid("CosimulationArea;0;;1001;5", "id");
    expectInvalid("CosimulationArea;7102;CURVE;1001;5", "kind");
    expectInvalid("CosimulationArea;7102", "elemIds"); // SURFACE with no face
    expectInvalid("CosimulationArea;7102;Point", "nodeIds");
    expectInvalid("CosimulationArea;7102;Volume", "elemIds");
    expectInvalid("CosimulationArea;7102;;1001", "faceIds"); // an element and no face
    expectInvalid("CosimulationArea;7102;;;5", "elemIds");   // a face and no element
    expectInvalid("CosimulationArea;7102;;0;1", "elemIds");
    expectInvalid("CosimulationArea;7102;;-5;1", "elemIds");
    expectInvalid("CosimulationArea;7102;;1001;0", "faceIds");
    expectInvalid("CosimulationArea;7102;;1001;7", "faceIds"); // the sides are numbered 1 to 6
    expectInvalid("CosimulationArea;7102;;1001;x", "faceIds");
    expectInvalid("CosimulationArea;7102;;1001;5;THRU;9", "elemIds"); // the pairs have no THRU
    expectInvalid("CosimulationArea;7102;Point;0", "GridID");
    expectInvalid("CosimulationArea;7102;Point;-4", "GridID");
    expectInvalid("CosimulationArea;7102;Point;1;Through", "Through");
    expectInvalid("CosimulationArea;7102;Point;Through;5", "Through");
    expectInvalid("CosimulationArea;7102;Point;10;Through;5", "GridID");     // upwards only
    expectInvalid("CosimulationArea;7102;Point;1;Through;9;By;2", "GridID"); // the lists have no BY
    expectInvalid("CosimulationArea;7102;Volume;x", "elemIds");
}

// ---------------------------------------------------------------------------------------------------------------------
// COSMINP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a COSMINP has one or two quantities of input, FORCE when PHYQUA1 is blank", "[model][cards][cosimulation]")
{
    REQUIRE((CosimulationInputsTable::INFO.category == CardCategory::CosimulationInput &&
             CosimulationInputsTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "CosimulationInputs;415\n"
        "CosimulationInputs;416;Force\n"
        "CosimulationInputs;417;Temperature\n"
        "CosimulationInputs;418;Force;Temperature\n"
        "CosimulationInputs;419;;Temperature\n"
        "CosimulationInputs;420;Temperature;Force");
    using Quantities = std::vector<CosimulationInputQuantity>;
    constexpr CosimulationInputQuantity INPUT_FORCE = CosimulationInputQuantity::Force;
    constexpr CosimulationInputQuantity INPUT_TEMP = CosimulationInputQuantity::Temp;
    REQUIRE(cardAt<CosimulationInputsTable>(model, 0).quantities ==
            Quantities{INPUT_FORCE}); // no continuation line at all
    REQUIRE(cardAt<CosimulationInputsTable>(model, 1).quantities == Quantities{INPUT_FORCE});
    REQUIRE(cardAt<CosimulationInputsTable>(model, 2).quantities == Quantities{INPUT_TEMP}); // any case
    REQUIRE(cardAt<CosimulationInputsTable>(model, 3).quantities == Quantities{INPUT_FORCE, INPUT_TEMP});
    REQUIRE(cardAt<CosimulationInputsTable>(model, 4).quantities ==
            Quantities{INPUT_FORCE, INPUT_TEMP}); // a blank PHYQUA1 is FORCE
    REQUIRE(cardAt<CosimulationInputsTable>(model, 5).quantities ==
            Quantities{INPUT_TEMP, INPUT_FORCE}); // the order is kept
    REQUIRE(cardAt<CosimulationInputsTable>(model, 0).id == 415);
    REQUIRE(referencesOf<CosimulationInputsTable>(model).fields.empty()); // the card names no other card
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // The default FORCE is a blank field, written back only when the deck asks for every field.
    Model plain = readBulk("CosimulationInputs;416;Force");

    const Model again = readModelText(writeModelText(plain), true);
    REQUIRE(cardAt<CosimulationInputsTable>(again).quantities == Quantities{INPUT_FORCE});

    // The edit path: a quantity is not given twice, and there are two at most.
    CosimulationInputsCard edited = cardAt<CosimulationInputsTable>(model, 3);
    edited.quantities = {INPUT_TEMP};
    REQUIRE_FALSE(CosimulationInputsFormat::check(edited).has_value());
    model.table<CosimulationInputsTable>().store(3, edited);
    REQUIRE(cardAt<CosimulationInputsTable>(model, 3).quantities == Quantities{INPUT_TEMP});
    edited.quantities = {INPUT_FORCE, INPUT_FORCE};
    REQUIRE(CosimulationInputsFormat::check(edited).has_value());
    edited.quantities = {INPUT_FORCE, INPUT_TEMP, INPUT_FORCE};
    REQUIRE(CosimulationInputsFormat::check(edited).has_value());
    edited.quantities.clear();
    REQUIRE(CosimulationInputsFormat::check(edited).has_value());

    // The number is the card's own; the COSMSRV that names it follows its renumbering.
    Model chain = readBulk(COSIMULATION_TEXT);
    Renumbering::apply(chain, RenumberSpace::CosimulationInput, Renumbering::offset(100));
    REQUIRE(cardAt<CosimulationInputsTable>(chain).id == 7300);
    REQUIRE(cardAt<CosimulationServiceTable>(chain).inputId == 7300);
    model.remove(*model.locateInType(typeIndexOf<CosimulationInputsTable>(), 417));
    REQUIRE_FALSE(model.locateInType(typeIndexOf<CosimulationInputsTable>(), 417).has_value());

    REQUIRE_THROWS_AS(readBulk("CosimulationInputs;415\nCosimulationInputs;415;Temperature"), base::DeckError);
    expectInvalid("CosimulationInputs;0", "id");
    expectInvalid("CosimulationInputs;-4;Force", "id");
    expectInvalid("CosimulationInputs;415;PRESSURE", "quantity1");
    expectInvalid("CosimulationInputs;415;Force;PRESSURE", "quantity2");
    expectInvalid("CosimulationInputs;415;Force;Force", "quantity2"); // a quantity once
    expectInvalid("CosimulationInputs;415;;Force", "quantity2");      // a blank PHYQUA1 is FORCE
    expectInvalid("CosimulationInputs;415;5", "quantity1");
    oor::testing::requireModelFileRoundTrip(plain);
    oor::testing::requireModelFileRoundTrip(plain, false);
}

// ---------------------------------------------------------------------------------------------------------------------
// COSMOUT
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// COSMSEL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a COSMSEL lists up to three services, which it names, under a number a Case Control command selects",
          "[model][cards][cosimulation]")
{
    REQUIRE((CosimulationSelectionTable::INFO.category == CardCategory::Other &&
             CosimulationSelectionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model =
        readBulk("CosimulationSelection;1;21\nCosimulationSelection;2;21;22;23\nCosimulationSelection;3;;22\n");
    REQUIRE((cardAt<CosimulationSelectionTable>(model, 0).id == 1 &&
             cardAt<CosimulationSelectionTable>(model, 0).services == Ids{21}));
    REQUIRE(cardAt<CosimulationSelectionTable>(model, 1).services == Ids{21, 22, 23});
    REQUIRE(cardAt<CosimulationSelectionTable>(model, 2).services == Ids{22}); // a blank field is skipped

    // Each service is a reference to a COSMSRV; the SID is selected by a Case Control command and names nothing.
    const RecordingVisitor visitor = referencesOf<CosimulationSelectionTable>(model, 1);
    REQUIRE(visitor.fields == std::vector<std::string>(3, "serviceIds"));
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::CosimulationService));
    REQUIRE(seenBy<CosimulationSelectionTable>(model, 1) == Seen{{ReferenceTarget::CosimulationService, 21},
                                                                 {ReferenceTarget::CosimulationService, 22},
                                                                 {ReferenceTarget::CosimulationService, 23}});
    REQUIRE(unresolvedIn(model) == 5); // 21, 21, 22, 23 and 22: the model has no COSMSRV
    requireRoundTrip(model);

    Model chain = readBulk(COSIMULATION_TEXT);
    REQUIRE(unresolvedIn(chain) == 2); // the elements 1001 and 1002 of the COSMGRP are not in the deck
    Renumbering::apply(chain, RenumberSpace::CosimulationService, Renumbering::offset(1000));
    REQUIRE(cardAt<CosimulationServiceTable>(chain).id == 8400);
    REQUIRE((cardAt<CosimulationSelectionTable>(chain).services == Ids{8400} &&
             cardAt<CosimulationSelectionTable>(chain).id == 5));
    REQUIRE(unresolvedIn(chain) == 2); // the elements 1001 and 1002 of the COSMGRP are not in the deck
    // The selection is the card's own number and no space renumbers it (the Case Control command is text).
    requireRoundTrip(chain);

    CosimulationSelectionCard edited = cardAt<CosimulationSelectionTable>(model, 0);
    edited.services = {21, 22, 23};
    REQUIRE_FALSE(CosimulationSelectionFormat::check(edited).has_value());
    model.table<CosimulationSelectionTable>().store(0, edited);
    REQUIRE(cardAt<CosimulationSelectionTable>(model, 0).services.size() == 3);
    edited.services.push_back(24);
    REQUIRE(CosimulationSelectionFormat::check(edited).has_value());
    edited.services.clear();
    REQUIRE(CosimulationSelectionFormat::check(edited).has_value());

    model.remove(*model.locateInType(typeIndexOf<CosimulationSelectionTable>(), 2));
    model.compact();
    REQUIRE(rowCount<CosimulationSelectionTable>(model) == 2);

    REQUIRE_THROWS_AS(readBulk("CosimulationSelection;1;21\nCosimulationSelection;1;22"), base::DeckError);
    expectInvalid("CosimulationSelection;0;21", "id");
    expectInvalid("CosimulationSelection;1", "serviceId1"); // at least one service
    expectInvalid("CosimulationSelection;1;0", "serviceIds");
    expectInvalid("CosimulationSelection;1;-3", "serviceIds");
    expectInvalid("CosimulationSelection;1;x", "serviceIds");
}

// ---------------------------------------------------------------------------------------------------------------------
// COSMSRV
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a COSMSRV names two groups, the quantities of input and of output, and keeps its service name as written",
          "[model][cards][cosimulation]")
{
    REQUIRE((CosimulationServiceTable::INFO.category == CardCategory::CosimulationService &&
             CosimulationServiceTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        MESH_TEXT + COSIMULATION_TEXT +
        "CosimulationService;22;cplSVC;;213;316;214;417\nCosimulationService;23;svc;NoFollower;213;316;213;417\n");
    const CosimulationServiceCard example = cardAt<CosimulationServiceTable>(model, 0);
    REQUIRE((example.id == 7400 && example.serviceName == "flowSvc" &&
             example.followerForces == CosimulationForceMode::Follow));
    REQUIRE((example.inputAreaId == 7100 && example.inputId == 7200 && example.outputAreaId == 7100 &&
             example.outputId == 7300));
    const CosimulationServiceCard blank = cardAt<CosimulationServiceTable>(model, 1);
    REQUIRE(
        (blank.serviceName == "cplSVC" && blank.followerForces == CosimulationForceMode::NoFollow)); // a blank FOLFORCE
    REQUIRE((blank.inputAreaId == 213 && blank.outputAreaId == 214)); // the groups may differ
    REQUIRE(cardAt<CosimulationServiceTable>(model, 2).followerForces == CosimulationForceMode::NoFollow);
    // The service name is a name, not a reference.
    RecordingVisitor visitor = referencesOf<CosimulationServiceTable>(model, 0);
    REQUIRE(visitor.fields == std::vector<std::string>{"inputAreaId", "inputId", "outputAreaId", "outputId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::CosimulationGroup, ReferenceTarget::CosimulationInput,
                                         ReferenceTarget::CosimulationGroup, ReferenceTarget::CosimulationOutput});
    REQUIRE(seenBy<CosimulationServiceTable>(model, 1) == Seen{{ReferenceTarget::CosimulationGroup, 213},
                                                               {ReferenceTarget::CosimulationInput, 316},
                                                               {ReferenceTarget::CosimulationGroup, 214},
                                                               {ReferenceTarget::CosimulationOutput, 417}});
    // The first service is complete; the two others name a group, a COSMINP and a COSMOUT that the model lacks.
    REQUIRE(unresolvedIn(model) == 8);
    Model complete = readBulk(MESH_TEXT + COSIMULATION_TEXT);
    REQUIRE(unresolvedIn(complete) == 0);
    requireRoundTrip(model);
    // The default is a blank field.
    Model plain = readBulk("CosimulationService;22;cplSVC;NoFollower;213;316;214;417");

    // Each ID is renumbered with its own kind, and the card's own number with its space.
    Renumbering::apply(complete, RenumberSpace::CosimulationGroup, Renumbering::offset(1000));
    REQUIRE((cardAt<CosimulationServiceTable>(complete).inputAreaId == 8100 &&
             cardAt<CosimulationServiceTable>(complete).outputAreaId == 8100));
    REQUIRE((cardAt<CosimulationServiceTable>(complete).inputId == 7200 &&
             cardAt<CosimulationServiceTable>(complete).outputId == 7300));
    Renumbering::apply(complete, RenumberSpace::CosimulationInput, Renumbering::offset(1000));
    Renumbering::apply(complete, RenumberSpace::CosimulationOutput, Renumbering::offset(2000));
    REQUIRE((cardAt<CosimulationServiceTable>(complete).inputId == 8200 &&
             cardAt<CosimulationServiceTable>(complete).outputId == 9300));
    REQUIRE(cardAt<CosimulationServiceTable>(complete).id == 7400);
    REQUIRE(unresolvedIn(complete) == 0);
    Renumbering::apply(complete, RenumberSpace::CosimulationService, Renumbering::offset(3000));
    REQUIRE(cardAt<CosimulationServiceTable>(complete).id == 10400);
    REQUIRE(unresolvedIn(complete) == 0);
    requireRoundTrip(complete);

    // Deleting the group leaves two references to it unresolved.
    complete.remove(*complete.locateInType(typeIndexOf<CosimulationAreaTable>(), 8100));
    REQUIRE(unresolvedIn(complete) == 2);

    // The edit path: the values are checked, then stored.
    CosimulationServiceCard edited = cardAt<CosimulationServiceTable>(model, 0);
    edited.serviceName = "cplSVC";
    edited.followerForces = CosimulationForceMode::NoFollow;
    REQUIRE_FALSE(CosimulationServiceFormat::check(edited).has_value());
    model.table<CosimulationServiceTable>().store(0, edited);
    REQUIRE((cardAt<CosimulationServiceTable>(model, 0).serviceName == "cplSVC" &&
             cardAt<CosimulationServiceTable>(model, 0).outputId == 7300));
    edited.serviceName = "has blank";
    REQUIRE_FALSE(CosimulationServiceFormat::check(edited).has_value());
    edited.serviceName.clear();
    REQUIRE(CosimulationServiceFormat::check(edited).has_value());
    edited.serviceName = std::string(165, 'x');
    REQUIRE_FALSE(CosimulationServiceFormat::check(edited).has_value());

    REQUIRE_THROWS_AS(readBulk("CosimulationService;21;a;;1;2;1;3\nCosimulationService;21;b;;1;2;1;3"),
                      base::DeckError);
    expectInvalid("CosimulationService;0;sc;;1;2;1;3", "id");
    expectInvalid("CosimulationService;21;;;1;2;1;3", "serviceName"); // SERV is required
    expectInvalid("CosimulationService;21;sc;SPIN;1;2;1;3", "followerForces");
    expectInvalid("CosimulationService;21;sc;;0;2;1;3", "inputAreaId");
    expectInvalid("CosimulationService;21;sc;;;2;1;3", "inputAreaId");
    expectInvalid("CosimulationService;21;sc;;1;0;1;3", "inputId");
    expectInvalid("CosimulationService;21;sc;;1;2;-1;3", "outputAreaId");
    expectInvalid("CosimulationService;21;sc;;1;2;1", "outputId");
    expectInvalid("CosimulationService;21;sc;;1;2", "outputAreaId");
    oor::testing::requireModelFileRoundTrip(plain);
}

// ---------------------------------------------------------------------------------------------------------------------
// FSICTRL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FSICTRL has the format reference defaults, an unrestricted service name and no ID", "[model][cards][fsi]")
{
    REQUIRE((FluidStructureControlTable::INFO.category == CardCategory::Other &&
             FluidStructureControlTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "FluidStructureControl;cplsrv;ExplicitSolver;1\n"
        "FluidStructureControl;FluidSrv\n"
        "FluidStructureControl;srvdv;ImplicitSolver;5;DesignVariables\n"
        "FluidStructureControl;srvu;;;Displacement\n"
        "FluidStructureControl;CPLSRV;;2");
    const FluidStructureControlCard example = cardAt<FluidStructureControlTable>(model, 0);
    REQUIRE((example.serviceName == "cplsrv" && example.type == FsiCouplingType::Explicit && example.frequency == 1));
    REQUIRE(example.analysis == FsiServiceInterface::Dv);
    const FluidStructureControlCard bare = cardAt<FluidStructureControlTable>(model, 1);
    REQUIRE(bare.serviceName == "FluidSrv"); // the name is kept as written
    REQUIRE(
        (bare.type == FsiCouplingType::Explicit && bare.frequency == 1 && bare.analysis == FsiServiceInterface::Dv));
    const FluidStructureControlCard implicit = cardAt<FluidStructureControlTable>(model, 2);
    REQUIRE((implicit.type == FsiCouplingType::Implicit && implicit.frequency == 5));
    REQUIRE(implicit.analysis == FsiServiceInterface::Dv);
    REQUIRE(cardAt<FluidStructureControlTable>(model, 3).analysis == FsiServiceInterface::U);
    REQUIRE(cardAt<FluidStructureControlTable>(model, 4).frequency == 2);
    // The card has no number: any number of cards, even of one service, and nothing is named.
    REQUIRE(rowCount<FluidStructureControlTable>(model) == 5);
    REQUIRE(referencesOf<FluidStructureControlTable>(model).fields.empty());
    REQUIRE(model.table<FluidStructureControlTable>().serviceNameAt(1) == "FluidSrv");
    requireRoundTrip(model);

    // Every default is a blank field.

    FluidStructureControlCard edited = cardAt<FluidStructureControlTable>(model, 2);
    edited.frequency = 10;
    edited.type = FsiCouplingType::Explicit;
    REQUIRE_FALSE(FluidStructureControlFormat::check(edited).has_value());
    model.table<FluidStructureControlTable>().store(2, edited);
    REQUIRE((cardAt<FluidStructureControlTable>(model, 2).frequency == 10 &&
             cardAt<FluidStructureControlTable>(model, 2).serviceName == "srvdv"));
    edited.frequency = 0;
    REQUIRE(FluidStructureControlFormat::check(edited).has_value());
    edited.frequency = 1;
    edited.serviceName = "ninechars";
    REQUIRE_FALSE(FluidStructureControlFormat::check(edited).has_value());
    model.table<FluidStructureControlTable>().store(2, edited);
    requireRoundTrip(model);
    edited.serviceName.clear();
    REQUIRE(FluidStructureControlFormat::check(edited).has_value());
    model.remove(
        CardLocation{typeIndexOf<FluidStructureControlTable>(), 0}); // a card with no ID is removed by its place
    model.compact();
    REQUIRE(rowCount<FluidStructureControlTable>(model) == 4);
    REQUIRE(cardAt<FluidStructureControlTable>(model, 0).serviceName == "FluidSrv");

    expectInvalid("FluidStructureControl;;ExplicitSolver", "serviceName");
    oor::testing::requireModelFileRoundTrip(readModelText("FluidStructureControl;ninechars", true));
    expectInvalid("FluidStructureControl;cplsrv;FAST", "couplingKind");
    expectInvalid("FluidStructureControl;cplsrv;;0", "frequency");
    expectInvalid("FluidStructureControl;cplsrv;;-2", "frequency");
    expectInvalid("FluidStructureControl;cplsrv;;1.5", "frequency");
    expectInvalid("FluidStructureControl;cplsrv;;;X", "interfaceKind");
}

// ---------------------------------------------------------------------------------------------------------------------
// WETELME
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a WETELME is a side of a structural element, numbered with the wetted elements", "[model][cards][fsi]")
{
    REQUIRE((WettedElemSideTable::INFO.category == CardCategory::WettedElement &&
             WettedElemSideTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model =
        readBulk(MESH_TEXT + "WettedElemSide;10001;1001;3\nWettedElemSide;10002;1002;1\nWettedElemSide;10003;1001;6\n");
    const WettedElemSideCard example = cardAt<WettedElemSideTable>(model, 0);
    REQUIRE((example.id == 10001 && example.elemId == 1001 && example.sideCode == 3));
    REQUIRE(
        (cardAt<WettedElemSideTable>(model, 1).sideCode == 1 && cardAt<WettedElemSideTable>(model, 2).sideCode == 6));
    const RecordingVisitor visitor = referencesOf<WettedElemSideTable>(model, 0);
    REQUIRE(visitor.fields == std::vector<std::string>{"elemId"}); // the WEID is the card's own number
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Element});
    REQUIRE(seenBy<WettedElemSideTable>(model, 0) == Seen{{ReferenceTarget::Element, 1001}});
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("WettedElemSide;10001;1001;3");
    REQUIRE(unresolvedIn(lacking) == 1);
    requireRoundTrip(model);

    // The number is unique in the type; a WETELMG may have the same number (a check of the model reports it).
    REQUIRE_THROWS_AS(readBulk("WettedElemSide;1;5;1\nWettedElemSide;1;6;2"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("WettedElemSide;1;5;1\nWettedElemNodes;1;Line2Nodes;1;2"));

    // The element follows its renumbering and the number follows the one of the wetted elements.
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<WettedElemSideTable>(model, 0).elemId == 2001 && cardAt<WettedElemSideTable>(model, 1).elemId == 2002));
    REQUIRE(cardAt<WettedElemSideTable>(model, 0).id == 10001); // an element renumbering leaves the wetted element
    Renumbering::apply(model, RenumberSpace::WettedElement, Renumbering::offset(100));
    REQUIRE((cardAt<WettedElemSideTable>(model, 0).id == 10101 && cardAt<WettedElemSideTable>(model, 2).id == 10103));
    REQUIRE(cardAt<WettedElemSideTable>(model, 0).elemId == 2001);
    REQUIRE(unresolvedIn(model) == 0);

    WettedElemSideCard edited = cardAt<WettedElemSideTable>(model, 0);
    edited.sideCode = 4;
    REQUIRE_FALSE(WettedElemSideFormat::check(edited).has_value());
    model.table<WettedElemSideTable>().store(0, edited);
    REQUIRE(cardAt<WettedElemSideTable>(model, 0).sideCode == 4);
    edited.sideCode = 7;
    REQUIRE(WettedElemSideFormat::check(edited).has_value());
    model.remove(*model.locateInType(typeIndexOf<WettedElemSideTable>(), 10102));
    model.compact();
    REQUIRE(rowCount<WettedElemSideTable>(model) == 2);

    expectInvalid("WettedElemSide;0;5;1", "id");
    expectInvalid("WettedElemSide;1;0;1", "elemId");
    expectInvalid("WettedElemSide;1;-5;1", "elemId");
    expectInvalid("WettedElemSide;1;5", "sideCode"); // no default
    expectInvalid("WettedElemSide;1", "elemId");
    expectInvalid("WettedElemSide;1;5;0", "sideCode");
    expectInvalid("WettedElemSide;1;5;7", "sideCode"); // 1 to 6
    expectInvalid("WettedElemSide;1;5;x", "sideCode");
}

// ---------------------------------------------------------------------------------------------------------------------
// WETELMG
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a WETELMG has the grid points of its shape, which it refers to, with the numbers of the wetted elements",
          "[model][cards][fsi]")
{
    REQUIRE((WettedElemNodesTable::INFO.category == CardCategory::WettedElement &&
             WettedElemNodesTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(MESH_TEXT +
                           "WettedElemNodes;10001;Quad4Shape;1;2;3;4\n"
                           "WettedElemNodes;10002;Tria3Shape;1;2;3\n"
                           "WettedElemNodes;10003;Tria6Shape;1;2;3;4;5;6\n"
                           "WettedElemNodes;10004;Quad8Shape;1;2;3;4;5;6;1;2\n"
                           "WettedElemNodes;10005;Line2Nodes;1;2\n"
                           "WettedElemNodes;10006;Line3Nodes;1;2;3\n");
    const WettedElemNodesCard example = cardAt<WettedElemNodesTable>(model, 0);
    REQUIRE((example.id == 10001 && example.type == WettedElementType::Quad4 && example.grids == Ids{1, 2, 3, 4}));
    REQUIRE((cardAt<WettedElemNodesTable>(model, 1).type == WettedElementType::Tria3)); // any case
    REQUIRE(cardAt<WettedElemNodesTable>(model, 2).grids == Ids{1, 2, 3, 4, 5, 6});
    REQUIRE((cardAt<WettedElemNodesTable>(model, 3).type == WettedElementType::Quad8 &&
             cardAt<WettedElemNodesTable>(model, 3).grids == Ids{1, 2, 3, 4, 5, 6, 1, 2}));
    REQUIRE((cardAt<WettedElemNodesTable>(model, 4).type == WettedElementType::Line2 &&
             cardAt<WettedElemNodesTable>(model, 4).grids == Ids{1, 2}));
    REQUIRE((cardAt<WettedElemNodesTable>(model, 5).type == WettedElementType::Line3 &&
             cardAt<WettedElemNodesTable>(model, 5).grids == Ids{1, 2, 3}));
    REQUIRE(gridCountOf(WettedElementType::Tria6) == 6);
    REQUIRE(gridCountOf(WettedElementType::Quad8) == 8);

    // One reference per grid point, in the order of the card.
    const RecordingVisitor visitor = referencesOf<WettedElemNodesTable>(model, 0);
    REQUIRE(visitor.fields == std::vector<std::string>{"g1", "g2", "g3", "g4"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::Node));
    REQUIRE(seenBy<WettedElemNodesTable>(model, 0) == Seen{{ReferenceTarget::Node, 1},
                                                           {ReferenceTarget::Node, 2},
                                                           {ReferenceTarget::Node, 3},
                                                           {ReferenceTarget::Node, 4}});
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("WettedElemNodes;10001;Quad4Shape;1;2;3;4");
    REQUIRE(unresolvedIn(lacking) == 4);
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<WettedElemNodesTable>(model, 0).grids == Ids{101, 102, 103, 104});
    REQUIRE(cardAt<WettedElemNodesTable>(model, 0).id == 10001);
    Renumbering::apply(model, RenumberSpace::WettedElement, Renumbering::offset(5));
    REQUIRE((cardAt<WettedElemNodesTable>(model, 0).id == 10006 && cardAt<WettedElemNodesTable>(model, 5).id == 10011));
    REQUIRE(unresolvedIn(model) == 0);

    // The edit path: the shape and the number of grid points go together.
    WettedElemNodesCard edited = cardAt<WettedElemNodesTable>(model, 0);
    edited.type = WettedElementType::Tria3;
    REQUIRE(WettedElemNodesFormat::check(edited).has_value()); // four grid points are too many for a triangle
    edited.grids.pop_back();
    REQUIRE_FALSE(WettedElemNodesFormat::check(edited).has_value());
    model.table<WettedElemNodesTable>().store(0, edited);
    REQUIRE((cardAt<WettedElemNodesTable>(model, 0).type == WettedElementType::Tria3 &&
             cardAt<WettedElemNodesTable>(model, 0).grids.size() == 3));
    edited.grids[1] = 0;
    REQUIRE(WettedElemNodesFormat::check(edited).has_value());
    model.remove(*model.locateInType(typeIndexOf<WettedElemNodesTable>(), 10007));
    model.compact();
    REQUIRE(rowCount<WettedElemNodesTable>(model) == 5);

    REQUIRE_THROWS_AS(readBulk("WettedElemNodes;1;Line2Nodes;1;2\nWettedElemNodes;1;Line2Nodes;3;4"), base::DeckError);
    expectInvalid("WettedElemNodes;0;Line2Nodes;1;2", "id");
    expectInvalid("WettedElemNodes;1;;1;2", "kind"); // no default
    expectInvalid("WettedElemNodes;1;TETRA4;1;2;3;4", "kind");
    expectInvalid("WettedElemNodes;1;Quad4Shape;1;2;3", "node4"); // as many as the shape has
    expectInvalid("WettedElemNodes;1;Tria3Shape;1;2;3;4", "nodes");
    expectInvalid("WettedElemNodes;1;Quad8Shape;1;2;3;4", "node5"); // the mid-side points are not optional
    expectInvalid("WettedElemNodes;1;Line2Nodes;1;;3", "node2");    // no gap
    expectInvalid("WettedElemNodes;1;Line2Nodes;1;0", "node2");
    expectInvalid("WettedElemNodes;1;Line2Nodes;1;-2", "node2");
    expectInvalid("WettedElemNodes;1;Line2Nodes;1;x", "node2");
}

// ---------------------------------------------------------------------------------------------------------------------
// WETSURF
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// WETLOAD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a WETLOAD is the excitation set of a TLOAD1, names a wetted surface and carries the service name",
          "[model][cards][fsi]")
{
    REQUIRE((WettedSurfaceLoadTable::INFO.category == CardCategory::Other &&
             WettedSurfaceLoadTable::INFO.idSpace == base::IdSpace::Group));
    const std::string tload = "TransientTableLoad1;10;1;;;0;7\nTableDynamic1;7;;;0.;1.;1.;1.;EndTable\n";
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "WettedElemNodes;5;Line2Nodes;1;2\n"
        "WettedSurface;1001;wall;5\n"
        "WettedSurfaceLoad;1;1001;cplsrv\n" +
        tload);
    const WettedSurfaceLoadCard card = cardAt<WettedSurfaceLoadTable>(model);
    REQUIRE((card.id == 1 && card.wettedSurfaceId == 1001 && card.serviceName == "cplsrv"));
    REQUIRE(model.table<WettedSurfaceLoadTable>().definesExcitationIds());
    REQUIRE(model.table<WettedSurfaceLoadTable>().definedExcitationId(0) == 1);

    // The EXCITEID of the TLOAD1 names the WETLOAD, which defines it; the wetted surface is the card's reference.
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<WettedSurfaceLoadTable>(model);
    REQUIRE(visitor.fields == std::vector<std::string>{"wettedSurfaceId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::WettedSurface});
    REQUIRE(seenBy<WettedSurfaceLoadTable>(model) == Seen{{ReferenceTarget::WettedSurface, 1001}});
    Model withoutLoad = readBulk(tload);
    REQUIRE(unresolvedIn(withoutLoad) == 1);
    Model withoutSurface = readBulk("WettedSurfaceLoad;1;1001;cplsrv\n" + tload);
    REQUIRE(unresolvedIn(withoutSurface) == 1);
    requireRoundTrip(model);

    // Several cards may share a load set: a service with more than one surface.
    Model shared =
        readBulk("WettedSurfaceLoad;1;1001;cplsrv\nWettedSurfaceLoad;1;1002;cplsrv\nWettedSurfaceLoad;2;1003;other\n");
    REQUIRE(rowCount<WettedSurfaceLoadTable>(shared) == 3);
    REQUIRE(cardAt<WettedSurfaceLoadTable>(shared, 1).wettedSurfaceId == 1002);
    requireRoundTrip(shared);

    // The surface follows the renumbering of the wetted surfaces.
    Renumbering::apply(model, RenumberSpace::WettedSurface, Renumbering::offset(1000));
    REQUIRE(cardAt<WettedSurfaceLoadTable>(model).wettedSurfaceId == 2001);
    REQUIRE(unresolvedIn(model) == 0);

    WettedSurfaceLoadCard edited = cardAt<WettedSurfaceLoadTable>(model);
    edited.serviceName = "srvdv";
    REQUIRE_FALSE(WettedSurfaceLoadFormat::check(edited).has_value());
    model.table<WettedSurfaceLoadTable>().store(0, edited);
    REQUIRE(cardAt<WettedSurfaceLoadTable>(model).serviceName == "srvdv");
    edited.serviceName = "ninechars";
    REQUIRE_FALSE(WettedSurfaceLoadFormat::check(edited).has_value());
    model.table<WettedSurfaceLoadTable>().store(0, edited);
    requireRoundTrip(model);
    model.remove(CardLocation{typeIndexOf<WettedSurfaceLoadTable>(), 0});
    REQUIRE(unresolvedIn(model) == 1); // the TLOAD1 names a load set that nothing defines now

    expectInvalid("WettedSurfaceLoad;0;1001;cplsrv", "id");
    expectInvalid("WettedSurfaceLoad;1;0;cplsrv", "wettedSurfaceId");
    expectInvalid("WettedSurfaceLoad;1", "wettedSurfaceId");
    expectInvalid("WettedSurfaceLoad;1;1001", "serviceName"); // no default
    oor::testing::requireModelFileRoundTrip(readModelText("WettedSurfaceLoad;1;1001;ninechars", true));
}
