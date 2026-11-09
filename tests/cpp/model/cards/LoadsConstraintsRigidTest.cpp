// Checks of the loads, the constraints and the rigid elements: the defaults, the variants, the
// references (including the set IDs of LOAD and SPCADD) and the layout of the cards with variable lists.

#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

} // namespace

TEST_CASE("MOMENT and GRAV read their fields and default the system to the basic one", "[model][cards]")
{
    const Model model =
        readBulk("PointMomentLoad;2;5;;3.7;0.;1.;0.\nGravityLoad;3;;32.17;0.;0.;-1.\nGravityLoad;4;7;9.81;1.;0.;0.;-1");
    const PointMomentLoadCard moment = cardAt<PointMomentLoadTable>(model);
    REQUIRE(moment.id == 2);
    REQUIRE(moment.node == 5);
    REQUIRE(moment.frameId == 0);
    REQUIRE(moment.magnitude == 3.7);
    REQUIRE(moment.direction == std::array<double, 3>{0.0, 1.0, 0.0});
    const GravityLoadCard first = cardAt<GravityLoadTable>(model, 0);
    REQUIRE(first.frameId == 0);
    REQUIRE(first.magnitude == 32.17);
    REQUIRE(first.mainBulkFlag == 0);
    const GravityLoadCard second = cardAt<GravityLoadTable>(model, 1);
    REQUIRE(second.frameId == 7);
    REQUIRE(second.mainBulkFlag == -1);
    REQUIRE_THROWS_AS(readBulk("GravityLoad;1;;9.81"), base::DeckError); // a zero vector
}

