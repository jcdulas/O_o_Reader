// Checks of the interface element cards: CACINF3, CACINF4 and PACINF (the acoustic infinite
// elements), CAXISYM (the axisymmetric line), CBUSH2D and PBUSH2D (the 2-D bush), CIFHEX, CIFPENT, CIFQDX, CIFQUAD and
// PCOHE (the cohesive interface elements) and CINTC (the line interface element). Per card: the fields in their format
// reference positions, the defaults, the references the card lists, the renumbering of the ID space it owns, the
// refused values and the round trip through a model file. The generic tests of CardRoundTripTest.cpp cover
// reading, writing and copying every sample; this file covers what each card means.

#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteProp/AcousticInfinitePropTable.hpp"
#include "oor/model/cards/elements/shellAxisym/ShellAxisymTable.hpp"
#include "oor/model/cards/elements/bushing2D/Bushing2DTable.hpp"
#include "oor/model/cards/elements/cohesiveAxisymQuad/CohesiveAxisymQuadTable.hpp"
#include "oor/model/cards/elements/cohesiveHexa/CohesiveHexaTable.hpp"
#include "oor/model/cards/elements/cohesivePenta/CohesivePentaTable.hpp"
#include "oor/model/cards/elements/cohesiveProp/CohesivePropTable.hpp"
#include "oor/model/cards/elements/cohesiveQuad/CohesiveQuadTable.hpp"
#include "oor/model/cards/elements/lineInterface/LineInterfaceTable.hpp"
#include "oor/model/cards/properties/bushProp2D/BushProp2DTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The nodes a model needs so that its references resolve: the nodes first to last, one record each.
std::string grids(int first, int last)
{
    std::string text;
    for (int id = first; id <= last; ++id)
        text += "Node;" + std::to_string(id) + ";;" + std::to_string(id) + ".;0.;0.\n";
    return text;
}

} // namespace

// --- the acoustic infinite elements --------------------------------------------------------------------------------

TEST_CASE("CACINF3, CACINF4 and PACINF read their formats and resolve their references", "[model][cards][interface]")
{
    Model model =
        readBulk("FluidMat;10;1.;0.1\nAcousticInfiniteProp;100;10;5;0.;1.;2.\nAcousticInfiniteProp;101;10;3\n" +
                 grids(1204, 1204) + grids(1208, 1208) + grids(1213, 1213) + grids(1225, 1225) + grids(1231, 1231) +
                 grids(1236, 1236) + grids(1242, 1242) +
                 "AcousticInfiniteTria3;3101;100;1204;1208;1213\nAcousticInfiniteQuad4;3102;101;1231;1236;1242;1225");
    const AcousticInfiniteTria3Card triangle = cardAt<AcousticInfiniteTria3Table>(model);
    REQUIRE((triangle.id == 3101 && triangle.propId == 100));
    REQUIRE((triangle.nodes == std::array<base::EntityId, 3>{1204, 1208, 1213}));
    const AcousticInfiniteQuad4Card quadrilateral = cardAt<AcousticInfiniteQuad4Table>(model);
    REQUIRE((quadrilateral.id == 3102 && quadrilateral.propId == 101));
    REQUIRE((quadrilateral.nodes == std::array<base::EntityId, 4>{1231, 1236, 1242, 1225}));
    const AcousticInfinitePropCard full = cardAt<AcousticInfinitePropTable>(model, 0);
    REQUIRE((full.id == 100 && full.matId == 10 && full.radialOrder == 5));
    REQUIRE((full.poleX == 0.0 && full.poleY == 1.0 && full.poleZ == 2.0));
    // The pole has no default on the format: a blank coordinate is 0.0, as every blank coordinate of the Bulk Data.
    const AcousticInfinitePropCard bare = cardAt<AcousticInfinitePropTable>(model, 1);
    REQUIRE((bare.radialOrder == 3 && bare.poleX == 0.0 && bare.poleY == 0.0 && bare.poleZ == 0.0));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<AcousticInfiniteTria3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodes", "nodes", "nodes"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node,
                                                            ReferenceTarget::Node, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<AcousticInfiniteQuad4Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 5);
    visitor = RecordingVisitor{};
    model.table<AcousticInfinitePropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});

    // The element IDs are the ones of every element, the property IDs the ones of every property.
    REQUIRE_THROWS_AS(readBulk("AcousticInfiniteTria3;1;100;1;2;3\nRod2;1;1;1;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AcousticInfiniteTria3;1;100;1;2;3\nAcousticInfiniteQuad4;1;100;1;2;3;4"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AcousticInfiniteProp;100;10;5\nShellProp;100;1;0.1"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<AcousticInfinitePropTable>(model, 0).id == 1100 &&
             cardAt<AcousticInfiniteTria3Table>(model).propId == 1100));
    REQUIRE((cardAt<AcousticInfinitePropTable>(model, 1).id == 1101 &&
             cardAt<AcousticInfiniteQuad4Table>(model).propId == 1101));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(5));
    REQUIRE(cardAt<AcousticInfinitePropTable>(model, 0).matId == 15);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1));
    REQUIRE((cardAt<AcousticInfiniteTria3Table>(model).nodes == std::array<base::EntityId, 3>{1205, 1209, 1214}));
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<AcousticInfiniteTria3Table>(model).id == 4101 && cardAt<AcousticInfiniteQuad4Table>(model).id == 4102));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("the acoustic infinite element cards refuse what their formats do not allow", "[model][cards][interface]")
{
    expectInvalid("AcousticInfiniteTria3;0;100;1;2;3", "id");
    expectInvalid("AcousticInfiniteTria3;100000000;100;1;2;3", "id");
    expectInvalid("AcousticInfiniteTria3;1;0;1;2;3", "propId");
    expectInvalid("AcousticInfiniteTria3;1;100;1;2", "node3");
    expectInvalid("AcousticInfiniteTria3;1;100;1;2;-3", "node3");
    expectInvalid("AcousticInfiniteQuad4;1;100;1;2;3", "node4");
    expectInvalid("AcousticInfiniteProp;0;10;5", "id");
    expectInvalid("AcousticInfiniteProp;100;0;5", "matId");
    expectInvalid("AcousticInfiniteProp;100;10", "radialOrder"); // no default
    expectInvalid("AcousticInfiniteProp;100;10;0", "radialOrder");
    expectInvalid("AcousticInfiniteProp;100;10;5;x", "poleX");
    // The format does not say the points of a base are different (it does of the cohesive elements): a repeated point
    // is read.
    REQUIRE_NOTHROW(readBulk("AcousticInfiniteTria3;1;100;5;5;6"));
}

