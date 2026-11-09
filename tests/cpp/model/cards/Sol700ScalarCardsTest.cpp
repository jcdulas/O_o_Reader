// Checks of the SOL 700 scalar elements and properties, the failure material and the moving coordinate system:
// CELAS1D, CELAS2D, CDAMP1D, CDAMP2D, PELAS1, PVISC1, PSHELL1, PCOMPA, COMPUDS and CORD3R. Per card:
// the fields in their format reference positions, the defaults, the references the card lists, the renumbering of the
// ID spaces it takes part in, the refused values and the round trip through a model file. The geometry of the four
// elements is in tests/cpp/analysis/Sol700ScalarGeometryTest.cpp.

#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Table.hpp"
#include "oor/model/cards/elements/damperDofPropertyExplicit/DamperDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/damperDofInlineExplicit/DamperDofInlineExplicitTable.hpp"
#include "oor/model/cards/elements/springDofPropertyExplicit/SpringDofPropertyExplicitTable.hpp"
#include "oor/model/cards/elements/springDofInlineExplicit/SpringDofInlineExplicitTable.hpp"
#include "oor/model/cards/materials/userCompositeFailureMat/UserCompositeFailureMatTable.hpp"
#include "oor/model/cards/properties/compPropExplicit/CompPropExplicitTable.hpp"
#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalTable.hpp"
#include "oor/model/cards/properties/springPropTable/SpringPropTableTable.hpp"
#include "oor/model/cards/properties/viscousDamperPropNonlinear/ViscousDamperPropNonlinearTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The record of a card from (slot number, text) pairs, the slots its card reader reads; `changes` replace some of
// them. Slots that are not named are blank.
std::string cardOf(const std::string& name, const std::vector<std::pair<int, std::string>>& given,
                   const std::vector<std::pair<int, std::string>>& changes = {})
{
    std::vector<std::string> fields;
    for (const auto* list : {&given, &changes})
    {
        for (const auto& [field, text] : *list)
        {
            if (fields.size() < static_cast<std::size_t>(field))
                fields.resize(static_cast<std::size_t>(field));
            fields[static_cast<std::size_t>(field - 1)] = text;
        }
    }
    return slotRecord(name, fields);
}

// What the elements of the decks name, so that every reference of the first deck resolves.
const std::string SUPPORT =
    "Node;57\n"
    "Node;8\n"
    "Node;9\n"
    "ScalarPoint;50\n"
    "RectFrame2;7;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
    "TableDynamic1;10;;;0.;0.;1.;100.;EndTable\n"
    "SpringPropTable;103;10\n"
    "SpringPropTable;1002;10\n"
    "DamperProp;102;0.5\n"
    "DamperProp;2001;0.5\n";

} // namespace

