// Checks of the crack tip, seam, slot and spring cards: CRAC2D, CRAC3D, PRAC2D, PRAC3D, CSEAM,
// PSEAM, CSLOT3, CSLOT4, CSPR and SWLDPRM. Per card: the fields in their format reference positions, the forms and the
// defaults, the references the card lists, the renumbering of the ID spaces it owns, the refused values and the round
// trip through a model file. The other cards (ADUM8, ADUM9, AXSLOT, GRIDS, PSPRMAT) are not needed: a reference to
// one is simply unresolved, and the tests that care read the references through a visitor.

#include "oor/model/cards/elements/acousticSlot3/AcousticSlot3Table.hpp"
#include "oor/model/cards/elements/acousticSlot4/AcousticSlot4Table.hpp"
#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCatalogue.hpp"
#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsTable.hpp"
#include "oor/model/cards/elements/crackTip2D/CrackTip2DTable.hpp"
#include "oor/model/cards/elements/crackTip3D/CrackTip3DTable.hpp"
#include "oor/model/cards/elements/crackTipProp2D/CrackTipProp2DTable.hpp"
#include "oor/model/cards/elements/crackTipProp3D/CrackTipProp3DTable.hpp"
#include "oor/model/cards/elements/seam/SeamTable.hpp"
#include "oor/model/cards/elements/seamProp/SeamPropTable.hpp"
#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The record of a card from the text of each of its slots (slot 1 first). Slots blank at the end are left out.
std::string freeCard(const std::string& name, std::vector<std::string> fields)
{
    while (!fields.empty() && fields.back().empty())
        fields.pop_back();
    return slotRecord(name, fields);
}

// The grid points of a crack tip element, each as (G number, ID).
using GridPoints = std::vector<std::pair<int, int>>;

// The grid points G`first` to G`last`, each with an ID that is its G number plus `shift`.
GridPoints run(int first, int last, int shift = 0)
{
    GridPoints points;
    for (int number = first; number <= last; ++number)
        points.emplace_back(number, number + shift);
    return points;
}

GridPoints joined(GridPoints first, const GridPoints& second)
{
    first.insert(first.end(), second.begin(), second.end());
    return first;
}

// A CRAC2D or a CRAC3D record: EID and PID, then the grid points by their G numbers (G1 is slot 3), blank where the
// list does not give one. `slots` is 18 or 64.
std::string crackCard(const std::string& name, std::size_t slots, int eid, int pid, const GridPoints& points)
{
    std::vector<std::string> fields(slots + 2);
    fields[0] = std::to_string(eid);
    fields[1] = std::to_string(pid);
    for (const auto& [number, id] : points)
        fields[static_cast<std::size_t>(number) + 1] = std::to_string(id);
    return freeCard(name, fields);
}

std::string crac2d(int eid, const GridPoints& points)
{
    return crackCard("CrackTip2D", 18, eid, 96501, points);
}

std::string crac3d(int eid, const GridPoints& points)
{
    return crackCard("CrackTip3D", 64, eid, 96511, points);
}

// The G numbers of a CRAC3D that the full crack requires (G1 to G10 and G19 to G28).
GridPoints requiredOfFullCrack3d()
{
    return joined(run(1, 10), run(19, 28));
}

// Bare node records for the IDs `first` to `last`.
std::string gridCards(int first, int last)
{
    std::string text;
    for (int id = first; id <= last; ++id)
        text += "Node;" + std::to_string(id) + "\n";
    return text;
}

// Four global parameters of a SWLDPRM.
const char* const GLOBAL_PARAMETERS =
    "ConnectorSearchParams;4;GeneratedDataPrint;1;GeometryCheckLevel;1;"
    "MaxDiameterHalvings;2;MoveEndPointsToProjection;1";

// The value of a parameter as a real (the parameter was written as a real).
double realValue(const ConnectorSearchParamsParameter& parameter)
{
    return std::get<double>(parameter.value.number);
}

std::int32_t integerValue(const ConnectorSearchParamsParameter& parameter)
{
    return std::get<std::int32_t>(parameter.value.number);
}

} // namespace