// --- CAXISYM -------------------------------------------------------------------------------------------------------

TEST_CASE("a CAXISYM has two or three nodes and an optional offset", "[model][cards][interface]")
{
    Model model = readBulk(
        "ShellAxisym;2201;2098;3008;3016\n"
        "ShellAxisym;2202;2098;3008;3016;3012\n"
        "ShellAxisym;2203;2098;3008;3016;;0.25\n"
        "ShellAxisym;2204;2098;3008;3016;3012;-0.5");
    const ShellAxisymCard example = cardAt<ShellAxisymTable>(model, 0); // two nodes, no offset
    REQUIRE((example.id == 2201 && example.propId == 2098 && !example.hasMiddleNode()));
    REQUIRE((example.nodes == std::array<base::EntityId, 3>{3008, 3016, 0}));
    REQUIRE_FALSE(example.offset.has_value()); // the format gives NOFF no default: a blank one stays blank
    const ShellAxisymCard curved = cardAt<ShellAxisymTable>(model, 1);
    REQUIRE((curved.hasMiddleNode() && curved.nodes == std::array<base::EntityId, 3>{3008, 3016, 3012}));
    REQUIRE((cardAt<ShellAxisymTable>(model, 2).offset == 0.25 &&
             !cardAt<ShellAxisymTable>(model, 2).hasMiddleNode()));
    REQUIRE((cardAt<ShellAxisymTable>(model, 3).offset == -0.5 &&
             cardAt<ShellAxisymTable>(model, 3).hasMiddleNode()));
    // The column of NOFF holds a NaN where it is blank.
    REQUIRE(std::isnan(model.table<ShellAxisymTable>().offsetColumn().get(0)));
    REQUIRE(model.table<ShellAxisymTable>().offsetColumn().get(2) == 0.25);

    RecordingVisitor visitor;
    model.table<ShellAxisymTable>().visitReferences(0, visitor);
    // The PAXISYM is a property card: the reference is a property reference, and the blank G3 is not visited.
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodes", "nodes"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node, ReferenceTarget::Node});
    visitor = RecordingVisitor{};
    model.table<ShellAxisymTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.size() == 4);
    REQUIRE(unresolvedIn(model) > 0); // no card is a property 2098 yet

    // It resolves as soon as a property of that ID exists.
    Model resolved = readBulk("IsoMat;1;2.1E+11;;0.3\nShellProp;2098;1;0.1\n" + grids(3008, 3008) + grids(3012, 3012) +
                              grids(3016, 3016) + "ShellAxisym;2201;2098;3008;3016;3012");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::Property, Renumbering::offset(1));
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<ShellAxisymTable>(resolved).propId == 2099);
    REQUIRE((cardAt<ShellAxisymTable>(resolved).nodes == std::array<base::EntityId, 3>{3108, 3116, 3112}));
    REQUIRE(unresolvedIn(resolved) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CAXISYM refuses what its format does not allow", "[model][cards][interface]")
{
    expectInvalid("ShellAxisym;0;2098;3008;3016", "id");
    expectInvalid("ShellAxisym;2201;0;3008;3016", "propId");
    expectInvalid("ShellAxisym;2201;2098;3008", "node2");
    expectInvalid("ShellAxisym;2201;2098;;3016", "node1");
    expectInvalid("ShellAxisym;2201;2098;3008;3008", "node1"); // the end points are unique
    expectInvalid("ShellAxisym;2201;2098;3008;3016;3008", "node1");
    expectInvalid("ShellAxisym;2201;2098;3008;3016;3016", "node2");
    expectInvalid("ShellAxisym;2201;2098;3008;3016;-1", "node3");
    expectInvalid("ShellAxisym;2201;2098;3008;3016;;x", "offset");
}

// --- CBUSH2D -------------------------------------------------------------------------------------------------------

TEST_CASE("a CBUSH2D has a plane in a system, an inner and an outer grid", "[model][cards][interface]")
{
    Model model = readBulk("BushProp2D;2101;75.;225.\nRectFrame2;5\n" + grids(4001, 4001) + grids(5001, 5001) +
                           "Bushing2D;2100;2101;4001;5001;0;PlaneXy\n"
                           "Bushing2D;2102;2101;4001;5001\n"
                           "Bushing2D;2103;2101;4001;5001;5;PlaneYz\n"
                           "Bushing2D;2104;2101;4001;5001;;PlaneZx");
    const Bushing2DCard example = cardAt<Bushing2DTable>(model, 0); // a system and a plane given
    REQUIRE((example.id == 2100 && example.propId == 2101 && example.frameId == 0 && example.plane == "PlaneXy"));
    REQUIRE((example.nodes == std::array<base::EntityId, 2>{4001, 5001}));
    const Bushing2DCard defaults = cardAt<Bushing2DTable>(model, 1);
    REQUIRE((defaults.frameId == 0 && defaults.plane == "PlaneXy")); // the format reference defaults
    const Bushing2DCard system = cardAt<Bushing2DTable>(model, 2);
    REQUIRE((system.frameId == 5 && system.plane == "PlaneYz"));
    REQUIRE(cardAt<Bushing2DTable>(model, 3).plane == "PlaneZx");
    REQUIRE(model.table<Bushing2DTable>().planeColumn().get(2) == "PlaneYz");
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<Bushing2DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodes", "nodes"}); // the basic system is not visited
    visitor = RecordingVisitor{};
    model.table<Bushing2DTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "nodes", "nodes", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Node,
                                                            ReferenceTarget::Node, ReferenceTarget::Coordinate});

    REQUIRE_THROWS_AS(readBulk("Bushing2D;2100;2101;1;2\nRod2;2100;1;1;2"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<BushProp2DTable>(model).id == 3101 && cardAt<Bushing2DTable>(model, 2).propId == 3101));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE(cardAt<Bushing2DTable>(model, 2).frameId == 15);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1));
    REQUIRE((cardAt<Bushing2DTable>(model, 0).nodes == std::array<base::EntityId, 2>{4002, 5002}));
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<Bushing2DTable>(model, 0).id == 3100);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CBUSH2D refuses what its format does not allow", "[model][cards][interface]")
{
    expectInvalid("Bushing2D;0;2101;1;2", "id");
    expectInvalid("Bushing2D;2100;;1;2", "propId"); // required
    expectInvalid("Bushing2D;2100;0;1;2", "propId");
    expectInvalid("Bushing2D;2100;2101;1", "nodeB");
    expectInvalid("Bushing2D;2100;2101;;2", "nodeA");
    expectInvalid("Bushing2D;2100;2101;7;7", "nodeA"); // two grids, not the same one twice
    expectInvalid("Bushing2D;2100;2101;1;2;-1", "frameId");
    expectInvalid("Bushing2D;2100;2101;1;2;0;AB", "plane");
}