TEST_CASE("PLOAD4 resolves P2 to P4 to P1 for a surface load and keeps the blanks of a line load", "[model][cards]")
{
    const Model model =
        readBulk("FacePressureLoad;1;10;5.\nFacePressureLoad;2;10;5.;;3.\nFacePressureLoad;3;10;5.;;3.;;;;;;;;Line");
    REQUIRE(cardAt<FacePressureLoadTable>(model, 0).pressures == std::array<double, 4>{5.0, 5.0, 5.0, 5.0});
    REQUIRE(cardAt<FacePressureLoadTable>(model, 1).pressures == std::array<double, 4>{5.0, 5.0, 3.0, 5.0});
    const FacePressureLoadCard line = cardAt<FacePressureLoadTable>(model, 2);
    REQUIRE(line.kind == PressureKind::Line);
    REQUIRE(line.pressures[0] == 5.0);
    REQUIRE(std::isnan(line.pressures[1]));
    REQUIRE(line.pressures[2] == 3.0);
    REQUIRE(std::isnan(line.pressures[3]));
    // The blanks of the line load stay blank when the card is written.

    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("PLOAD4 reads the solid face, the THRU form and the direction", "[model][cards]")
{
    const Model model = readBulk(
        "FacePressureLoad;1;2215;12.;9.;6.;;37;8;2;0.;1.;0.\n"
        "FacePressureLoad;2;2215;12.;;;;Through;2242\n"
        "FacePressureLoad;3;1;1.;;;;;;;;;;;Tangent");
    const FacePressureLoadCard solid = cardAt<FacePressureLoadTable>(model, 0);
    REQUIRE(solid.node1 == 37);
    REQUIRE(solid.node3Or4 == 8);
    REQUIRE(solid.frameId == 2);
    REQUIRE(solid.direction == std::array<double, 3>{0.0, 1.0, 0.0});
    REQUIRE_FALSE(solid.isRange());
    const FacePressureLoadCard range = cardAt<FacePressureLoadTable>(model, 1);
    REQUIRE(range.isRange());
    REQUIRE(range.elemId == 2215);
    REQUIRE(range.elemId2 == 2242);
    REQUIRE(range.node1 == 0);
    REQUIRE(cardAt<FacePressureLoadTable>(model, 2).lineDirection == LineLoadDirection::Tangent);
    REQUIRE(cardAt<FacePressureLoadTable>(model, 2).kind == PressureKind::Surface);
}

TEST_CASE("PLOAD4 lists the elements, the grid points and the system as references", "[model][cards]")
{
    Model model = readBulk("FacePressureLoad;1;2215;12.;;;;37;8;2\nFacePressureLoad;2;2215;12.;;;;Through;2242");
    RecordingVisitor solid;
    model.table<FacePressureLoadTable>().visitReferences(0, solid);
    REQUIRE(solid.fields == std::vector<std::string>{"elemId", "node1", "node3", "frameId"});
    REQUIRE(solid.targets == std::vector<ReferenceTarget>{ReferenceTarget::Element, ReferenceTarget::Node,
                                                          ReferenceTarget::Node, ReferenceTarget::Coordinate});
    RecordingVisitor range;
    model.table<FacePressureLoadTable>().visitReferences(1, range);
    REQUIRE(range.fields == std::vector<std::string>{"elemId", "elemId2"});
}

TEST_CASE("LOAD reads its scale factors and load sets over continuation lines", "[model][cards]")
{
    const Model model = readBulk("LoadCombination;101;-0.5;1.;3;4.5;4;3.;5;4.;6;5.;7");
    const LoadCombinationCard card = cardAt<LoadCombinationTable>(model);
    REQUIRE(card.scale == -0.5);
    REQUIRE(card.terms.size() == 5);
    REQUIRE(card.terms[1] == LoadCombinationTerm{4.5, 4});
    REQUIRE(card.terms[4] == LoadCombinationTerm{5.0, 7});
    REQUIRE_THROWS_AS(readBulk("LoadCombination;1;1.;1.;1"), base::DeckError); // a LOAD refers to itself
    REQUIRE_THROWS_AS(readBulk("LoadCombination;1;1.;1."), base::DeckError);   // a scale factor without a load set
}

TEST_CASE("the load sets of a LOAD and the constraint sets of an SPCADD are resolved by category", "[model][cards]")
{
    // Every reference resolves: load sets 10 and 11 (a LOAD may use another LOAD), constraint sets 5, 6 and 8.
    Model model = readBulk(
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "PointForceLoad;10;1;;1.;1.;0.;0.\n"
        "PointForceLoad;10;2;;1.;1.;0.;0.\n"
        "GravityLoad;11;;9.8;0.;0.;-1.\n"
        "SinglePointConstrList;5;123;1\n"
        "SinglePointConstr;6;2;1\n"
        "MultiPointConstr;8;1;1;1.;2;1;-1.\n"
        "LoadCombination;20;1.;1.;10;2.;11\n"
        "LoadCombination;21;1.;1.;20\n"
        "SinglePointConstrUnion;30;5;6;8\n");
    REQUIRE(unresolvedIn(model) == 0);

    // A set that no card has, and a set of the other category, do not resolve.
    Model wrong = readBulk(
        "Node;1;;0.;0.;0.\n"
        "PointForceLoad;10;1;;1.;1.;0.;0.\n"
        "SinglePointConstrList;5;123;1\n"
        "LoadCombination;20;1.;1.;10;2.;12;3.;5\n"
        "SinglePointConstrUnion;30;5;7;10");
    REQUIRE(unresolvedIn(wrong) == 4); // L 12, L 5 (a constraint set), S 7, S 10 (a load set)
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    (void)ReferenceChecker::check(wrong, sink);
    REQUIRE(sink.hasErrors());

    REQUIRE(categoryOfSet(ReferenceTarget::LoadCombinationSet) == CardCategory::Load);
    REQUIRE(categoryOfSet(ReferenceTarget::ConstraintSet) == CardCategory::Constraint);
    REQUIRE_FALSE(categoryOfSet(ReferenceTarget::Node).has_value());
    REQUIRE(cardsOfSet(model, CardCategory::Load, 10).size() == 2);
    REQUIRE(cardsOfSet(model, CardCategory::Load, 99).empty());
    REQUIRE(setIdsOfCategory(model, CardCategory::Load) == std::unordered_set<base::EntityId>{10, 11, 20, 21});
    REQUIRE(setIdsOfCategory(model, CardCategory::Constraint) == std::unordered_set<base::EntityId>{5, 6, 8, 30});
    REQUIRE(resolveReference(model, ReferenceTarget::LoadCombinationSet, 10).has_value());
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::LoadCombinationSet, 5).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::ConstraintSet, 5).has_value());
}

TEST_CASE("a removed card no longer makes its set exist", "[model][cards]")
{
    Model model = readBulk("PointForceLoad;10;1;;1.;1.;0.;0.\nLoadCombination;20;1.;1.;10");
    REQUIRE(unresolvedIn(model) == 1); // only the missing GRID 1 of the FORCE
    const CardLocation force = *cardsOfSet(model, CardCategory::Load, 10).begin();
    model.remove(force);
    REQUIRE(unresolvedIn(model) == 1); // the LOAD now has no set 10; GRID 1 is no longer referenced
}

TEST_CASE("LOAD and SPCADD list their sets as references", "[model][cards]")
{
    Model model = readBulk("LoadCombination;20;1.;1.;10;2.;11\nSinglePointConstrUnion;30;5;6");
    RecordingVisitor load;
    model.table<LoadCombinationTable>().visitReferences(0, load);
    REQUIRE(load.fields == std::vector<std::string>{"loadSetId1", "loadSetId1"});
    REQUIRE(load.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::LoadCombinationSet, ReferenceTarget::LoadCombinationSet});
    RecordingVisitor spcadd;
    model.table<SinglePointConstrUnionTable>().visitReferences(0, spcadd);
    REQUIRE(spcadd.fields == std::vector<std::string>{"memberSetId", "memberSetId"});
    REQUIRE(spcadd.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ConstraintSet, ReferenceTarget::ConstraintSet});
}