TEST_CASE("a CRAC2D is the full crack with G1 to G10, or the symmetric half crack without G8 to G10",
          "[model][cards][crack]")
{
    // The full crack: G1 to G10 and, among the optional points, G12, G13 and G17 (field 19, on the third line).
    const GridPoints fullPoints = joined(run(1, 10), {{12, 12}, {13, 13}, {17, 17}});
    // The half crack: G1 to G7, and the optional G11 to G14.
    const GridPoints halfPoints = joined(run(1, 7), run(11, 14));
    Model model =
        readBulk(crac2d(96101, fullPoints) + "\n" + crac2d(96102, halfPoints) + "\n" + crac2d(96103, run(1, 18)));

    const CrackTip2DCard full = cardAt<CrackTip2DTable>(model, 0);
    REQUIRE((full.id == 96101 && full.propId == 96501));
    REQUIRE_FALSE(full.isHalfCrack());
    for (int number = 1; number <= 10; ++number)
        REQUIRE(full.nodes[static_cast<std::size_t>(number) - 1] == number);
    REQUIRE(full.nodes[10] == 0); // G11 is blank
    REQUIRE((full.nodes[11] == 12 && full.nodes[12] == 13));
    REQUIRE((full.nodes[16] == 17 && full.nodes[17] == 0));

    const CrackTip2DCard half = cardAt<CrackTip2DTable>(model, 1);
    REQUIRE(half.isHalfCrack());
    REQUIRE((half.nodes[6] == 7 && half.nodes[7] == 0 && half.nodes[8] == 0 && half.nodes[9] == 0));
    REQUIRE((half.nodes[10] == 11 && half.nodes[13] == 14 && half.nodes[14] == 0));

    // Every one of the 18 fields, the last on the third line.
    REQUIRE(cardAt<CrackTip2DTable>(model, 2).nodes[17] == 18);
    REQUIRE((model.findTable<CrackTip2DTable>()->rows().rowCount() == 3));
    requireRoundTrip(model);

    // The element is an element, in the space every element shares.
    REQUIRE_THROWS_AS(readBulk(crac2d(5, run(1, 10)) + "\nSpringOffsetExplicit;5;22;1;2"), base::DeckError);
}

TEST_CASE("a CRAC2D names a PRAC2D and its grid points, and follows their renumbering", "[model][cards][crack]")
{
    const GridPoints points = joined(run(1, 10), {{12, 12}});
    Model model =
        readBulk(gridCards(1, 12) + "IsoMat;1;2.1E+11;;0.3\nCrackTipProp2D;96501;1;0.1\n" + crac2d(96101, points));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CrackTip2DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 12); // the property and the eleven grid points that are not blank
    REQUIRE((visitor.fields[0] == "propId" && visitor.targets[0] == ReferenceTarget::Property));
    for (std::size_t index = 1; index < visitor.fields.size(); ++index)
        REQUIRE((visitor.fields[index] == "nodes" && visitor.targets[index] == ReferenceTarget::Node));

    // A point that does not exist is one unresolved reference, and so is a property.
    Model missing =
        readBulk(gridCards(1, 9) + "IsoMat;1;2.1E+11;;0.3\nCrackTipProp2D;96501;1;0.1\n" + crac2d(96101, points));
    REQUIRE(unresolvedIn(missing) == 2); // G10 and G12
    Model noProperty = readBulk(gridCards(1, 12) + crac2d(96101, points));
    REQUIRE(unresolvedIn(noProperty) == 1);

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    const CrackTip2DCard moved = cardAt<CrackTip2DTable>(model);
    REQUIRE((moved.nodes[0] == 101 && moved.nodes[9] == 110 && moved.nodes[11] == 112));
    REQUIRE(moved.nodes[10] == 0); // a blank field stays blank
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CrackTip2DTable>(model).propId == 97501 && cardAt<CrackTipProp2DTable>(model).id == 97501));
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<CrackTip2DTable>(model).id == 97101);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CRAC2D refuses what the full and the half crack forms do not allow", "[model][cards][crack]")
{
    expectInvalid(crac2d(0, run(1, 10)), "id");
    expectInvalid(crac2d(100000000, run(1, 10)), "id"); // the format reference: EID below 100,000,000
    expectInvalid(crackCard("CrackTip2D", 18, 1, 0, run(1, 10)), "propId");
    expectInvalid(crac2d(1, run(2, 10)), "node1");
    expectInvalid(crac2d(1, joined(run(1, 6), run(8, 10))), "node7");
    // G8 to G10 are all given or all blank.
    expectInvalid(crac2d(1, run(1, 9)), "node10");
    expectInvalid(crac2d(1, joined(run(1, 7), {{9, 9}, {10, 10}})), "node8");
    // The half crack (G8 to G10 blank) has no G15 to G18.
    expectInvalid(crac2d(1, joined(run(1, 7), {{15, 15}})), "node15");
    expectInvalid(crac2d(1, joined(run(1, 7), {{18, 18}})), "node18");
    expectInvalid(crac2d(1, joined(run(1, 10), {{11, -5}})), "node11");
}