// --- PBUSH2D -------------------------------------------------------------------------------------------------------

TEST_CASE("a PBUSH2D is the nominal line alone, with a squeeze film damper or with cross coupling terms",
          "[model][cards][interface]")
{
    Model model = readBulk(
        "BushProp2D;1000;75.;225.;0.035;0.045;;;SqueezeFilm;1.5;2.5;0.04;Long;1.8;250.;2;85.;45.;110.;135.;0.02;0.\n"
        "BushProp2D;1001;10.;20.\n"
        "BushProp2D;1002;10.;20.;;;1.;2.;Cross;0.1;0.2;0.3;0.4;0.5;0.6\n"
        "BushProp2D;1003;10.;20.;;;;;SqueezeFilm;1.;2.;0.05;Short;2.1;-5.;1;0.;359.5");
    // Two ports, the first line and the squeeze film damper.
    const BushProp2DCard squeezed = cardAt<BushProp2DTable>(model, 0);
    REQUIRE((squeezed.id == 1000 && squeezed.stiffness11 == 75.0 && squeezed.stiffness22 == 225.0));
    REQUIRE((squeezed.damping11 == 0.035 && squeezed.damping22 == 0.045 && squeezed.mass11 == 0.0 &&
             squeezed.mass22 == 0.0));
    REQUIRE((squeezed.squeeze.has_value() && !squeezed.crossCoupling.has_value()));
    const BushProp2DSqueeze& film = *squeezed.squeeze;
    REQUIRE((film.bearingDiameter == 1.5 && film.bearingLength == 2.5 && film.bearingClearance == 0.04 &&
             film.bearingModel == BushProp2DSolution::Long));
    REQUIRE((film.viscosity == 1.8 && film.vaporPressure == 250.0 && film.portCount == 2));
    REQUIRE((film.pressure1 == 85.0 && film.portAngle1 == 45.0));
    REQUIRE((film.pressure2 == 110.0 && film.portAngle2 == 135.0 && film.offset1 == 0.02 && film.offset2 == 0.0));
    // The plain form has neither option, and the defaults of its damping and mass terms.
    const BushProp2DCard plain = cardAt<BushProp2DTable>(model, 1);
    REQUIRE((!plain.squeeze && !plain.crossCoupling && plain.damping11 == 0.0 && plain.mass22 == 0.0));
    const BushProp2DCard coupled = cardAt<BushProp2DTable>(model, 2);
    REQUIRE((coupled.mass11 == 1.0 && coupled.mass22 == 2.0 && !coupled.squeeze.has_value() &&
             coupled.crossCoupling.has_value()));
    REQUIRE((coupled.crossCoupling->stiffness12 == 0.1 && coupled.crossCoupling->stiffness21 == 0.2 &&
             coupled.crossCoupling->damping12 == 0.3));
    REQUIRE((coupled.crossCoupling->damping21 == 0.4 && coupled.crossCoupling->mass12 == 0.5 &&
             coupled.crossCoupling->mass21 == 0.6));
    // One port: the second port stays blank, the offsets are 0.0 and the bearing is the one that was given.
    const BushProp2DSqueeze oneport = *cardAt<BushProp2DTable>(model, 3).squeeze;
    REQUIRE(
        (oneport.bearingModel == BushProp2DSolution::Short && oneport.portCount == 1 && oneport.vaporPressure == -5.0));
    REQUIRE((!oneport.pressure2 && !oneport.portAngle2 && oneport.offset1 == 0.0 && oneport.portAngle1 == 359.5));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<BushProp2DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty()); // a PBUSH2D names no other card
    // A property: its ID is in the space of all the properties.
    REQUIRE_THROWS_AS(readBulk("BushProp2D;5;1.;2.\nShellProp;5;1;0.1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("BushProp2D;5;1.;2.\nBushProp2D;5;3.;4."), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(10));
    REQUIRE((cardAt<BushProp2DTable>(model, 0).id == 1010 && cardAt<BushProp2DTable>(model, 3).id == 1013));
    requireRoundTrip(model);
}

TEST_CASE("a PBUSH2D has at most one option, whatever way it is made", "[model][cards][interface]")
{
    BushProp2DCard card;
    card.id = 1;
    card.stiffness11 = 1.0;
    card.stiffness22 = 2.0;
    card.crossCoupling = BushProp2DCross{};
    REQUIRE_FALSE(BushProp2DFormat::check(card).has_value());
    BushProp2DSqueeze film;
    film.bearingDiameter = 1.0;
    film.bearingLength = 2.0;
    film.bearingClearance = 0.05;
    film.viscosity = 2.1;
    film.pressure1 = 1.0;
    card.squeeze = film;
    const auto problem = BushProp2DFormat::check(card); // not both
    REQUIRE(problem.has_value());
    REQUIRE(std::string(problem->field) == "crossCoupling");
    card.crossCoupling.reset();
    REQUIRE_FALSE(BushProp2DFormat::check(card).has_value());
    card.squeeze->portCount = 2; // two ports need the second one
    REQUIRE(std::string(BushProp2DFormat::check(card)->field) == "pressure2");
    card.squeeze->pressure2 = 3.0;
    REQUIRE(std::string(BushProp2DFormat::check(card)->field) == "portAngle2");
    card.squeeze->portAngle2 = 360.0; // below 360
    REQUIRE(std::string(BushProp2DFormat::check(card)->field) == "portAngle2");
    card.squeeze->portAngle2 = 0.0;
    REQUIRE_FALSE(BushProp2DFormat::check(card).has_value());
}

TEST_CASE("a PBUSH2D refuses what its format does not allow", "[model][cards][interface]")
{
    const std::string head = "BushProp2D;1;75.;225.;;;;;SqueezeFilm;";
    expectInvalid("BushProp2D;0;75.;225.", "id");
    expectInvalid("BushProp2D;1;50.", "stiffness22"); // K11 and K22 are required
    expectInvalid("BushProp2D;1;;150.", "stiffness11");
    expectInvalid("BushProp2D;1;75.;225.;x", "damping11");
    // The squeeze film damper: every required field, and the ranges of the format.
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.", "portCount"); // no third line: NPORT has no default
    expectInvalid(head + ";2.;0.04;Long;1.8;250.;1;85.;45.", "bearingDiameter");
    expectInvalid(head + "0.;2.;0.04;Long;1.8;250.;1;85.;45.", "bearingDiameter");
    expectInvalid(head + "1.;-2.;0.04;Long;1.8;250.;1;85.;45.", "bearingLength");
    expectInvalid(head + "1.;2.;0.;Long;1.8;250.;1;85.;45.", "bearingClearance");
    expectInvalid(head + "1.;2.;0.04;TALL;1.8;250.;1;85.;45.", "bearingModel");
    expectInvalid(head + "1.;2.;0.04;Long;0.;250.;1;85.;45.", "viscosity");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;;1;85.;45.", "vaporPressure");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;3;85.;45.", "portCount");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;0;85.;45.", "portCount");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;1;-1.;45.", "pressure1");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;1;85.", "portAngle1");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;1;85.;360.", "portAngle1");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;1;85.;-1.", "portAngle1");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;2;85.;45.", "pressure2");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;2;85.;45.;110.", "portAngle2");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;2;85.;45.;-110.;135.", "pressure2");
    expectInvalid(head + "1.;2.;0.04;Long;1.8;250.;2;85.;45.;110.;400.", "portAngle2");
    // The cross coupling form has only its own line.
    expectInvalid("BushProp2D;1;75.;225.;;;;;Cross;0.1;x", "stiffness21");
}