TEST_CASE("a CELAS1D has the fields of a CELAS1 and a continuation line with a coordinate system and FOLLOW",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(SUPPORT +
                           "SpringDofPropertyExplicit;1001;103;57;1;8;1;7;CoordinateFrame\n"
                           "SpringDofPropertyExplicit;1002;;57;1;50\n"
                           "SpringDofPropertyExplicit;1003;103;57;321;8;12\n"
                           "SpringDofPropertyExplicit;1004;103;9;;8;;;CoordinateFrame\n");
    const SpringDofPropertyExplicitCard full = cardAt<SpringDofPropertyExplicitTable>(model, 0);
    REQUIRE((full.id == 1001 && full.propId == 103 && full.node1 == 57 && full.components1 == 1 && full.node2 == 8 &&
             full.components2 == 1));
    REQUIRE((full.frameId == 7 && full.follow == ScalarFollow::Cord));
    // A blank PID is the EID, a blank end of the continuation line is "no coordinate system, nothing followed", and
    // a scalar point has no component.
    const SpringDofPropertyExplicitCard defaults = cardAt<SpringDofPropertyExplicitTable>(model, 1);
    REQUIRE((defaults.propId == 1002 && defaults.node2 == 50 && defaults.components2 == 0));
    REQUIRE((defaults.frameId == 0 && defaults.follow == ScalarFollow::Fixed));
    // A component is 0 or a set of the digits 1 to 6, kept in ascending order.
    const SpringDofPropertyExplicitCard sets = cardAt<SpringDofPropertyExplicitTable>(model, 2);
    REQUIRE((sets.components1 == 123 && sets.components2 == 12));
    // The word of FOLLOW is read in any case, and FOLLOW does not need a CORD (the card keeps what it is given).
    const SpringDofPropertyExplicitCard lower = cardAt<SpringDofPropertyExplicitTable>(model, 3);
    REQUIRE((lower.follow == ScalarFollow::Cord && lower.components1 == 0 && lower.frameId == 0));
    REQUIRE(unresolvedIn(model) == 0);

    // The references: the property (a PELAS or a PELAS1), the two points (grid or scalar) and the coordinate system.
    RecordingVisitor visitor;
    model.table<SpringDofPropertyExplicitTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1", "node2", "frameId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::ScalarOrGridPoint,
                                         ReferenceTarget::ScalarOrGridPoint, ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<SpringDofPropertyExplicitTable>().visitReferences(3, visitor); // a blank coordinate system names nothing
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1", "node2"});

    // The element space is shared by every element; the three other spaces the card names follow a renumbering.
    REQUIRE_THROWS_AS(readBulk("SpringDofPropertyExplicit;1;1;1;1;2;1\nSpringDofProperty;1;1;1;1;2;1"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<SpringDofPropertyExplicitTable>(model, 0).node1 == 1057 &&
             cardAt<SpringDofPropertyExplicitTable>(model, 0).node2 == 1008));
    REQUIRE(cardAt<SpringDofPropertyExplicitTable>(model, 1).node2 == 1050); // a scalar point is in the node space
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<SpringDofPropertyExplicitTable>(model, 0).propId == 1103 &&
             cardAt<SpringDofPropertyExplicitTable>(model, 1).propId == 2002));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE(cardAt<SpringDofPropertyExplicitTable>(model, 0).frameId == 107);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<SpringDofPropertyExplicitTable>(model, 0).id == 2001 &&
             cardAt<SpringDofPropertyExplicitTable>(model, 3).id == 2004));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CELAS2D has its stiffness and no S or GE, and the same continuation line", "[model][cards][sol700-scalar]")
{
    Model model =
        readBulk(SUPPORT +
                 "SpringDofInlineExplicit;1001;4700.;57;1;8;4;7;CoordinateFrame\nSpringDofInlineExplicit;1002;1.E+5;9;"
                 "3\nSpringDofInlineExplicit;1003;5.;50;;8\n");
    const SpringDofInlineExplicitCard full = cardAt<SpringDofInlineExplicitTable>(model, 0);
    REQUIRE((full.id == 1001 && full.stiffness == 4700.0 && full.node1 == 57 && full.components1 == 1 &&
             full.node2 == 8 && full.components2 == 4));
    REQUIRE((full.frameId == 7 && full.follow == ScalarFollow::Cord));
    const SpringDofInlineExplicitCard grounded = cardAt<SpringDofInlineExplicitTable>(model, 1);
    REQUIRE((grounded.node2 == 0 && grounded.components2 == 0 && grounded.frameId == 0 &&
             grounded.follow == ScalarFollow::Fixed));
    REQUIRE(cardAt<SpringDofInlineExplicitTable>(model, 2).node1 == 50);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<SpringDofInlineExplicitTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "frameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ScalarOrGridPoint,
                                                            ReferenceTarget::ScalarOrGridPoint,
                                                            ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<SpringDofInlineExplicitTable>().visitReferences(1, visitor); // one end at ground, no coordinate system
    REQUIRE(visitor.fields == std::vector<std::string>{"node1"});
    // The fields of a CELAS2 that the SOL 700 format does not have are refused.
    requireRoundTrip(model);
}

TEST_CASE("a CDAMP1D and a CDAMP2D are the CDAMP1 and CDAMP2 with the continuation line",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(SUPPORT +
                           "DamperDofPropertyExplicit;1001;102;57;1;8;1;7;CoordinateFrame\n"
                           "DamperDofPropertyExplicit;2001;;9;2\n"
                           "DamperDofInlineExplicit;3001;0.5;57;1;8;1;7\n"
                           "DamperDofInlineExplicit;3002;2.5;50;;9;3\n");
    const DamperDofPropertyExplicitCard property = cardAt<DamperDofPropertyExplicitTable>(model, 0);
    REQUIRE((property.id == 1001 && property.propId == 102 && property.node1 == 57 && property.components1 == 1));
    REQUIRE((property.node2 == 8 && property.components2 == 1 && property.frameId == 7 &&
             property.follow == ScalarFollow::Cord));
    const DamperDofPropertyExplicitCard defaults = cardAt<DamperDofPropertyExplicitTable>(model, 1);
    REQUIRE((defaults.propId == 2001 && defaults.node2 == 0 && defaults.frameId == 0));
    const DamperDofInlineExplicitCard own = cardAt<DamperDofInlineExplicitTable>(model, 0);
    REQUIRE((own.id == 3001 && own.damping == 0.5 && own.node1 == 57 && own.node2 == 8 && own.components2 == 1));
    REQUIRE((own.frameId == 7 && own.follow == ScalarFollow::Fixed)); // a CORD alone does not make the direction follow
    REQUIRE((cardAt<DamperDofInlineExplicitTable>(model, 1).damping == 2.5 &&
             cardAt<DamperDofInlineExplicitTable>(model, 1).node1 == 50));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<DamperDofPropertyExplicitTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"propId", "node1", "node2", "frameId"});
    REQUIRE(visitor.targets[0] == ReferenceTarget::Property);
    visitor = RecordingVisitor{};
    model.table<DamperDofInlineExplicitTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "frameId"});

    // All four SOL 700 scalar elements, and the other scalar elements, are in the one element space.
    REQUIRE_THROWS_AS(readBulk("DamperDofPropertyExplicit;1;1;1;1\nDamperDofInlineExplicit;1;1.;1;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringDofInlineExplicit;1;1.;1;1\nDamperDofInlineExplicit;1;1.;1;1"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(10));
    REQUIRE((cardAt<DamperDofPropertyExplicitTable>(model, 0).propId == 112 &&
             cardAt<DamperDofPropertyExplicitTable>(model, 1).propId == 2011));
    requireRoundTrip(model);
}