TEST_CASE("a CRAC3D is the full crack with G1 to G10 and G19 to G28, or the symmetric half crack",
          "[model][cards][crack]")
{
    // The full crack: the required points, the mid-plane G37 to G46 (all given) and the surface points up to G64.
    const GridPoints fullPoints = joined(joined(requiredOfFullCrack3d(), run(37, 46)), run(47, 64));
    // The half crack: G1 to G7 and G19 to G25, with optional points G11 to G14, G29 to G32 and G37 to G42.
    const GridPoints halfPoints =
        joined(joined(joined(run(1, 7), run(19, 25)), joined(run(11, 14), run(29, 32))), run(37, 42));
    Model model = readBulk(crac3d(96111, requiredOfFullCrack3d()) + "\n" + crac3d(96112, fullPoints) + "\n" +
                           crac3d(96113, halfPoints));

    const CrackTip3DCard minimal = cardAt<CrackTip3DTable>(model, 0);
    REQUIRE((minimal.id == 96111 && minimal.propId == 96511));
    REQUIRE_FALSE(minimal.isHalfCrack());
    REQUIRE((minimal.nodes[0] == 1 && minimal.nodes[9] == 10 && minimal.nodes[10] == 0 && minimal.nodes[18] == 19));
    REQUIRE((minimal.nodes[27] == 28 && minimal.nodes[28] == 0 && minimal.nodes[36] == 0)); // G37 is blank

    const CrackTip3DCard full = cardAt<CrackTip3DTable>(model, 1);
    REQUIRE((full.nodes[36] == 37 && full.nodes[45] == 46 && full.nodes[46] == 47));
    REQUIRE(full.nodes[63] == 64); // the last field of the card, on the ninth line

    const CrackTip3DCard half = cardAt<CrackTip3DTable>(model, 2);
    REQUIRE(half.isHalfCrack());
    REQUIRE((half.nodes[6] == 7 && half.nodes[7] == 0 && half.nodes[18] == 19 && half.nodes[24] == 25));
    REQUIRE((half.nodes[25] == 0 && half.nodes[36] == 37 && half.nodes[41] == 42 && half.nodes[42] == 0));
    requireRoundTrip(model);
}