// --- the cohesive interface elements -------------------------------------------------------------------------------

TEST_CASE("a PCOHE has its defaults, names an MCOHE and a cohesive element names it", "[model][cards][interface]")
{
    Model model = readBulk("CohesiveMat;7701\nCohesiveProp;7700;7701\nCohesiveProp;7702;7701;1;0.5;GaussPoints;1\n" +
                           grids(1, 8) + "CohesiveHexa;10;7700;1;2;3;4;5;6;7;8\nCohesiveQuad;11;7702;1;2;3;4");
    const CohesivePropCard bare =
        cardAt<CohesivePropTable>(model, 0); // the PID and the MID alone, and the defaults of the other fields
    REQUIRE((bare.id == 7700 && bare.matId == 7701));
    REQUIRE((bare.integration == COHESIVE_PROP_INTEGRATION_GAUSS && bare.thickness == 1.0 &&
             bare.output == CohesivePropOutput::Grid));
    REQUIRE(bare.tangentStiffness == 0);
    const CohesivePropCard full = cardAt<CohesivePropTable>(model, 1);
    REQUIRE((full.integration == COHESIVE_PROP_INTEGRATION_LOBATTO && full.thickness == 0.5));
    REQUIRE((full.output == CohesivePropOutput::Gauss &&
             full.tangentStiffness == 1)); // INT = 1 sets OUTPUT to GRID in the solver, the card keeps the word
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CohesivePropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    REQUIRE_THROWS_AS(readBulk("CohesiveProp;7;7701\nShellProp;7;1;0.1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("CohesiveProp;7;7701\nCohesiveProp;7;7702"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CohesivePropTable>(model, 0).id == 8700 && cardAt<CohesiveHexaTable>(model).propId == 8700));
    REQUIRE(cardAt<CohesiveQuadTable>(model).propId == 8702);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE((cardAt<CohesivePropTable>(model, 0).matId == 7711 && cardAt<CohesivePropTable>(model, 1).matId == 7711));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a PCOHE refuses what its format does not allow", "[model][cards][interface]")
{
    expectInvalid("CohesiveProp;0;7701", "id");
    expectInvalid("CohesiveProp;7700", "matId");
    expectInvalid("CohesiveProp;7700;0", "matId");
    expectInvalid("CohesiveProp;7700;7701;2", "integration");
    expectInvalid("CohesiveProp;7700;7701;-1", "integration");
    expectInvalid("CohesiveProp;7700;7701;;0.", "thickness");
    expectInvalid("CohesiveProp;7700;7701;;-1.", "thickness");
    expectInvalid("CohesiveProp;7700;7701;;;PEAK", "output");
    expectInvalid("CohesiveProp;7700;7701;;;;2", "tangentStiffness");
}

TEST_CASE("the cohesive elements keep their corner and edge points in one array", "[model][cards][interface]")
{
    Model model = readBulk(
        grids(1, 20) + "CohesiveProp;700;701\nCohesiveMat;701\n" + "CohesiveHexa;700;700;1;2;3;4;5;6;7;8\n" +
        "CohesiveHexa;701;700;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20\n" +
        "CohesiveHexa;702;700;1;2;3;4;5;6;7;8;9;10;11;12;;;;;17;18;19;20\n" +
        "CohesivePenta;710;700;1;2;3;4;5;6\n" + "CohesivePenta;711;700;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15\n" +
        "CohesiveAxisymQuad;720;700;1;2;3;4\n" + "CohesiveAxisymQuad;721;700;1;2;3;4;5;6;7;8\n" +
        "CohesiveQuad;730;700;1;2;3;4\nCohesiveQuad;731;700;1;2;3;4;5;;7");
    // CIFHEX: the linear element (G1 to G8), then all the points, then the quadratic one without the points halfway
    // between the faces.
    const CohesiveHexaCard linear = cardAt<CohesiveHexaTable>(model, 0);
    REQUIRE((linear.id == 700 && linear.propId == 700));
    for (std::size_t index = 0; index < 8; ++index)
        REQUIRE(linear.nodes[index] == static_cast<base::EntityId>(index + 1));
    for (std::size_t index = 8; index < CohesiveHexaCard::NODE_COUNT; ++index)
        REQUIRE(linear.nodes[index] == 0);
    const CohesiveHexaCard all = cardAt<CohesiveHexaTable>(model, 1);
    for (std::size_t index = 0; index < CohesiveHexaCard::NODE_COUNT; ++index)
        REQUIRE(all.nodes[index] == static_cast<base::EntityId>(index + 1));
    const CohesiveHexaCard quadratic = cardAt<CohesiveHexaTable>(model, 2);
    for (std::size_t index = 8; index < 12; ++index)
        REQUIRE(quadratic.nodes[index] == static_cast<base::EntityId>(index + 1)); // G9 to G12
    for (std::size_t index = 12; index < 16; ++index)
        REQUIRE(quadratic.nodes[index] == 0); // G13 to G16 are optional
    for (std::size_t index = 16; index < 20; ++index)
        REQUIRE(quadratic.nodes[index] == static_cast<base::EntityId>(index + 1)); // G17 to G20
    REQUIRE((cardAt<CohesivePentaTable>(model, 0).nodes[5] == 6 &&
             cardAt<CohesivePentaTable>(model, 0).nodes[6] == 0));
    REQUIRE(cardAt<CohesivePentaTable>(model, 1).nodes[14] == 15);
    REQUIRE((cardAt<CohesiveAxisymQuadTable>(model, 0).nodes[3] == 4 &&
             cardAt<CohesiveAxisymQuadTable>(model, 0).nodes[4] == 0));
    REQUIRE(cardAt<CohesiveAxisymQuadTable>(model, 1).nodes[7] == 8);
    const CohesiveQuadCard sides = cardAt<CohesiveQuadTable>(model, 1); // G5 and G7 without the side point G6
    REQUIRE((sides.nodes[4] == 5 && sides.nodes[5] == 0 && sides.nodes[6] == 7 && sides.nodes[7] == 0));
    REQUIRE(unresolvedIn(model) == 0);
    // The element space is shared with every element.
    REQUIRE_THROWS_AS(readBulk("CohesiveHexa;1;700;1;2;3;4;5;6;7;8\nRod2;1;1;1;2"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("CohesiveQuad;1;700;1;2;3;4\nCohesiveAxisymQuad;1;700;1;2;3;4"),
                      base::DeckError);

    // The blank slots are not visited; the references are the property and the points that are given.
    RecordingVisitor visitor;
    model.table<CohesiveHexaTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 9);
    REQUIRE(visitor.targets.front() == ReferenceTarget::Property);
    visitor = RecordingVisitor{};
    model.table<CohesiveHexaTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.size() == 21);
    visitor = RecordingVisitor{};
    model.table<CohesiveQuadTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.size() == 7); // PID and G1 to G5, G7
    visitor = RecordingVisitor{};
    model.table<CohesivePentaTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 7);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<CohesiveHexaTable>(model, 2).nodes[8] == 109 &&
             cardAt<CohesiveHexaTable>(model, 2).nodes[12] == 0));
    REQUIRE(cardAt<CohesiveQuadTable>(model, 1).nodes[5] == 0); // a blank slot stays blank
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<CohesiveHexaTable>(model, 0).id == 1700 && cardAt<CohesiveQuadTable>(model, 1).id == 1731));
    requireRoundTrip(model);
}