TEST_CASE("the values the four SOL 700 scalar elements refuse are reported by field", "[model][cards][sol700-scalar]")
{
    expectInvalid("SpringDofPropertyExplicit;0;1;1;1;2;1", "id");
    expectInvalid("SpringDofPropertyExplicit;1;0;1;1;2;1", "propId");
    expectInvalid("SpringDofPropertyExplicit;1;1;-1;1;2;1", "node1");
    expectInvalid("SpringDofPropertyExplicit;1;1;1;7;2;1", "components1");
    expectInvalid("SpringDofPropertyExplicit;1;1;1;1;2;9", "components2");
    expectInvalid("SpringDofPropertyExplicit;1;1;1;1;-2;1", "node2");
    expectInvalid("SpringDofPropertyExplicit;1;;1;1;1;1", "node2"); // the two connection points must be distinct
    expectInvalid("SpringDofPropertyExplicit;1;1;1;1;2;1;-3", "frameId");
    expectInvalid("SpringDofPropertyExplicit;1;1;1;1;2;1;;ROTATE", "follow");
    expectInvalid("SpringDofInlineExplicit;1;;1;1;2;1", "stiffness");
    expectInvalid("SpringDofInlineExplicit;1;1.;1;1;1;1", "node2");
    expectInvalid("SpringDofInlineExplicit;1;1.;1;1;2;1;;BOTH", "follow");
    expectInvalid("DamperDofPropertyExplicit;1;1;1;1;1;1", "node2");
    expectInvalid("DamperDofPropertyExplicit;1;1;1;0;2;1;-1", "frameId");
    expectInvalid("DamperDofInlineExplicit;1;;1;1;2;1", "damping");
    expectInvalid("DamperDofInlineExplicit;0;1.;1;1;2;1", "id");
}

TEST_CASE("a CELAS1D with no coordinate system is written as one line and FOLLOW makes the continuation line",
          "[model][cards][sol700-scalar]")
{
    const Model plain = readBulk("SpringDofPropertyExplicit;1;1;1;1;2;1");

    const Model following = readBulk("SpringDofPropertyExplicit;1;1;1;1;2;1;;CoordinateFrame");

    requireRoundTrip(plain);
    requireRoundTrip(following);
}

TEST_CASE("a PELAS1 is a spring property of the one property space and names the TABLED1 of its curve",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        "SpringPropTable;24;43\nSpringPropTable;25;44\nTableDynamic1;43;;;0.;0.;1.;100.;EndTable\nTableDynamic1;44;;;0."
        ";0.;"
        "1.;50.;EndTable");
    REQUIRE((cardAt<SpringPropTableTable>(model, 0).id == 24 && cardAt<SpringPropTableTable>(model, 0).tableId == 43));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<SpringPropTableTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});

    // All the PELAS and PELAS1 IDs are one space: it is the space of every property.
    REQUIRE_THROWS_AS(readBulk("SpringPropTable;5;10\nSpringProp;5;100."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringPropTable;5;10\nSpringPropTable;5;11"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringPropTable;5;10\nShellProp;5;1;0.1"), base::DeckError);

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(1000));
    REQUIRE((cardAt<SpringPropTableTable>(model, 0).tableId == 1043 &&
             cardAt<SpringPropTableTable>(model, 1).tableId == 1044));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<SpringPropTableTable>(model, 0).id == 124 && cardAt<SpringPropTableTable>(model, 1).id == 125));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("SpringPropTable;0;3", "id");
    expectInvalid("SpringPropTable;1", "tableId");
    expectInvalid("SpringPropTable;1;-2", "tableId");
}