TEST_CASE("a CRAC3D names a PRAC3D and its grid points, and follows their renumbering", "[model][cards][crack]")
{
    Model model = readBulk(gridCards(1, 28) + "IsoMat;1;2.1E+11;;0.3\nCrackTipProp3D;96511;1\n" +
                           crac3d(96111, requiredOfFullCrack3d()));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CrackTip3DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 21); // the property and the twenty required points
    REQUIRE((visitor.fields[0] == "propId" && visitor.targets[0] == ReferenceTarget::Property));
    REQUIRE((visitor.fields[20] == "nodes" && visitor.targets[20] == ReferenceTarget::Node));

    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    const CrackTip3DCard moved = cardAt<CrackTip3DTable>(model);
    REQUIRE((moved.nodes[0] == 1001 && moved.nodes[18] == 1019 && moved.nodes[27] == 1028 && moved.nodes[10] == 0));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CrackTip3DTable>(model).propId == 97511 && cardAt<CrackTipProp3DTable>(model).id == 97511));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CRAC3D refuses what the full and the half crack forms do not allow", "[model][cards][crack]")
{
    expectInvalid(crac3d(0, requiredOfFullCrack3d()), "id");
    expectInvalid(crackCard("CrackTip3D", 64, 1, 0, requiredOfFullCrack3d()), "propId");
    // The full crack needs G1 to G10 and G19 to G28.
    expectInvalid(crac3d(1, joined(run(2, 10), run(19, 28))), "node1");
    expectInvalid(crac3d(1, joined(run(1, 10), run(20, 28))), "node19");
    expectInvalid(crac3d(1, joined(run(1, 10), joined(run(19, 25), run(27, 28)))), "node26");
    // G37 to G46 are all given or all blank: a partial mid-plane is a fatal error of the format reference.
    expectInvalid(crac3d(1, joined(requiredOfFullCrack3d(), run(37, 45))), "node46");
    expectInvalid(crac3d(1, joined(requiredOfFullCrack3d(), {{40, 40}})), "node37");
    // The half crack needs G1 to G7 and G19 to G25, and has none of the points the format reference excludes.
    const GridPoints halfRequired = joined(run(1, 7), run(19, 25));
    expectInvalid(crac3d(1, joined(run(1, 7), run(19, 24))), "node25");
    expectInvalid(crac3d(1, joined(halfRequired, {{15, 15}})), "node15");
    expectInvalid(crac3d(1, joined(halfRequired, {{27, 27}})), "node27");
    expectInvalid(crac3d(1, joined(halfRequired, {{44, 44}})), "node44");
    expectInvalid(crac3d(1, joined(halfRequired, {{55, 55}})), "node55");
    expectInvalid(crac3d(1, joined(halfRequired, {{64, 64}})), "node64");
    expectInvalid(crac3d(1, joined(requiredOfFullCrack3d(), {{11, -1}})), "node11");
    // G8 to G10 are all given or all blank in the full crack too.
    expectInvalid(crac3d(1, joined(run(1, 9), run(19, 28))), "node10");
}

TEST_CASE("a PRAC2D has the defaults of the format reference and names a material", "[model][cards][crack]")
{
    Model model = readBulk(
        "CrackTipProp2D;96501;1;0.1\n"
        "CrackTipProp2D;96502;2;0.2;1;0.13;0.7;165.\n"
        "CrackTipProp2D;96503;1;0.3;;0.5\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "IsoMat;2;1.E+5;;0.3");
    const CrackTipProp2DCard minimal = cardAt<CrackTipProp2DTable>(model, 0);
    REQUIRE((minimal.id == 96501 && minimal.matId == 1 && minimal.thickness == 0.1));
    REQUIRE((minimal.planeCondition == 0 && minimal.nonStructMass == 0.0 && minimal.gammaCoefficient == 0.5 &&
             minimal.phiAngle == 180.0));
    const CrackTipProp2DCard full = cardAt<CrackTipProp2DTable>(model, 1);
    REQUIRE((full.matId == 2 && full.thickness == 0.2 && full.planeCondition == 1 && full.nonStructMass == 0.13));
    REQUIRE((full.gammaCoefficient == 0.7 && full.phiAngle == 165.0));
    // A blank IPLANE is 0, its format reference default.
    const CrackTipProp2DCard blankPlane = cardAt<CrackTipProp2DTable>(model, 2);
    REQUIRE((blankPlane.planeCondition == 0 && blankPlane.nonStructMass == 0.5));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CrackTipProp2DTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});

    REQUIRE_THROWS_AS(readBulk("CrackTipProp2D;7;1;0.1\nShellProp;7;1;0.1"), base::DeckError); // one property space
    REQUIRE_THROWS_AS(readBulk("CrackTipProp2D;7;1;0.1\nCrackTipProp2D;7;1;0.2"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<CrackTipProp2DTable>(model, 0).matId == 101 && cardAt<CrackTipProp2DTable>(model, 1).matId == 102));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<CrackTipProp2DTable>(model, 2).id == 97503);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("CrackTipProp2D;0;1;0.1", "id");
    expectInvalid("CrackTipProp2D;7;0;0.1", "matId");
    expectInvalid("CrackTipProp2D;7;;0.1", "matId");
    expectInvalid("CrackTipProp2D;7;1", "thickness"); // required
    expectInvalid("CrackTipProp2D;7;1;0.", "thickness");
    expectInvalid("CrackTipProp2D;7;1;-0.1", "thickness");
    expectInvalid("CrackTipProp2D;7;1;0.1;2", "planeCondition");
    expectInvalid("CrackTipProp2D;7;1;0.1;-1", "planeCondition");
    expectInvalid("CrackTipProp2D;7;1;0.1;0;-0.5", "nonStructMass");
    expectInvalid("CrackTipProp2D;7;1;0.1;0;0.;xx", "gammaCoefficient");
}