TEST_CASE("the cohesive elements refuse what their formats do not allow", "[model][cards][interface]")
{
    expectInvalid("CohesiveHexa;0;700;1;2;3;4;5;6;7;8", "id");
    expectInvalid("CohesiveHexa;100000000;700;1;2;3;4;5;6;7;8", "id");
    expectInvalid("CohesiveHexa;1;0;1;2;3;4;5;6;7;8", "propId");
    expectInvalid("CohesiveHexa;1;700;1;2;3;4;5;6;7", "node8"); // the eight corners are required
    expectInvalid("CohesiveHexa;1;700;1;2;3;4;5;6;7;8;9;-10", "node10");
    expectInvalid("CohesiveHexa;1;700;1;2;3;4;5;6;7;8;9;9", "node9"); // the points are unique
    expectInvalid("CohesiveHexa;1;700;1;2;3;4;5;6;7;8;1", "node1");
    expectInvalid("CohesivePenta;1;700;1;2;3;4;5", "node6");
    expectInvalid("CohesivePenta;1;700;1;2;3;4;5;5", "node5");
    expectInvalid("CohesiveAxisymQuad;1;700;1;2;3", "node4");
    expectInvalid("CohesiveAxisymQuad;1;700;1;2;3;3", "node3");
    expectInvalid("CohesiveQuad;1;700;1;2;3", "node4");
    expectInvalid("CohesiveQuad;1;700;1;2;3;4;1", "node1");
}