TEST_CASE("a PVISC1 is one nonlinear damper with the TABLED1 of its force against the velocity",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        "ViscousDamperPropNonlinear;8;248\nViscousDamperPropNonlinear;9;249\nTableDynamic1;248;;;0.;0.;1.;10.;"
        "EndTable\nTableDynamic1;249;;;0.;0.;1.;20.;EndTable");
    REQUIRE((cardAt<ViscousDamperPropNonlinearTable>(model, 0).id == 8 &&
             cardAt<ViscousDamperPropNonlinearTable>(model, 0).tableId == 248));
    REQUIRE(rowCount<ViscousDamperPropNonlinearTable>(model) == 2); // one property to a card (the format shows one)
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<ViscousDamperPropNonlinearTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});

    REQUIRE_THROWS_AS(readBulk("ViscousDamperPropNonlinear;8;248\nViscousDamperProp;8;1.;2."),
                      base::DeckError); // the property space
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<ViscousDamperPropNonlinearTable>(model, 0).tableId == 348 &&
             cardAt<ViscousDamperPropNonlinearTable>(model, 1).tableId == 349));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<ViscousDamperPropNonlinearTable>(model, 1).id == 1009);
    requireRoundTrip(model);

    expectInvalid("ViscousDamperPropNonlinear;0;248", "id");
    expectInvalid("ViscousDamperPropNonlinear;8", "tableId");
    expectInvalid("ViscousDamperPropNonlinear;8;0", "tableId");
}