TEST_CASE("a PRAC3D has the defaults of the format reference and names a material", "[model][cards][crack]")
{
    Model model = readBulk(
        "CrackTipProp3D;96511;2\n"
        "CrackTipProp3D;96512;1;0.7;165.\n"
        "CrackTipProp3D;96513;1;;90.\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "IsoMat;2;1.E+5;;0.3");
    const CrackTipProp3DCard minimal = cardAt<CrackTipProp3DTable>(model, 0);
    REQUIRE(
        (minimal.id == 96511 && minimal.matId == 2 && minimal.gammaCoefficient == 0.5 && minimal.phiAngle == 180.0));
    const CrackTipProp3DCard full = cardAt<CrackTipProp3DTable>(model, 1);
    REQUIRE((full.matId == 1 && full.gammaCoefficient == 0.7 && full.phiAngle == 165.0));
    const CrackTipProp3DCard blankGamma = cardAt<CrackTipProp3DTable>(model, 2);
    REQUIRE((blankGamma.gammaCoefficient == 0.5 && blankGamma.phiAngle == 90.0));

    RecordingVisitor visitor;
    model.table<CrackTipProp3DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<CrackTipProp3DTable>(model, 0).matId == 102 && cardAt<CrackTipProp3DTable>(model, 1).matId == 101));
    requireRoundTrip(model);

    expectInvalid("CrackTipProp3D;0;1", "id");
    expectInvalid("CrackTipProp3D;7", "matId");
    expectInvalid("CrackTipProp3D;7;0", "matId");
    expectInvalid("CrackTipProp3D;7;1;xx", "gammaCoefficient");
}

TEST_CASE("the patch numbers of a CSEAM name properties or elements as its CTYPE says", "[model][cards][seam]")
{
    Model model = readBulk("IsoMat;1;2.1E+11;;0.3\nShellProp;37;1;0.1\nShellProp;52;1;0.1\nSeamProp;96521;1;;2.\n" +
                           gridCards(1, 4) +
                           "Node;21450\n"
                           "Node;63318\n"
                           "ShellQuad4;201;37;1;2;3;4\n"
                           "ShellQuad4;202;52;1;2;3;4\n"
                           "Seam;96121;96521;;;37;52;;;21450;;63318\n"
                           "Seam;96122;96521;;Elements;201;202;;;21450;;63318\n"
                           "Seam;96124;96521;;;37;52;;;;0.;;0.;0.;10.;0.;0.");
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor patches;
    model.table<SeamTable>().visitReferences(0, patches);
    REQUIRE(patches.fields == std::vector<std::string>{"propId", "patchAStart", "patchBStart", "nodes", "nodes"});
    REQUIRE(patches.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Property,
                                                            ReferenceTarget::Property, ReferenceTarget::Node,
                                                            ReferenceTarget::Node});
    RecordingVisitor elements;
    model.table<SeamTable>().visitReferences(1, elements);
    REQUIRE(elements.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Element,
                                                             ReferenceTarget::Element, ReferenceTarget::Node,
                                                             ReferenceTarget::Node});
    RecordingVisitor located;
    model.table<SeamTable>().visitReferences(2, located);
    REQUIRE(located.fields ==
            std::vector<std::string>{"propId", "patchAStart", "patchBStart"}); // no grid point: the coordinates

    // The reference is of the kind the CTYPE says: properties 201 and 202 do not satisfy the elements of an ELEM seam,
    // and elements 37 and 52 do not satisfy the PSHELL properties of the first format.
    const std::string common =
        "IsoMat;1;2.1E+11;;0.3\nShellProp;1;1;0.1\nSeamProp;96521;1;;2.\nNode;21450\nNode;63318\n";
    Model propertiesNamedByElements = readBulk(
        common + "ShellProp;201;1;0.1\nShellProp;202;1;0.1\nSeam;96122;96521;;Elements;201;202;;;21450;;63318");
    REQUIRE(unresolvedIn(propertiesNamedByElements) == 2);
    Model elementsNamedByProperties =
        readBulk(common + gridCards(1, 4) +
                 "ShellQuad4;37;1;1;2;3;4\nShellQuad4;52;1;1;2;3;4\nSeam;96121;96521;;;37;52;;;21450;;63318");
    REQUIRE(unresolvedIn(elementsNamedByProperties) == 2);
}