TEST_CASE("TEMP reads grid point and temperature pairs, over continuation lines too", "[model][cards]")
{
    const Model model =
        readBulk("NodeTemperatureLoad;3;86;305.4;57;233.6\nNodeTemperatureLoad;4;1;10.;2;20.;3;30.;4;40.;5;50.");
    const NodeTemperatureLoadCard small = cardAt<NodeTemperatureLoadTable>(model, 0);
    REQUIRE(small.points.size() == 2);
    REQUIRE(small.points[1] == NodeTemperature{57, 233.6});
    const NodeTemperatureLoadCard large = cardAt<NodeTemperatureLoadTable>(model, 1);
    REQUIRE(large.points.size() == 5);
    REQUIRE(large.points[3] == NodeTemperature{4, 40.0});
    REQUIRE_THROWS_AS(readBulk("NodeTemperatureLoad;3;86"), base::DeckError); // a point without a temperature
}

TEST_CASE("SPC has one or two groups and the second one needs its grid point", "[model][cards]")
{
    const Model model =
        readBulk("SinglePointConstr;1;41;3;-1.8\nSinglePointConstr;2;41;321;;5;6;0.5\nSinglePointConstr;3;7");
    const SinglePointConstrCard one = cardAt<SinglePointConstrTable>(model, 0);
    REQUIRE_FALSE(one.hasSecondGroup());
    REQUIRE(one.components[0] == 3);
    REQUIRE(one.enforced[0] == -1.8);
    const SinglePointConstrCard two = cardAt<SinglePointConstrTable>(model, 1);
    REQUIRE(two.hasSecondGroup());
    REQUIRE(two.components == std::array<std::int32_t, 2>{123, 6});
    REQUIRE(two.enforced == std::array<double, 2>{0.0, 0.5});
    REQUIRE(cardAt<SinglePointConstrTable>(model, 2).components[0] ==
            0); // a scalar point or a grid point with nothing: C blank
    REQUIRE_THROWS_AS(readBulk("SinglePointConstr;1;1;1;;;6"), base::DeckError);
}

TEST_CASE("SPCADD keeps its sets in the order they are written", "[model][cards]")
{
    const Model model = readBulk("SinglePointConstrUnion;101;4;6;2;11;;;;9;5");
    REQUIRE(cardAt<SinglePointConstrUnionTable>(model).sets == std::vector<base::EntityId>{4, 6, 2, 11, 9, 5});
}

TEST_CASE("MPC puts two terms on each line, from the second field", "[model][cards]")
{
    Model model = readBulk("MultiPointConstr;3;36;3;5.4;2;;3.75;1;4;-2.18;5;6;;7;1");
    const MultiPointConstrCard card = cardAt<MultiPointConstrTable>(model);
    REQUIRE(card.terms.size() == 5);
    REQUIRE(card.terms[0] == MultiPointConstrTerm{36, 3, 5.4});
    REQUIRE(card.terms[1] == MultiPointConstrTerm{2, 0, 3.75});
    REQUIRE(card.terms[2] == MultiPointConstrTerm{1, 4, -2.18});
    REQUIRE(card.terms[3] == MultiPointConstrTerm{5, 6, 0.0}); // Aj defaults to 0.0 after the first term
    REQUIRE(card.terms[4] == MultiPointConstrTerm{7, 1, 0.0});
    RecordingVisitor visitor;
    model.table<MultiPointConstrTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.size() == 5);
}

TEST_CASE("RBE2 reads ALPHA and TREF after the last grid point", "[model][cards]")
{
    const Model model = readBulk(
        "RigidBodyStar;1;5;321;10;11\nRigidBodyStar;2;5;123;10;6.5E-6\nRigidBodyStar;3;5;123;10;11;6.5E-6;20."
        "\nRigidBodyStar;4;5;123;10;;20.");
    const RigidBodyStarCard plain = cardAt<RigidBodyStarTable>(model, 0);
    REQUIRE(plain.independent == 5);
    REQUIRE(plain.components == 123); // the digits are kept in order
    REQUIRE(plain.dependents == std::vector<base::EntityId>{10, 11});
    REQUIRE(plain.alphaCoefficient == 0.0);
    const RigidBodyStarCard alpha = cardAt<RigidBodyStarTable>(model, 1);
    REQUIRE(alpha.dependents == std::vector<base::EntityId>{10});
    REQUIRE(alpha.alphaCoefficient == 6.5e-6);
    REQUIRE(alpha.referenceTemperature == 0.0);
    const RigidBodyStarCard both = cardAt<RigidBodyStarTable>(model, 2);
    REQUIRE(both.dependents == std::vector<base::EntityId>{10, 11});
    REQUIRE(both.alphaCoefficient == 6.5e-6);
    REQUIRE(both.referenceTemperature == 20.0);
    const RigidBodyStarCard trefOnly = cardAt<RigidBodyStarTable>(model, 3);
    REQUIRE(trefOnly.dependents == std::vector<base::EntityId>{10});
    REQUIRE(trefOnly.alphaCoefficient == 0.0);
    REQUIRE(trefOnly.referenceTemperature == 20.0);
}