TEST_CASE("a PSHELL1 reads its words, keeps a blank FORM blank and resolves the NUMB of the formulation",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        "IsoMat;2;2.1E+11;;0.3\n"
        "IsoMat;3;2.1E+11;;0.3\n"
        "ShellPropNodal;7;2;BelytschkoLinTsay;Gauss;4;0.8;;;1.75;1.75;1.75;1.75\n"
        "ShellPropNodal;8;2\n"
        "ShellPropNodal;9;2;Membrane\n"
        "ShellPropNodal;10;2;Membrane;;4\n"
        "ShellPropNodal;11;3;C0Triangle\n"
        "ShellPropNodal;12;3;KeyHoff;LobattoPoints;2;1.;Top;Yes;0.1;0.2;0.3;0.4;ConstantApproximation;"
        "NoAvoidShearLocking\n");
    const ShellPropNodalCard example = cardAt<ShellPropNodalTable>(model, 0);
    REQUIRE((example.id == 7 && example.matId == 2 && example.formulation == ShellPropNodalForm::Blt));
    REQUIRE((example.quadrature == ShellPropNodalQuadrature::Gauss && example.integrationPointCount == 4 &&
             example.shearFactor == 0.8));
    REQUIRE((example.referenceSurface == ShellPropNodalReference::Middle &&
             example.spinCorrection == ShellPropNodalSpinCorrection::No));
    REQUIRE((example.thickness1 == 1.75 && example.thickness2 == 1.75 && example.thickness3 == 1.75 &&
             example.thickness4 == 1.75));
    REQUIRE((!example.transverseShear && !example.shearLocking));

    // The formulation of a blank FORM depends on the element (KEYHOFF for a CQUAD4, C0-TRIA for a CTRIA3): it stays
    // blank, and so do TRANSHR and SHRLCK, whose defaults depend on it. The other defaults are the ones of the format.
    const ShellPropNodalCard defaults = cardAt<ShellPropNodalTable>(model, 1);
    REQUIRE(!defaults.formulation);
    REQUIRE((defaults.quadrature == ShellPropNodalQuadrature::Gauss && defaults.integrationPointCount == 3 &&
             defaults.shearFactor == 0.83333));
    REQUIRE((defaults.referenceSurface == ShellPropNodalReference::Middle &&
             defaults.spinCorrection == ShellPropNodalSpinCorrection::No));
    REQUIRE((defaults.thickness1 == 0.0 && !defaults.transverseShear && !defaults.shearLocking));

    // A blank NUMB is 1 for a membrane and 3 for the others, and a value written by the deck is kept.
    REQUIRE((cardAt<ShellPropNodalTable>(model, 2).formulation == ShellPropNodalForm::Membrane &&
             cardAt<ShellPropNodalTable>(model, 2).integrationPointCount == 1));
    REQUIRE(cardAt<ShellPropNodalTable>(model, 3).integrationPointCount == 4);
    // The triangle is C0-TRIA or CO-TRIA (a letter O): both are read.
    REQUIRE(cardAt<ShellPropNodalTable>(model, 4).formulation == ShellPropNodalForm::C0Tria);

    const ShellPropNodalCard full = cardAt<ShellPropNodalTable>(model, 5);
    REQUIRE((full.formulation == ShellPropNodalForm::Keyhoff && full.quadrature == ShellPropNodalQuadrature::Lobatto &&
             full.integrationPointCount == 2));
    REQUIRE((full.shearFactor == 1.0 && full.referenceSurface == ShellPropNodalReference::Top &&
             full.spinCorrection == ShellPropNodalSpinCorrection::Yes));
    REQUIRE((full.thickness1 == 0.1 && full.thickness2 == 0.2 && full.thickness3 == 0.3 && full.thickness4 == 0.4));
    REQUIRE((full.transverseShear == ShellPropNodalTransverseShear::Approximate &&
             full.shearLocking == ShellPropNodalShearLock::NoAvoid));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<ShellPropNodalTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    REQUIRE_THROWS_AS(readBulk("ShellPropNodal;7;2\nShellProp;7;2;0.1"), base::DeckError); // the property space
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<ShellPropNodalTable>(model, 0).matId == 1002 && cardAt<ShellPropNodalTable>(model, 4).matId == 1003));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE((cardAt<ShellPropNodalTable>(model, 0).id == 107 && cardAt<ShellPropNodalTable>(model, 5).id == 112));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("the values a PSHELL1 refuses are reported by field", "[model][cards][sol700-scalar]")
{
    const std::vector<std::pair<int, std::string>> valid = {{1, "1"}, {2, "2"}, {3, "BelytschkoLinTsay"}};
    expectInvalid(cardOf("ShellPropNodal", valid, {{1, "0"}}), "id");
    expectInvalid(cardOf("ShellPropNodal", valid, {{2, ""}}), "matId");
    expectInvalid(cardOf("ShellPropNodal", valid, {{3, "FOO"}}), "formulation");
    expectInvalid(cardOf("ShellPropNodal", valid, {{4, "SIMPSON"}}), "quadrature");
    expectInvalid(cardOf("ShellPropNodal", valid, {{5, "6"}}),
                  "integrationPointCount"); // at most 5 points through the thickness
    expectInvalid(cardOf("ShellPropNodal", valid, {{5, "0"}}), "integrationPointCount");
    expectInvalid(cardOf("ShellPropNodal", valid, {{7, "MIDDLE"}}), "referenceSurface");
    expectInvalid(cardOf("ShellPropNodal", valid, {{8, "MAYBE"}}), "spinCorrection");
    expectInvalid(cardOf("ShellPropNodal", valid, {{13, "QUADRATIC"}}), "transverseShear");
    expectInvalid(cardOf("ShellPropNodal", valid, {{14, "SOMETIMES"}}), "shearLocking");
    expectInvalid(cardOf("ShellPropNodal", valid, {{9, "ABC"}}), "thickness1");
}