TEST_CASE("a renumbering of the properties or the elements follows the patches of a CSEAM by its CTYPE",
          "[model][cards][seam]")
{
    Model model = readBulk("IsoMat;1;2.1E+11;;0.3\nShellProp;37;1;0.1\nShellProp;52;1;0.1\nSeamProp;96521;1;;2.\n" +
                           gridCards(1, 4) +
                           "Node;21450\n"
                           "Node;63318\n"
                           "ShellQuad4;201;37;1;2;3;4\n"
                           "ShellQuad4;202;52;1;2;3;4\n"
                           "Seam;96121;96521;;;37;52;;;21450;;63318\n"
                           "Seam;96122;96521;;Elements;201;202;;;21450;;63318");
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    const SeamCard byProperty = cardAt<SeamTable>(model, 0);
    REQUIRE((byProperty.propId == 97521 && byProperty.patchAStart == 1037 && byProperty.patchBStart == 1052));
    const SeamCard byElement = cardAt<SeamTable>(model, 1);
    REQUIRE((byElement.propId == 97521 && byElement.patchAStart == 201 &&
             byElement.patchBStart == 202)); // elements are not properties
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<SeamTable>(model, 0).id == 97121);
    REQUIRE(cardAt<SeamTable>(model, 0).patchAStart == 1037); // a property is not an element
    REQUIRE(
        (cardAt<SeamTable>(model, 1).patchAStart == 1201 && cardAt<SeamTable>(model, 1).patchBStart == 1202));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<SeamTable>(model, 0).nodes == std::array<base::EntityId, 2>{22450, 64318});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("Seam;5;96521;;;37;52;;;1;;2\nCrackTip2D;5;1;1;2;3;4;5;6;7;8;9;10"),
                      base::DeckError);
}

TEST_CASE("a CSEAM refuses what its two formats and its CTYPE do not allow", "[model][cards][seam]")
{
    expectInvalid("Seam;0;96521;;;37;52;;;1;;2", "id");
    expectInvalid("Seam;1;0;;;37;52;;;1;;2", "propId");
    expectInvalid("Seam;1;2;;;;52;;;1;;2", "patchAStart");
    expectInvalid("Seam;1;2;;;37;52;-4;;1;;2", "patchAEnd");
    expectInvalid("Seam;1;2;;SOLID;37;52;;;1;;2", "patchKind");
    requireRoundTrip(readBulk("Seam;1;2;ABCDEFGHI;;37;52;;;1;;2"));
    // CTYPE ELEM: two different elements.
    expectInvalid("Seam;1;2;;Elements;201;;;;1;;2", "patchBStart");
    expectInvalid("Seam;1;2;;Elements;201;201;;;1;;2", "patchBStart");
    expectInvalid("Seam;1;2;;Elements;201;202;203;203;1;;2", "patchBEnd");
    // The grid points are both required in the first format.
    expectInvalid("Seam;1;2;;;37;52;;;1", "endNode");
    expectInvalid("Seam;1;2;;;37;52;;;;;2", "startNode");
    expectInvalid("Seam;1;2;;;37;52", "startNode"); // no second line
    // The alternate format has six reals at most, and a card with none is not that format.
    expectInvalid("Seam;1;2;;;37;52;;;;1.;;2.;3.;4.;xx;6.", "endY");
}