// --- CINTC ---------------------------------------------------------------------------------------------------------

TEST_CASE("a CINTC reads its LIST into boundaries and writes it back", "[model][cards][interface]")
{
    Model model = readBulk(
        "LineInterface;1001;NodeList;boundaries=(421,422(Quadratic),-423(Quadratic),424(Linear))\n"
        "LineInterface;1002;;boundaries=(5,6)\n"
        "LineInterface;1003;;boundaries = ( 7,8(Quadratic),-9 )\n"
        "LineInterface;1004;NodeList;boundaries=(1,2,3,4,5,6,7,8,9,10,11,12(Quadratic))");
    const LineInterfaceCard example = cardAt<LineInterfaceTable>(model, 0); // both interpolations, a sign
    REQUIRE((example.id == 1001 && example.type == LineInterfaceType::GridList));
    REQUIRE(example.boundaries ==
            std::vector<LineInterfaceBoundary>{{421, LineInterfaceInterpolation::Linear},
                                                   {422, LineInterfaceInterpolation::Quadratic},
                                                   {-423, LineInterfaceInterpolation::Quadratic},
                                                   {424, LineInterfaceInterpolation::Linear}});
    const LineInterfaceCard blank =
        cardAt<LineInterfaceTable>(model, 1); // the interpolation and the type are linear and GRDLIST when left out
    REQUIRE((blank.type == LineInterfaceType::GridList && blank.boundaries.size() == 2));
    REQUIRE((blank.boundaries[0].boundaryId == 5 &&
             blank.boundaries[1].interpolation == LineInterfaceInterpolation::Linear));
    const LineInterfaceCard spaced =
        cardAt<LineInterfaceTable>(model, 2); // whitespace around grammar punctuation
    REQUIRE((spaced.boundaries.size() == 3 &&
             spaced.boundaries[1].interpolation == LineInterfaceInterpolation::Quadratic));
    REQUIRE((spaced.boundaries[2].boundaryId == -9 && spaced.boundaries[0].boundaryId == 7));
    const LineInterfaceCard longList =
        cardAt<LineInterfaceTable>(model, 3); // more items than the fields of one line
    REQUIRE(longList.boundaries.size() == 12);
    REQUIRE((longList.boundaries[11].boundaryId == 12 &&
             longList.boundaries[11].interpolation == LineInterfaceInterpolation::Quadratic));
    REQUIRE(unresolvedIn(model) == 21); // the 21 boundaries of the four lists name GMBNDC cards that the deck lacks

    // The written LIST has its commas and no blank; a linear boundary has no parentheses unless defaults are written.
    const std::string text = writeModelText(model);
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("boundaries=(421,422(Quadratic),-423(Quadratic),424)"));
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("boundaries=(5,6)"));
    const std::string explicitText = writeModelText(model, false);
    REQUIRE_THAT(explicitText, Catch::Matchers::ContainsSubstring(
                                   "boundaries=(421(Linear),422(Quadratic),-423(Quadratic),424(Linear))"));
    // The model file reads back to the same cards, and so does a list with every linear boundary written out.
    requireRoundTrip(model);
    const Model again = readModelText(
        "LineInterface;1001;NodeList;boundaries=(421(Linear),422(Quadratic),-423(Quadratic),424(Linear))", true);
    REQUIRE(cardAt<LineInterfaceTable>(again, 0).boundaries == example.boundaries);

    // A CINTC is the only line interface element: its ID is unique among the CINTC cards and nothing else.
    REQUIRE_THROWS_AS(readBulk("LineInterface;1;;boundaries=(1,2)\nLineInterface;1;;boundaries=(3,4)"),
                      base::DeckError);
    REQUIRE_NOTHROW(readBulk("LineInterface;1;;boundaries=(1,2)\nRod2;1;1;1;2"));
    REQUIRE(LineInterfaceTable::INFO.idSpace == base::IdSpace::PerCardType);
    REQUIRE(LineInterfaceTable::INFO.category == CardCategory::Other);
    RecordingVisitor visitor;
    model.table<LineInterfaceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>(4, "boundaryList"));
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::GeometricBoundary));
}