TEST_CASE("a PCOMPA extends the PCOMP of its number and keeps the defaults of its format",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        "IsoMat;1;2.1E+11;;0.3\n"
        "CompProp;10;;;;;;;;1;0.125;0.;Yes\n"
        "CompPropExplicit;10;BelytschkoWongChiang;;;;;;1;Yes\n"
        "CompProp;11;;;;;;;;1;0.125;0.;Yes\n"
        "CompPropExplicit;11\n"
        "CompProp;12;;;;;;;;1;0.125;0.;Yes\n"
        "CompPropExplicit;12;KeyHoff;0.5;Top;Elements;Yes;No;0;No\n");
    const CompPropExplicitCard example = cardAt<CompPropExplicitTable>(model, 0);
    REQUIRE((example.id == 10 && example.formulation == "BelytschkoWongChiang" && example.shearFactor == 0.83333));
    REQUIRE((example.referenceSurface == CompPropExplicitReference::Middle &&
             example.stressDefinition == CompPropExplicitStressDefinition::Fiber));
    REQUIRE(
        (example.skipFiberTimeStep == CompPropExplicitYesNo::No && example.strainOutput == CompPropExplicitYesNo::Yes));
    REQUIRE((example.laminationTheory == 1 && example.spinCorrection == CompPropExplicitYesNo::Yes));

    const CompPropExplicitCard defaults = cardAt<CompPropExplicitTable>(model, 1);
    REQUIRE((defaults.id == 11 && defaults.formulation.empty() && defaults.shearFactor == 0.83333 &&
             defaults.laminationTheory == 0));
    REQUIRE((defaults.skipFiberTimeStep == CompPropExplicitYesNo::No &&
             defaults.strainOutput == CompPropExplicitYesNo::Yes));
    REQUIRE(defaults.spinCorrection == CompPropExplicitYesNo::No);

    const CompPropExplicitCard full = cardAt<CompPropExplicitTable>(model, 2);
    REQUIRE((full.formulation == "KeyHoff" && full.shearFactor == 0.5 &&
             full.referenceSurface == CompPropExplicitReference::Top));
    REQUIRE((full.stressDefinition == CompPropExplicitStressDefinition::Element &&
             full.skipFiberTimeStep == CompPropExplicitYesNo::Yes));
    REQUIRE((full.strainOutput == CompPropExplicitYesNo::No && full.laminationTheory == 0 &&
             full.spinCorrection == CompPropExplicitYesNo::No));
    REQUIRE(unresolvedIn(model) == 0);

    // The ID space is the card type's own (one PCOMPA to a PCOMP) and the ID is a property reference.
    RecordingVisitor visitor;
    model.table<CompPropExplicitTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    REQUIRE_THROWS_AS(readBulk("CompPropExplicit;10\nCompPropExplicit;10"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CompPropExplicitTable>(model, 0).id == 1010 && cardAt<CompPropExplicitTable>(model, 2).id == 1012));
    REQUIRE(model.findTable<CompPropExplicitTable>()->idAtRow(0) == 1010); // the ID column follows
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("the values a PCOMPA refuses are reported by field", "[model][cards][sol700-scalar]")
{
    const std::vector<std::pair<int, std::string>> valid = {{1, "1"}, {2, "BelytschkoLinTsay"}};
    expectInvalid(cardOf("CompPropExplicit", valid, {{1, "0"}}), "id");
    oor::testing::requireModelFileRoundTrip(
        readModelText(cardOf("CompPropExplicit", valid, {{2, "TOOLONGFORM"}}), true));
    expectInvalid(cardOf("CompPropExplicit", valid, {{3, "-1."}}), "shearFactor");
    expectInvalid(cardOf("CompPropExplicit", valid, {{4, "MIDDLE"}}), "referenceSurface");
    expectInvalid(cardOf("CompPropExplicit", valid, {{5, "BOTH"}}), "stressDefinition");
    expectInvalid(cardOf("CompPropExplicit", valid, {{6, "MAYBE"}}), "skipFiberTimeStep");
    expectInvalid(cardOf("CompPropExplicit", valid, {{7, "MAYBE"}}), "strainOutput");
    expectInvalid(cardOf("CompPropExplicit", valid, {{8, "2"}}), "laminationTheory");
    expectInvalid(cardOf("CompPropExplicit", valid, {{8, "-1"}}), "laminationTheory");
    expectInvalid(cardOf("CompPropExplicit", valid, {{9, "SOMETIMES"}}), "spinCorrection");
}

namespace
{

// A COMPUDS with the least it needs: the elastic constants, the density and the four failure stresses.
const std::vector<std::pair<int, std::string>> USER_COMPOSITE_FAILURE_FIELDS = {
    {1, "1"},       {2, "LAYGRP1"}, {9, "18E6"}, {10, "1.2E6"}, {11, "0.31"}, {12, "0.7E6"},
    {15, "0.0582"}, {25, "215"},    {26, "165"}, {27, "55"},    {28, "120"}};

} // namespace