TEST_CASE("a SWLDPRM has its global parameters, then the sections of an element type", "[model][cards][swldprm]")
{
    Model model = readModelText(
        std::string(GLOBAL_PARAMETERS) + "\nConnectorSearchParams;;Fastener;4;;;GeometryCheckLevel;3\n", false);
    const ConnectorSearchParamsCard across = cardAt<ConnectorSearchParamsTable>(model, 0);
    REQUIRE(across.global.size() == 4);
    REQUIRE((across.global[0].name == "GeneratedDataPrint" && across.global[3].name == "MoveEndPointsToProjection" &&
             integerValue(across.global[3]) == 1));
    const ConnectorSearchParamsCard blanks = cardAt<ConnectorSearchParamsTable>(model, 1);
    REQUIRE((blanks.global.empty() && blanks.sections.size() == 1));
    REQUIRE((blanks.sections[0].element == ConnectorSearchParamsElement::Fastener &&
             blanks.sections[0].parameters.size() == 1));

    // The card may be written in lower case, separated by tabs.
    const Model tabs = readBulk("ConnectorSearchParams;1;GeneratedDataPrint;1");
    const ConnectorSearchParamsCard fromTabs = cardAt<ConnectorSearchParamsTable>(tabs);
    REQUIRE(fromTabs.global.size() == 1);
    REQUIRE((fromTabs.global[0].name == "GeneratedDataPrint" && integerValue(fromTabs.global[0]) == 1));

    // Keywords are matched without regard to the case, and a card with no parameter is allowed.
    const Model lower = readBulk("ConnectorSearchParams;;Weld;4;MaxEndPointMoves;2");
    REQUIRE((cardAt<ConnectorSearchParamsTable>(lower).sections.size() == 1 &&
             cardAt<ConnectorSearchParamsTable>(lower).sections[0].element == ConnectorSearchParamsElement::Weld));
    const Model empty = readBulk("ConnectorSearchParams");
    REQUIRE((cardAt<ConnectorSearchParamsTable>(empty).global.empty() &&
             cardAt<ConnectorSearchParamsTable>(empty).sections.empty()));
    requireRoundTrip(empty);
    requireRoundTrip(readBulk(GLOBAL_PARAMETERS));
}