TEST_CASE("a rigid element shares the element ID space and lists its grid points as references", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("ShellQuad4;1;1;1;2;3;4\nRigidBodyStar;1;5;123;10"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("RigidBodyStar;7;5;123;10\nRigidInterpolation;7;5;123;1;1.;123;1;1"), base::DeckError);
    Model model =
        readBulk("RigidBodyStar;1;5;123;10;11\nRigidInterpolation;2;5;123;2;1.;123;2;1;2;3.;1;1;4;DependentDofs;1;6;1");
    RecordingVisitor star;
    model.table<RigidBodyStarTable>().visitReferences(0, star);
    REQUIRE(star.fields == std::vector<std::string>{"independentNode", "dependentNode", "dependentNode"});
    RecordingVisitor interpolation;
    model.table<RigidInterpolationTable>().visitReferences(0, interpolation);
    REQUIRE(interpolation.fields == std::vector<std::string>{"referenceNode", "node", "node", "node", "dependentNode"});
    REQUIRE(unresolvedIn(model) == 8); // none of the 8 grid point references (5, 10, 11, 5, 1, 2, 4, 6) exists
}

TEST_CASE("RBE3 reads the weighted groups, the UM list and ALPHA as the format reference lays them out",
          "[model][cards]")
{
    const Model model = readBulk(
        "RigidInterpolation;14;200;1234;4;1.;123;3;1;3;5;3.8;1;3;2;4;6;6.1;2;3;7;8;9;4.4;1;2;15;16;DependentDofs;3;200;"
        "14;5;3;7;2;"
        "ThermalExpansion;1.2E-5");
    const RigidInterpolationCard card = cardAt<RigidInterpolationTable>(model);
    REQUIRE(card.referenceNode == 200);
    REQUIRE(card.refComponents == 1234);
    REQUIRE(card.groups.size() == 4);
    REQUIRE(card.groups[0] == RigidInterpolationGroup{1.0, 123, {1, 3, 5}});
    REQUIRE(card.groups[1] == RigidInterpolationGroup{3.8, 1, {2, 4, 6}});
    REQUIRE(card.groups[2] == RigidInterpolationGroup{6.1, 2, {7, 8, 9}});
    REQUIRE(card.groups[3] == RigidInterpolationGroup{4.4, 1, {15, 16}});
    REQUIRE(card.dependents.size() == 3);
    REQUIRE(card.dependents[0] == RigidInterpolationDependent{200, 14});
    REQUIRE(card.alphaCoefficient == 1.2e-5);
    REQUIRE(card.referenceTemperature == 0.0);
}

TEST_CASE("RBE3 writes UM and ALPHA at the start of a line and reads its own text back", "[model][cards]")
{
    const Model model = readBulk(
        "RigidInterpolation;14;200;1234;4;1.;123;3;1;3;5;3.8;1;3;2;4;6;6.1;2;3;7;8;9;4.4;1;2;15;16;DependentDofs;3;200;"
        "14;5;3;7;2;"
        "ThermalExpansion;1.2E-5;20.");

    oor::testing::requireModelFileRoundTrip(model);
    const Model again = readModelText(writeModelText(model), true);
    const RigidInterpolationCard original = cardAt<RigidInterpolationTable>(model);
    const RigidInterpolationCard copy = cardAt<RigidInterpolationTable>(again);
    REQUIRE(copy.groups == original.groups);
    REQUIRE(copy.dependents == original.dependents);
    REQUIRE(copy.alphaCoefficient == original.alphaCoefficient);
    REQUIRE(copy.referenceTemperature == original.referenceTemperature);
}

TEST_CASE("RBE3 reads its key words and ThermalExpansion cannot come before DependentDofs", "[model][cards]")
{
    const Model model =
        readBulk("RigidInterpolation;1;5;123;1;1.;123;2;1;2;DependentDofs;1;3;1;ThermalExpansion;1.E-5");
    const RigidInterpolationCard card = cardAt<RigidInterpolationTable>(model);
    REQUIRE(card.dependents == std::vector<RigidInterpolationDependent>{RigidInterpolationDependent{3, 1}});
    REQUIRE(card.alphaCoefficient == 1.0e-5);
    REQUIRE_THROWS_AS(readBulk("RigidInterpolation;1;5;123;1;1.;123;2;1;2;ThermalExpansion;1.E-5;DependentDofs;1;3;1"),
                      base::DeckError);
}