TEST_CASE("a COMPUDS reads the six lines of its format, resolves G1Z and G2Z and keeps the leading zeros",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        // The lines of the elastic constants, NV and S, and the four failure stresses.
        "UserCompositeFailureMat;1;LAYGRP1;;1.8E+7;1.2E+6;0.31;7.E+5;6.E+5;5.E+5;0.0582;5;74;;;;215;165;55;120\n"
        "UserCompositeFailureMat;2;GRP;;1.8E+7;1.2E+6;0.31;7.E+5;;;0.0582;;;;;;215;165;55;120\n" +
        cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS,
               {{1, "3"},     {3, "PLYUSR"},    {13, "2.5E6"}, {14, "2.6E6"},    {17, "9"},      {18, "10."},
                {19, "0.5"},  {20, "Elements"}, {21, "-0.1"},  {29, "Velocity"}, {30, "5.5"},    {31, "All"},
                {33, "TSAI"}, {34, "HILL"},     {35, "STRSS"}, {36, "CHANG"},    {37, "HASHIN"}, {41, "0101"},
                {42, "1111"}, {43, "0011"},     {44, "1001"},  {45, "0110"}}));
    const UserCompositeFailureMatCard example = cardAt<UserCompositeFailureMatTable>(model, 0);
    REQUIRE((example.id == 1 && example.groupName == "LAYGRP1" && example.userRoutineName.empty()));
    REQUIRE((example.youngsModulus1 == 18.0E6 && example.youngsModulus2 == 1.2E6 && example.poissonRatio12 == 0.31 &&
             example.shearModulus12 == 0.7E6));
    REQUIRE((example.shearModulus1Z == 0.6E6 && example.shearModulus2Z == 0.5E6 && example.density == 0.0582));
    REQUIRE((example.historyVariableCount == 5 && example.shearStrength == 74.0));
    REQUIRE((example.tensionStrength1 == 215.0 && example.compressionStrength1 == 165.0 &&
             example.tensionStrength2 == 55.0 && example.compressionStrength2 == 120.0));
    // The other fields have the defaults of the format.
    REQUIRE((example.nonlinearShearCoefficient == 0.0 &&
             example.transverseShear == UserCompositeFailureMatShearFailure::Sublayer &&
             example.interactionTerm12 == 0.0));
    REQUIRE(
        (example.degradation == UserCompositeFailureMatDegradation::Steps && example.degradationParameter == 100.0));
    REQUIRE(example.degradationStart == UserCompositeFailureMatDegradationStart::Individual);
    REQUIRE((example.fiberTensionTheory.empty() && example.fiberCompressionTheory.empty() &&
             example.matrixTensionTheory.empty() && example.matrixCompressionTheory.empty()));
    REQUIRE(example.matrixShearTheory.empty());
    REQUIRE((example.fiberTensionDegradation == "1111" && example.fiberCompressionDegradation == "1010" &&
             example.matrixTensionDegradation == "0110"));
    REQUIRE((example.matrixCompressionDegradation == "0110" && example.matrixShearDegradation == "0001"));

    // A blank G1Z or G2Z is the G12 (the format for G1Z, the MAT8 for G2Z), stored resolved.
    const UserCompositeFailureMatCard blank = cardAt<UserCompositeFailureMatTable>(model, 1);
    REQUIRE((blank.groupName == "GRP" && blank.shearModulus12 == 0.7E6 && blank.shearModulus1Z == 0.7E6 &&
             blank.shearModulus2Z == 0.7E6));

    const UserCompositeFailureMatCard full = cardAt<UserCompositeFailureMatTable>(model, 2);
    REQUIRE((full.id == 3 && full.userRoutineName == "PLYUSR" && full.shearModulus1Z == 2.5E6 &&
             full.shearModulus2Z == 2.6E6));
    REQUIRE((full.historyVariableCount == 9 && full.shearStrength == 10.0 && full.nonlinearShearCoefficient == 0.5 &&
             full.transverseShear == UserCompositeFailureMatShearFailure::Element));
    REQUIRE((full.interactionTerm12 == -0.1 && full.degradation == UserCompositeFailureMatDegradation::Velocity &&
             full.degradationParameter == 5.5));
    REQUIRE(full.degradationStart == UserCompositeFailureMatDegradationStart::All);
    REQUIRE((full.fiberTensionTheory == "TSAI" && full.fiberCompressionTheory == "HILL" &&
             full.matrixTensionTheory == "STRSS" && full.matrixCompressionTheory == "CHANG"));
    REQUIRE(full.matrixShearTheory == "HASHIN");
    REQUIRE((full.fiberTensionDegradation == "0101" && full.fiberCompressionDegradation == "1111" &&
             full.matrixTensionDegradation == "0011" && full.matrixCompressionDegradation == "1001"));
    REQUIRE(full.matrixShearDegradation == "0110");

    // A material of its own: the one material space, and no reference to another card.
    REQUIRE_THROWS_AS(readBulk("UserCompositeFailureMat;5;GRP;;1.8E+7;1.2E+6;0.31;7.E+5;;;0.0582;;;;;;215;165;55;"
                               "120\nIsoMat;5;2.1E+11;;0.3"),
                      base::DeckError);
    RecordingVisitor visitor;
    model.table<UserCompositeFailureMatTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<UserCompositeFailureMatTable>(model, 0).id == 1001 &&
             cardAt<UserCompositeFailureMatTable>(model, 2).id == 1003));
    REQUIRE(model.findTable<UserCompositeFailureMatTable>()->idAtRow(1) == 1002);
    requireRoundTrip(model);
}