TEST_CASE("the boundaries of a CINTC list are GMBNDC cards and follow their renumbering, with their sign",
          "[model][cards][interface]")
{
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Node;3;;2.;0.;0.\n"
        "GeomBoundaryCurve;5;1;2;Node;3\n"
        "GeomBoundaryCurve;6;1;2;Node;3\n"
        "LineInterface;1;;boundaries=(5,-6(Quadratic))");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(Renumbering::apply(model, RenumberSpace::GeometricBoundary, Renumbering::offset(10)) == 2);
    REQUIRE(cardAt<LineInterfaceTable>(model).boundaries ==
            std::vector<LineInterfaceBoundary>{{15, LineInterfaceInterpolation::Linear},
                                                   {-16, LineInterfaceInterpolation::Quadratic}});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("a CINTC refuses what its format does not allow", "[model][cards][interface]")
{
    expectInvalid("LineInterface;0;;boundaries=(1,2)", "id");
    expectInvalid("LineInterface;100000000;;boundaries=(1,2)", "id");
    expectInvalid("LineInterface;1;OTHER;boundaries=(1,2)", "kind");
    expectInvalid("LineInterface;1;;LIST=(1,2)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2(quadratic))", "boundaryList");
    expectInvalid("LineInterface;1", "boundaryList");                 // the LIST is required
    expectInvalid("LineInterface;1;;boundaries=(1)", "boundaryList"); // at least two boundaries
    expectInvalid("LineInterface;1;;boundaries=(1,0)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2(X))", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2(Quadratic)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,,2)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2,)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,a)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2.5)", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,2(Quadratic)(Linear))", "boundaryList");
    expectInvalid("LineInterface;1;;boundaries=(1,99999999999)", "boundaryList");
    expectInvalid("LineInterface;1;;422(Quadratic),-423(Quadratic)", "boundaryList"); // not a boundaries=(...)
    // A list in parentheses with no LIST= is refused too.
    expectInvalid("LineInterface;1001;NodeList;(421,422(Quadratic),-423(Quadratic),424(Linear))", "boundaryList");
}