TEST_CASE("a SWLDPRM keeps a name the table does not list, and checks the values of the names it lists",
          "[model][cards][swldprm]")
{
    const Model model = readBulk(
        "ConnectorSearchParams;6;SKIN;1;FUTURE;2.5;MaxShellNormalAngle;-1.;MaxShellNormalAngle;-1;ProjectionTolerance;-"
        "0.5;ProjectionToleranceSwitches;12");
    const ConnectorSearchParamsCard card = cardAt<ConnectorSearchParamsTable>(model);
    REQUIRE(card.global.size() == 6);
    REQUIRE((card.global[0].name == "SKIN" && integerValue(card.global[0]) == 1));
    REQUIRE((card.global[1].name == "FUTURE" && realValue(card.global[1]) == 2.5));
    REQUIRE((realValue(card.global[2]) == -1.0 && integerValue(card.global[3]) == -1)); // -1 skips the check
    REQUIRE(realValue(card.global[4]) == -0.5); // PROJTOL is a magnitude: a negative value is the format's option
    REQUIRE(integerValue(card.global[5]) == 12);
    REQUIRE_NOTHROW(
        readBulk("ConnectorSearchParams;6;ProjectionToleranceSwitches;2112;MaxEndPointMoves;-3;MaxProjectionDistance;0."
                 ";PatchStiffnessFactor;0.1;PatchDiameterFactor;10;DiagnosticTableUnit;7"));
    requireRoundTrip(model);

    expectInvalid("ConnectorSearchParams;1;GeneratedDataPrint;3", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;StopAfterConnectivityCheck;3", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;GeometryCheckLevel;4", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MaxDiameterHalvings;9", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;WeldMass;2", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MoveEndPointsToProjection;-1", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MaxShellNormalAngle;95.", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MaxShellNormalAngle;-2.", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;SeamEndMinEdgeAngle;45.", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;SeamEndMaxNormalAngle;100.", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;PatchDiameterFactor;0.5", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MaxProjectionDistance;-0.1", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;PatchStiffnessFactor;-1.", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;ProjectionTolerance;1.5", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;DiagnosticTableUnit;-1", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;ProjectionToleranceSwitches;3111",
                  "parameterValue"); // each digit is 0, 1 or 2
    expectInvalid("ConnectorSearchParams;1;ProjectionToleranceSwitches;22222", "parameterValue");
    expectInvalid("ConnectorSearchParams;1;MaxEndPointMoves;2.5", "parameterValue"); // an integer parameter
    expectInvalid("ConnectorSearchParams;;Weld;4;StopAfterConnectivityCheck;1",
                  "parameterName");                                                  // CHKRUN is global
    expectInvalid("ConnectorSearchParams;1;MaxShellNormalAngle", "parameterValue1"); // a name needs its value
    expectInvalid("ConnectorSearchParams;2;MaxShellNormalAngle;15.;GeneratedDataPrint", "parameterValue2");
    expectInvalid("ConnectorSearchParams;1;MaxShellNormalAngle;CWELD", "parameterValue1"); // a keyword is not a value
    expectInvalid("ConnectorSearchParams;1;MaxShellNormalAngle;xx", "parameterValue1");
    expectInvalid("ConnectorSearchParams;1;5;2", "parameterName"); // a name starts with a letter
    oor::testing::requireModelFileRoundTrip(readBulk("ConnectorSearchParams;1;ABCDEFGHI;1"));
}

TEST_CASE(
    "the catalogue of the SWLDPRM parameters has the table of the format reference, each default inside its own range",
    "[model][cards][swldprm]")
{
    const std::span<const ConnectorSearchParamsInfo> catalogue = connectorSearchParamsCatalogue();
    REQUIRE(catalogue.size() == 16);
    for (const ConnectorSearchParamsInfo& info : catalogue)
    {
        INFO(info.name);
        ConnectorSearchParamsValue value;
        if (info.kind == ConnectorSearchParamsKind::Integer)
            value.number = static_cast<std::int32_t>(info.defaultValue);
        else
            value.number = info.defaultValue;
        REQUIRE(connectorSearchParamsValueProblem(info, value).empty());
        REQUIRE(findConnectorSearchParam(info.name) == &info);
    }
    const ConnectorSearchParamsInfo* printSwitch = findConnectorSearchParam("GeneratedDataPrint");
    REQUIRE(printSwitch != nullptr);
    REQUIRE((printSwitch->kind == ConnectorSearchParamsKind::Integer && printSwitch->defaultValue == 0.0));
    REQUIRE(connectorSearchParamsRuleText(*printSwitch) == "one of 0, 1, 2, 11, 12, 100, 101, 111, 200, 201 or 211");
    REQUIRE(connectorSearchParamsRuleText(*findConnectorSearchParam("MaxShellNormalAngle")) ==
            "a real from 0 to 90, or -1");
    REQUIRE(findConnectorSearchParam("ProjectionTolerance")->defaultValue == 0.02);
    REQUIRE(findConnectorSearchParam("ProjectionToleranceSwitches")->defaultValue == 1111.0);
    REQUIRE(findConnectorSearchParam("StopAfterConnectivityCheck")->onlyGlobal);
    REQUIRE_FALSE(findConnectorSearchParam("GeometryCheckLevel")->onlyGlobal);
    REQUIRE(findConnectorSearchParam("SKIN") == nullptr);
}