TEST_CASE("the values a COMPUDS refuses are reported by field", "[model][cards][sol700-scalar]")
{
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{1, "0"}}), "id");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{2, ""}}), "groupName");
    oor::testing::requireModelFileRoundTrip(
        readModelText(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{2, "TOOLONGGROUP"}}), true));
    oor::testing::requireModelFileRoundTrip(
        readModelText(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{3, "TOOLONGNAME"}}), true));
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{9, ""}}), "youngsModulus1");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{9, "-1."}}), "youngsModulus1");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{10, "0."}}), "youngsModulus2");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{11, "0."}}), "poissonRatio12");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{12, "-2."}}), "shearModulus12");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{13, "-2."}}), "shearModulus1Z");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{14, "0."}}), "shearModulus2Z");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{15, ""}}), "density");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{17, "10"}}),
                  "historyVariableCount"); // 0 to 9 history variables
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{17, "-1"}}),
                  "historyVariableCount");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{18, "-1."}}), "shearStrength");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{19, "-1."}}),
                  "nonlinearShearCoefficient");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{20, "BOTH"}}), "transverseShear");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{25, ""}}), "tensionStrength1");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{26, "0."}}),
                  "compressionStrength1");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{27, "-1."}}), "tensionStrength2");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{28, "0."}}),
                  "compressionStrength2");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{29, "SLOWLY"}}), "degradation");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{30, "0."}}),
                  "degradationParameter");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{31, "SOME"}}), "degradationStart");
    oor::testing::requireModelFileRoundTrip(
        readModelText(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{35, "TOOLONGMODE"}}), true));
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{41, "111"}}),
                  "fiberTensionDegradation"); // four digits, each 0 or 1
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{42, "1012"}}),
                  "fiberCompressionDegradation");
    expectInvalid(cardOf("UserCompositeFailureMat", USER_COMPOSITE_FAILURE_FIELDS, {{45, "00001"}}),
                  "matrixShearDegradation");
}

TEST_CASE("a CORD3R defines one or two moving systems from three points and shares the coordinate system space",
          "[model][cards][sol700-scalar]")
{
    Model model = readBulk(
        "Node;1\n"
        "Node;2;;0.;0.;1.\n"
        "Node;3;;1.;0.;0.\n"
        "Node;4152\n"
        "Node;4310\n"
        "Node;4153\n"
        "Node;4311\n"
        "RectFrame3;1001;1;4152;4310\n"
        "RectFrame3;1002;1;2;3;1003;2;4153;4311\n");
    REQUIRE(rowCount<RectFrame3Table>(model) == 3);
    const RectFrame3Card example = cardAt<RectFrame3Table>(model, 0);
    REQUIRE((example.id == 1001 && example.nodes == std::array<base::EntityId, 3>{1, 4152, 4310}));
    const RectFrame3Card first = cardAt<RectFrame3Table>(model, 1);
    const RectFrame3Card second = cardAt<RectFrame3Table>(model, 2);
    REQUIRE((first.id == 1002 && first.nodes == std::array<base::EntityId, 3>{1, 2, 3}));
    REQUIRE((second.id == 1003 && second.nodes == std::array<base::EntityId, 3>{2, 4153, 4311}));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<RectFrame3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodes", "nodes", "nodes"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));

    // The coordinate system space is the one of every system.
    REQUIRE_THROWS_AS(readBulk("RectFrame3;5;1;2;3\nRectFrame1;5;1;2;3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RectFrame3;5;1;2;3\nRectFrame2;5;;0.;0.;0.;0.;0.;1.;1.;0.;0."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RectFrame3;5;1;2;3;5;4;5;6"), base::DeckError); // the two systems of a card

    const std::uint64_t before = model.table<RectFrame3Table>().version();
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<RectFrame3Table>(model, 0).nodes == std::array<base::EntityId, 3>{1001, 5152, 5310});
    REQUIRE(model.table<RectFrame3Table>().version() > before); // a cache of positions sees the change
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE((cardAt<RectFrame3Table>(model, 0).id == 1101 && cardAt<RectFrame3Table>(model, 2).id == 1103));
    requireRoundTrip(model);

    expectInvalid("RectFrame3;0;1;2;3", "id");
    expectInvalid("RectFrame3;1;0;2;3", "node1");
    expectInvalid("RectFrame3;1;1;1;3", "node2"); // the three points must be different (the format: must be unique)
    expectInvalid("RectFrame3;1;1;2;1", "node3");
    expectInvalid("RectFrame3;1;1;2;2", "node3");
    expectInvalid("RectFrame3;1;1;2", "node3A");
    expectInvalid("RectFrame3;1;1;2;3;4", "node1B");      // a second system needs all its fields
    expectInvalid("RectFrame3;1;1;2;3;4;5;5;6", "node2"); // and follows the same rules
    expectInvalid("RectFrame3;1;1;2;3;0;4;5;6", "id");
}
