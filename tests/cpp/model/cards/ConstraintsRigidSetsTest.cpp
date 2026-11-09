// Checks of the constraints, rigid elements and sets: SPCD, SPCOFF, SPCOFF1, SPCAX, MPCADD, MPCAX, SUPORT,
// SUPORT1, GMSPC, RBAR, RBAR1, RBE1, RJOINT, RROD, RSPLINE, RSSCON, RTRPLT, RTRPLT1, SEQGP, ASET, BSET, CSET, OMIT,
// QSET, USET and their "1" forms, BNDGRID, SET1 and SET3: field positions, defaults, THRU lists, variants, references,
// renumbering and rules.

#include "oor/model/cards/constraints/autoConstrExclusion/AutoConstrExclusionTable.hpp"
#include "oor/model/cards/constraints/autoConstrExclusionList/AutoConstrExclusionListTable.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/freeBodySupport/FreeBodySupportTable.hpp"
#include "oor/model/cards/constraints/freeBodySupportSet/FreeBodySupportSetTable.hpp"
#include "oor/model/cards/constraints/geomSinglePointConstr/GeomSinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrAxisym/MultiPointConstrAxisymTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrAxisym/SinglePointConstrAxisymTable.hpp"
#include "oor/model/cards/nodes/nodeSequence/NodeSequenceTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar1/RigidBar1Table.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"
#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointTable.hpp"
#include "oor/model/cards/rigid_elements/rigidRod/RigidRodTable.hpp"
#include "oor/model/cards/rigid_elements/rigidShellSolidConnection/RigidShellSolidConnectionTable.hpp"
#include "oor/model/cards/rigid_elements/rigidSpline/RigidSplineTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Table.hpp"
#include "oor/model/cards/sets/analysisDofSet/AnalysisDofSetTable.hpp"
#include "oor/model/cards/sets/analysisDofSet1/AnalysisDofSet1Table.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet/FixedBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/fixedBoundaryDofSet1/FixedBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet/FreeBoundaryDofSetTable.hpp"
#include "oor/model/cards/sets/freeBoundaryDofSet1/FreeBoundaryDofSet1Table.hpp"
#include "oor/model/cards/sets/generalizedDofSet/GeneralizedDofSetTable.hpp"
#include "oor/model/cards/sets/generalizedDofSet1/GeneralizedDofSet1Table.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/cards/sets/omittedDofSet/OmittedDofSetTable.hpp"
#include "oor/model/cards/sets/omittedDofSet1/OmittedDofSet1Table.hpp"
#include "oor/model/cards/sets/shapeBoundaryNodeSet/ShapeBoundaryNodeSetTable.hpp"
#include "oor/model/cards/sets/typedListSet3/TypedListSet3Table.hpp"
#include "oor/model/cards/sets/userDofSet/UserDofSetTable.hpp"
#include "oor/model/cards/sets/userDofSet1/UserDofSet1Table.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
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

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when given).
void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
}

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

// Nodes 1 to 40 so that the cards below have something to refer to.
std::string grids()
{
    std::string text;
    for (int id = 1; id <= 40; ++id)
        text += "Node;" + std::to_string(id) + ";;0.;0.;" + std::to_string(id) + ".\n";
    return text;
}

// Records the references a card lists.
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

using Pairs = std::vector<DofPair>;

} // namespace

// ------------------------------------------------------------------------------------------------ constraints

TEST_CASE("SPCD is a load set card: its two groups, its motion and the sets that select it", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "EnforcedMotionConstr;140;27;3;-1.8;8;;3.4\nEnforcedMotionConstr;101;"
                           "1\nEnforcedMotionConstr;102;1;123;0.5;2;456;-0.5");
    REQUIRE(EnforcedMotionConstrTable::INFO.category == CardCategory::Load);
    REQUIRE(EnforcedMotionConstrTable::INFO.idSpace == base::IdSpace::Group);
    EnforcedMotionConstrCard card = cardAt<EnforcedMotionConstrTable>(model, 0);
    REQUIRE((card.id == 140 && card.nodes[0] == 27 && card.components[0] == 3 && card.enforced[0] == Approx(-1.8)));
    REQUIRE((card.nodes[1] == 8 && card.components[1] == 0 && card.enforced[1] == Approx(3.4)));
    card = cardAt<EnforcedMotionConstrTable>(model, 1);
    REQUIRE_FALSE(card.hasSecondGroup());
    REQUIRE((card.components[0] == 0 && card.enforced[0] == 0.0));
    REQUIRE(cardAt<EnforcedMotionConstrTable>(model, 2).components[1] == 456);
    // The set is a load set: LOAD and the EXCITEID of a dynamic load find it (as a SPC command does not).
    REQUIRE(resolveReference(model, ReferenceTarget::LoadCombinationSet, 140).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::ExcitationSet, 101).has_value());
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::ConstraintSet, 140).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<EnforcedMotionConstrTable>(model, 0).nodes[0] == 127 &&
             cardAt<EnforcedMotionConstrTable>(model, 0).nodes[1] == 108));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    REQUIRE(cardAt<EnforcedMotionConstrTable>(model, 0).id == 1140);
    requireRoundTrip(model);
    Model lacking = readBulk("EnforcedMotionConstr;140;27;3;-1.8;8;;3.4");
    REQUIRE(unresolvedIn(lacking) == 2);

    expectInvalid("EnforcedMotionConstr;0;1", "id");
    expectInvalid("EnforcedMotionConstr;1;0", "node1");
    expectInvalid("EnforcedMotionConstr;1;1;7", "components1");
    expectInvalid("EnforcedMotionConstr;1;1;;;;3", "node2");
    expectInvalid("EnforcedMotionConstr;1;1;1;1.;2;9", "components2");
}

TEST_CASE("the load command and the dynamic loads select an SPCD", "[model][cards]")
{
    Model model = readModelText(
        "[case]\nSubcaseStart;1\nStaticLoadSelection;140\n[model]\nNode;27;;0.;0.;0.\nEnforcedMotionConstr;140;27;3;1."
        "\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readModelText(
        "[case]\nSubcaseStart;1\nStaticLoadSelection;101\n[model]\nNode;27;;0.;0.;0.\nEnforcedMotionConstr;140;27;3;1."
        "\n",
        true);
    REQUIRE(unresolvedIn(lacking) == 1);
}

TEST_CASE("SPCAX and MPCAX name rings, harmonics and components", "[model][cards]")
{
    Model model = readBulk(
        "SinglePointConstrAxisym;4;7;3;24;4.5\n"
        "SinglePointConstrAxisym;3;10;0;1;0.\n"
        "MultiPointConstrAxisym;36;19;5;1;1.;21;3;2;-4.2\n"
        "MultiPointConstrAxisym;33;5;0;1;1.;6;0;1\n"
        "AxisymConeRing;7;1.;0.\n"
        "AxisymConeRing;10;1.;0.\n"
        "AxisymConeRing;19;1.;0.\n"
        "AxisymConeRing;21;1.;0.\n"
        "AxisymConeRing;5;1.;0.\n"
        "AxisymConeRing;6;1.;0.");
    SinglePointConstrAxisymCard spcax = cardAt<SinglePointConstrAxisymTable>(model, 0);
    REQUIRE(
        (spcax.id == 4 && spcax.ringId == 7 && spcax.harmonic == 3 && spcax.components == 24 && spcax.enforced == 4.5));
    REQUIRE(cardAt<SinglePointConstrAxisymTable>(model, 1).harmonic == 0);
    const MultiPointConstrAxisymCard mpcax = cardAt<MultiPointConstrAxisymTable>(model, 0);
    REQUIRE(mpcax.id == 36);
    REQUIRE(mpcax.terms == std::vector<MultiPointConstrAxisymTerm>{{19, 5, 1, 1.0}, {21, 3, 2, -4.2}});
    REQUIRE(cardAt<MultiPointConstrAxisymTable>(model, 1).terms[1].coefficient == 0.0); // a blank Aj is 0.0
    REQUIRE(SinglePointConstrAxisymTable::INFO.category == CardCategory::Constraint);
    REQUIRE(MultiPointConstrAxisymTable::INFO.category == CardCategory::Constraint);
    requireRoundTrip(model);
    // The first term is in field 5, the second in field 9: the first field of the continuation line.

    // The rings are RINGAX points of the node ID space: they are checked references.
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<MultiPointConstrAxisymTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Node));

    expectInvalid("SinglePointConstrAxisym;1;2;3;7;1.", "components");
    expectInvalid("SinglePointConstrAxisym;1;2;3;0;1.", "components");
    expectInvalid("SinglePointConstrAxisym;1;2;3;1", "enforcedValue");
    expectInvalid("SinglePointConstrAxisym;1;0;3;1;1.", "ringId");
    expectInvalid("SinglePointConstrAxisym;1;2;-1;1;1.", "harmonicId");
    expectInvalid("SinglePointConstrAxisym;0;2;3;1;1.", "id");
    expectInvalid("MultiPointConstrAxisym;1;5;0;1;0.", "coefficient1");
    expectInvalid("MultiPointConstrAxisym;1;5;0;7;1.", "components1");
    expectInvalid("MultiPointConstrAxisym;1;5;0;1", "coefficient");
    expectInvalid("MultiPointConstrAxisym;1", "ringId1");
    expectInvalid("MultiPointConstrAxisym;1;0;0;1;1.", "ringId1");
}

TEST_CASE("MPCADD combines multipoint constraint sets", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "MultiPointConstr;1;1;1;1.;2;1;-1.\nMultiPointConstr;2;3;1;1.;4;1;-1."
                           "\nMultiPointConstrUnion;10;2;1\nMultiPointConstrUnion;11;1;2;;;;;;3");
    REQUIRE(cardAt<MultiPointConstrUnionTable>(model, 0).sets ==
            std::vector<base::EntityId>{2, 1}); // kept in the order written
    REQUIRE(cardAt<MultiPointConstrUnionTable>(model, 1).sets.size() == 3);
    REQUIRE(unresolvedIn(model) == 1); // the set 3 does not exist
    Renumbering::apply(model, RenumberSpace::ConstraintSet, Renumbering::offset(100));
    REQUIRE(cardAt<MultiPointConstrUnionTable>(model, 0).id == 110);
    REQUIRE(cardAt<MultiPointConstrUnionTable>(model, 0).sets == std::vector<base::EntityId>{102, 101});
    requireRoundTrip(model);

    expectInvalid("MultiPointConstrUnion;1;1", "memberSetId1");
    expectInvalid("MultiPointConstrUnion;1", "memberSetId1");
    expectInvalid("MultiPointConstrUnion;0;1", "id");
    expectInvalid("MultiPointConstrUnion;1;0", "memberSetId1");
}

TEST_CASE("the SUPORT1 Case Control command selects the SUPORT1 cards of its set", "[model][cards]")
{
    Model model = readModelText(
        "[case]\n"
        "SubcaseStart;1\n"
        "FreeBodySupportSelection;8\n"
        "[model]\n"
        "Node;18;;0.;0.;0.\n"
        "Node;17;;0.;0.;0.\n"
        "FreeBodySupportSet;8;18;135\n"
        "FreeBodySupportSet;8;17;3\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(FreeBodySupportSetTable::INFO.category == CardCategory::Constraint);
    REQUIRE(FreeBodySupportSetTable::INFO.idSpace == base::IdSpace::Group);
    Model lacking = readModelText(
        "[case]\nSubcaseStart;1\nFreeBodySupportSelection;6\n[model]\nNode;18;;0.;0.;0.\nFreeBodySupportSet;8;18;135\n",
        true);
    REQUIRE(unresolvedIn(lacking) == 1);
    // The command follows a renumbering of the constraint sets.
    Renumbering::apply(model, RenumberSpace::ConstraintSet, Renumbering::offset(10));
    REQUIRE(cardAt<FreeBodySupportSetTable>(model).id == 18);
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("SUPORT and SUPORT1 list points and components, four pairs a line, with field 8 blank on SUPORT1",
          "[model][cards]")
{
    Model model = readBulk(grids() +
                           "FreeBodySupport;18;135\n"
                           "FreeBodySupport;1;123;2;456;3;;4;1;5;23\n"
                           "FreeBodySupportSet;8;18;135\n"
                           "FreeBodySupportSet;7;1;123;2;456;3;;4;1");
    REQUIRE(FreeBodySupportSetTable::INFO.idSpace == base::IdSpace::Group);
    REQUIRE(FreeBodySupportTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(cardAt<FreeBodySupportTable>(model, 0).dofs == Pairs{{18, 135}});
    REQUIRE(cardAt<FreeBodySupportTable>(model, 1).dofs == Pairs{{1, 123}, {2, 456}, {3, 0}, {4, 1}, {5, 23}});
    REQUIRE(cardAt<FreeBodySupportSetTable>(model, 0).dofs == Pairs{{18, 135}});
    REQUIRE(cardAt<FreeBodySupportSetTable>(model, 1).dofs == Pairs{{1, 123}, {2, 456}, {3, 0}, {4, 1}});
    requireRoundTrip(model);
    // The fourth pair of a SUPORT1 is the first of its continuation line (field 9), the fifth pair of a SUPORT is too.

    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("FreeBodySupport;18;135\nFreeBodySupportSet;8;18;135");
    REQUIRE(unresolvedIn(lacking) == 2);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<FreeBodySupportTable>(model, 0).dofs == Pairs{{118, 135}});

    expectInvalid("FreeBodySupport", "node1");
    expectInvalid("FreeBodySupport;0;123", "node1");
    expectInvalid("FreeBodySupport;1;7", "components1");
    expectInvalid("FreeBodySupport;;123", "node1");
    expectInvalid("FreeBodySupportSet;0;1;123", "id");
    expectInvalid("FreeBodySupportSet;1", "node1");
    expectInvalid("FreeBodySupportSet;1;1;1237", "components1");
}

TEST_CASE("SPCOFF and SPCOFF1 are cards of the constraints that belong to no set", "[model][cards]")
{
    Model model = readBulk(
        "AutoConstrExclusion;27;245;8;1\nAutoConstrExclusionList;3;2;4;11;7;9;12;15;20\nAutoConstrExclusionList;1345;7;"
        "Through;29");
    REQUIRE(AutoConstrExclusionTable::INFO.category == CardCategory::Constraint);
    REQUIRE(AutoConstrExclusionTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(AutoConstrExclusionListTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(cardAt<AutoConstrExclusionTable>(model).dofs == Pairs{{27, 245}, {8, 1}});
    REQUIRE(cardAt<AutoConstrExclusionListTable>(model, 0).components == 3);
    REQUIRE(cardAt<AutoConstrExclusionListTable>(model, 0).nodes.size() == 8);
    REQUIRE(cardAt<AutoConstrExclusionListTable>(model, 1).nodes == std::vector<IdRun>{{7, 29}});
    requireRoundTrip(model);
    expectInvalid("AutoConstrExclusionList;3", "nodes");
    expectInvalid("AutoConstrExclusionList;7;1", "components");
    expectInvalid("AutoConstrExclusion;5;9", "components1");
}

TEST_CASE("GMSPC names a geometric entity and the components held on its grid", "[model][cards]")
{
    Model model = readBulk("GeomSinglePointConstr;1;2;ElementFace;1207\nGeomSinglePointConstr;2;12;GeomCurve;102");
    const GeomSinglePointConstrCard card = cardAt<GeomSinglePointConstrTable>(model, 1);
    REQUIRE((card.id == 2 && card.components == 12 && card.entityType == "GeomCurve" && card.entityId == 102));
    REQUIRE(GeomSinglePointConstrTable::INFO.category == CardCategory::Constraint);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    expectInvalid("GeomSinglePointConstr;1;3;;5", "entityType");
    expectInvalid("GeomSinglePointConstr;1;0;ElementFace;5", "components");
    expectInvalid("GeomSinglePointConstr;1;3;ElementFace", "entityId");
    expectInvalid("GeomSinglePointConstr;0;3;ElementFace;5", "id");
    expectInvalid("GeomSinglePointConstr;1;3;Element Face;5", "entityType"); // a word has no blank
}

// ------------------------------------------------------------------------------------------------ rigid elements

TEST_CASE("RBAR resolves its blank independent and dependent components", "[model][cards]")
{
    Model model = readBulk(
        "RigidBar;6;1;2\nRigidBar;7;1;2;1236;34\nRigidBar;8;3;4;;123456;123456\nRigidBar;9;1;2;12;34;56;6;1.5E-6;20.");
    // CNA is 123456 when CNA and CNB are blank, and with CMA and CMB blank every component that is
    // not independent is dependent.
    RigidBarCard card = cardAt<RigidBarTable>(model, 0);
    REQUIRE((card.independentComponentsA == 123456 && card.independentComponentsB == 0 && card.componentsAtA == 0 &&
             card.componentsAtB == 123456));
    card = cardAt<RigidBarTable>(model, 1);
    REQUIRE((card.independentComponentsA == 1236 && card.independentComponentsB == 34 && card.componentsAtA == 45 &&
             card.componentsAtB == 1256));
    card = cardAt<RigidBarTable>(model, 2);
    REQUIRE((card.independentComponentsA == 0 && card.independentComponentsB == 123456 &&
             card.componentsAtA == 123456 && card.componentsAtB == 0));
    card = cardAt<RigidBarTable>(model, 3);
    REQUIRE((card.independentComponentsA == 12 && card.independentComponentsB == 34 && card.componentsAtA == 56 &&
             card.componentsAtB == 6)); // given: left as they are
    REQUIRE((card.alphaCoefficient == Approx(1.5e-6) && card.referenceTemperature == 20.0));
    // The resolved values are written (they depend on other fields), the zero ones are blank.

    requireRoundTrip(model);
    // The rigid elements share the element ID space.
    REQUIRE(RigidBarTable::INFO.category == CardCategory::Element);
    REQUIRE(RigidBarTable::INFO.idSpace == base::IdSpace::Element);
    REQUIRE_THROWS_AS(readBulk("Rod2;6;1;1;2\nRigidBar;6;1;2"), base::DeckError);

    expectInvalid("RigidBar;0;1;2", "id");
    expectInvalid("RigidBar;1;0;2", "nodeA");
    expectInvalid("RigidBar;1;1;1", "nodeB");
    expectInvalid("RigidBar;1;1;2;7", "independentComponentsA");
    expectInvalid("RigidBar;1;1;2;12;;12", "componentsAtA");
    expectInvalid("RigidBar;1;1;2;;12;;12", "componentsAtB");
    expectInvalid("RigidBar;1;1;2;;;;;x", "alphaCoefficient");
}

TEST_CASE("RBAR1, RJOINT, RROD, RTRPLT and RTRPLT1 have their fields, defaults and rules", "[model][cards]")
{
    Model model = readBulk(
        "RigidBar1;6;3;4\n"
        "RigidBar1;7;3;4;12;;20.\n"
        "RigidJoint;8;3;4\n"
        "RigidJoint;9;1;2;12345\n"
        "RigidRod;24;1;2;3;;4.5E-6\n"
        "RigidRod;15;3;4;;3\n"
        "RigidRod;16;5;6\n"
        "RigidTriaPlate;41;1;2;3\n"
        "RigidTriaPlate;42;10711;10712;10713;234;;123;156;123456;456\n"
        "RigidTriaPlate;43;1;2;3;123456;;;;;;6.5E-6;20.\n"
        "RigidTriaPlate1;51;1;2;3\n"
        "RigidTriaPlate1;52;1;2;3;1236;3;6.E-6\n"
        "RigidTriaPlate1;53;1;2;3;12;;1.E-6;20.");
    // A blank CB is 123456.
    REQUIRE(cardAt<RigidBar1Table>(model, 0).componentsB == 123456);
    REQUIRE((cardAt<RigidBar1Table>(model, 1).componentsB == 12 &&
             cardAt<RigidBar1Table>(model, 1).referenceTemperature == 20.0));
    REQUIRE(cardAt<RigidJointTable>(model, 0).componentsB == 123456);
    REQUIRE(cardAt<RigidJointTable>(model, 1).componentsB == 12345);
    REQUIRE((cardAt<RigidRodTable>(model, 0).componentsAtA == 3 && cardAt<RigidRodTable>(model, 0).componentsAtB == 0));
    REQUIRE(cardAt<RigidRodTable>(model, 0).alphaCoefficient == Approx(4.5e-6));
    REQUIRE(cardAt<RigidRodTable>(model, 1).componentsAtB == 3);
    REQUIRE((cardAt<RigidRodTable>(model, 2).componentsAtA == 0 &&
             cardAt<RigidRodTable>(model, 2).componentsAtB == 0)); // Lagrange: both blank
    // RTRPLT: CMA is the first field of the continuation line; with CNA, CNB and CNC blank CNA is 123456.
    RigidTriaPlateCard plate = cardAt<RigidTriaPlateTable>(model, 0);
    REQUIRE(plate.independent == std::array<std::int32_t, 3>{123456, 0, 0});
    plate = cardAt<RigidTriaPlateTable>(model, 1);
    REQUIRE((plate.nodes == std::array<base::EntityId, 3>{10711, 10712, 10713}));
    REQUIRE(plate.independent == std::array<std::int32_t, 3>{234, 0, 123});
    REQUIRE(plate.dependent == std::array<std::int32_t, 3>{156, 123456, 456});
    plate = cardAt<RigidTriaPlateTable>(model, 2);
    REQUIRE((plate.alphaCoefficient == Approx(6.5e-6) && plate.referenceTemperature == 20.0));
    RigidTriaPlate1Card plate1 = cardAt<RigidTriaPlate1Table>(model, 0);
    REQUIRE(plate1.components == std::array<std::int32_t, 2>{123456, 123456});
    plate1 = cardAt<RigidTriaPlate1Table>(model, 1);
    REQUIRE((plate1.components == std::array<std::int32_t, 2>{1236, 3} && plate1.alphaCoefficient == Approx(6e-6)));
    REQUIRE(cardAt<RigidTriaPlate1Table>(model, 2).referenceTemperature == 20.0);
    requireRoundTrip(model);
    // The defaults are left blank when written, the CMA of an RTRPLT is on the continuation line.

    expectInvalid("RigidBar1;1;1;2;0", "componentsB");
    expectInvalid("RigidBar1;1;1;1", "nodeB");
    expectInvalid("RigidBar1;1;1;2;7", "componentsB");
    expectInvalid("RigidJoint;1;1;1", "nodeB");
    expectInvalid("RigidJoint;1;1;2;0", "componentsB");
    expectInvalid("RigidRod;1;1;2;4", "componentsAtA");
    expectInvalid("RigidRod;1;1;2;1;2", "componentsAtB");
    expectInvalid("RigidRod;1;1;2;;5", "componentsAtB");
    expectInvalid("RigidRod;1;1;1", "nodeB");
    expectInvalid("RigidTriaPlate;1;1;2;2", "nodeC");
    expectInvalid("RigidTriaPlate;1;1;2;3;7", "independentComponentsA");
    expectInvalid("RigidTriaPlate;1;1;2;3;12;;;2", "componentsAtA");
    expectInvalid("RigidTriaPlate;1;1;2;0", "nodeC");
    expectInvalid("RigidTriaPlate1;1;1;2;3;0", "componentsAtB");
    expectInvalid("RigidTriaPlate1;1;1;2;1", "nodeC");
    expectInvalid("RigidTriaPlate1;1;1;2;3;12;7", "componentsAtC");
}

TEST_CASE("RBE1 lists independent pairs, UM and dependent pairs over its lines, with ALPHA and TREF at the end",
          "[model][cards]")
{
    Model model = readBulk(
        "RigidBodyMulti;64;1;63;123456;DependentDofs;2;66;135;4.5E-6\n"
        "RigidBodyMulti;100;4;1;123;2;3;3;2;4;1;DependentDofs;6;7;123;8;456;9;12;10;3;11;4;6.5E-6;20.\n"
        "RigidBodyMulti;200;3;1;123;2;3;3;456;DependentDofs;4;4;1;5;2;6;3;7;4");
    RigidBodyMultiCard card = cardAt<RigidBodyMultiTable>(model, 0);
    REQUIRE(card.independent == Pairs{{63, 123456}});
    REQUIRE(card.dependent == Pairs{{66, 135}});
    REQUIRE((card.alphaCoefficient == Approx(4.5e-6) && card.referenceTemperature == 0.0));
    card = cardAt<RigidBodyMultiTable>(model, 1);
    // Three pairs on the first line, the fourth is in field 10 of the next one (field 9 is blank), UM is in field 17.
    REQUIRE(card.independent == Pairs{{1, 123}, {2, 3}, {3, 2}, {4, 1}});
    REQUIRE(card.dependent == Pairs{{7, 123}, {8, 456}, {9, 12}, {10, 3}, {11, 4}});
    REQUIRE((card.alphaCoefficient == Approx(6.5e-6) && card.referenceTemperature == 20.0));
    // With three independent pairs UM is the first field of the continuation line (field 9); the fourth dependent pair
    // is in field 18.
    card = cardAt<RigidBodyMultiTable>(model, 2);
    REQUIRE(card.independent.size() == 3);
    REQUIRE(card.dependent == Pairs{{4, 1}, {5, 2}, {6, 3}, {7, 4}});
    requireRoundTrip(model);

    // A TREF with no ALPHA writes ALPHA as 0.0 so that the two reals read back.
    Model tref = readBulk("RigidBodyMulti;5;1;1;123456;DependentDofs;2;2;123;;20.");
    REQUIRE((cardAt<RigidBodyMultiTable>(tref).alphaCoefficient == 0.0 &&
             cardAt<RigidBodyMultiTable>(tref).referenceTemperature == 20.0));
    requireRoundTrip(tref);
    // The points are node references.
    Model resolved = readBulk(grids() + "RigidBodyMulti;5;1;1;123456;DependentDofs;2;2;123;3;456");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<RigidBodyMultiTable>(resolved).independent == Pairs{{101, 123456}});
    REQUIRE(cardAt<RigidBodyMultiTable>(resolved).dependent == Pairs{{102, 123}, {103, 456}});
    REQUIRE(unresolvedIn(model) == 18); // none of the points is defined

    expectInvalid("RigidBodyMulti;1;1;1;123;DependentDofs", "dependentNode1");
    expectInvalid("RigidBodyMulti;1;2;1;123;2;456", "dependentMarker");
    expectInvalid("RigidBodyMulti;1;1;1;123;DependentDofs;1;2;0", "dependentComponents1");
    expectInvalid("RigidBodyMulti;1;1;1;123;DependentDofs;1;1;34", "dependentComponents1");
    expectInvalid("RigidBodyMulti;1;1;1;;DependentDofs;1;2;3", "independentComponents1");
    expectInvalid("RigidBodyMulti;1;1;1;123;DependentDofs;1;0;3", "dependentNode1");
    expectInvalid("RigidBodyMulti;1;1;0;123;DependentDofs;1;2;3", "independentNode1");
    expectInvalid("RigidBodyMulti;0;1;1;123;DependentDofs;1;2;3", "id");
    expectInvalid("RigidBodyMulti;1;1;1;123;DependentDofs;2;2;3;4", "dependentComponents2");
    expectInvalid("RigidBodyMulti;1;1;;;DependentDofs;1;2;3", "independentNode1");
}

TEST_CASE("RSPLINE has D/L, then a point and its components after the first point, ending on a point", "[model][cards]")
{
    Model model = readBulk(
        "RigidSpline;83;0.04;21;22;123456;23;;24;123;86;123;81\nRigidSpline;74;;1;2;3;4\nRigidSpline;75;0.2;1;2");
    RigidSplineCard card = cardAt<RigidSplineTable>(model, 0);
    REQUIRE((card.id == 83 && card.ratio == Approx(0.04)));
    REQUIRE(card.points == Pairs{{21, 0}, {22, 123456}, {23, 0}, {24, 123}, {86, 123}, {81, 0}});
    card = cardAt<RigidSplineTable>(model, 1);
    REQUIRE(card.ratio == 0.1); // a blank D/L is 0.1
    REQUIRE(card.points == Pairs{{1, 0}, {2, 3}, {4, 0}});
    requireRoundTrip(model);

    (void)0; // 0.1 is the default: left blank
    Model resolved = readBulk(grids() + "RigidSpline;74;;1;2;3;4");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<RigidSplineTable>(resolved).points == Pairs{{101, 0}, {102, 3}, {104, 0}});

    expectInvalid("RigidSpline;1;;1;2;3;4;5", "components"); // the last field is a components field
    expectInvalid("RigidSpline;1;;1", "node2");
    expectInvalid("RigidSpline;1;-0.1;1;2", "diameterRatio");
    expectInvalid("RigidSpline;1;;1;2;7;3", "components2");
    expectInvalid("RigidSpline;1;;0;2", "node1");
    expectInvalid("RigidSpline;0;;1;2", "id");
    expectInvalid("RigidSpline;1;;1;;3;2", "node2");
}

TEST_CASE("RSSCON joins elements or grid points and its fields follow its type", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "ShellProp;1;1;0.1\n"
                           "SolidProp;2;1\n"
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "ShellQuad4;1;1;1;2;3;4\n"
                           "SolidHexa;2;2;5;6;7;8;9;10;11;12\n"
                           "RigidShellSolidConnection;120;Node;21;22;23;24;25;26\n"
                           "RigidShellSolidConnection;121;Node;33;36;38\n"
                           "RigidShellSolidConnection;125;Elements;1;2\n"
                           "RigidShellSolidConnection;126;;1;2");
    RigidShellSolidConnectionCard card = cardAt<RigidShellSolidConnectionTable>(model, 0);
    REQUIRE((card.kind == RigidShellSolidConnectionType::Grid &&
             card.ids == std::array<base::EntityId, 6>{21, 22, 23, 24, 25, 26}));
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 1).ids == std::array<base::EntityId, 6>{33, 36, 38, 0, 0, 0});
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 2).kind == RigidShellSolidConnectionType::Element);
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 3).kind ==
            RigidShellSolidConnectionType::Element); // a blank TYPE is ELEM
    // The default TYPE is left blank, GRID is written.

    requireRoundTrip(model);
    // The IDs are elements for ELEM and grid points for GRID.
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor elements;
    model.table<RigidShellSolidConnectionTable>().visitReferences(2, elements);
    REQUIRE(elements.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Element));
    RecordingVisitor points;
    model.table<RigidShellSolidConnectionTable>().visitReferences(1, points);
    REQUIRE(points.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::Node));
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 2).ids[0] == 101);
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 2).id == 225);
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 1).ids[0] == 33); // the grid points are not elements
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<RigidShellSolidConnectionTable>(model, 1).ids[0] == 1033);

    expectInvalid("RigidShellSolidConnection;1;FOO;1;2", "kind");
    expectInvalid("RigidShellSolidConnection;1;INTC;1;2;3", "kind");
    expectInvalid("RigidShellSolidConnection;1;Elements;1", "solidIdA1");
    expectInvalid("RigidShellSolidConnection;1;Elements;1;2;3", "solidIdB1");
    expectInvalid("RigidShellSolidConnection;1;Node;;2", "shellId1");
    expectInvalid("RigidShellSolidConnection;1;Node;1;2;-3", "solidIdB1");
    expectInvalid("RigidShellSolidConnection;0;Node;1;2", "id");
}

// ------------------------------------------------------------------------------------------------ sets

TEST_CASE("the DOF set cards list points and components over their continuation lines", "[model][cards]")
{
    Model model = readBulk(
        "AnalysisDofSet;14;3;27;2456;1;4\n"
        "AnalysisDofSet;1;123;2;456;3;;4;1;5;23\n"
        "FixedBoundaryDofSet;14;3\n"
        "FreeBoundaryDofSet;14;3;27;2456\n"
        "GeneralizedDofSet;1;123;2;456;3;;4;1;5;23\n"
        "OmittedDofSet;14;3;27;2456\n"
        "UserDofSet;U5;341;25;19;0\n"
        "UserDofSet;U2;1;123;2;456;3;;4;1;5;23");
    REQUIRE(cardAt<AnalysisDofSetTable>(model, 0).dofs == Pairs{{14, 3}, {27, 2456}, {1, 4}});
    REQUIRE(cardAt<AnalysisDofSetTable>(model, 1).dofs == Pairs{{1, 123}, {2, 456}, {3, 0}, {4, 1}, {5, 23}});
    REQUIRE(cardAt<FixedBoundaryDofSetTable>(model).dofs == Pairs{{14, 3}});
    REQUIRE(cardAt<FreeBoundaryDofSetTable>(model).dofs == Pairs{{14, 3}, {27, 2456}});
    REQUIRE(cardAt<GeneralizedDofSetTable>(model).dofs.size() == 5);
    REQUIRE(cardAt<OmittedDofSetTable>(model).dofs == Pairs{{14, 3}, {27, 2456}});
    REQUIRE(cardAt<UserDofSetTable>(model, 0).setName == "U5");
    REQUIRE(cardAt<UserDofSetTable>(model, 0).dofs == Pairs{{341, 25}, {19, 0}});
    // USET has the set name in field 1, three pairs on its first line and field 8 blank: the fourth pair is in field 9.
    REQUIRE(cardAt<UserDofSetTable>(model, 1).dofs == Pairs{{1, 123}, {2, 456}, {3, 0}, {4, 1}, {5, 23}});
    // The cards have no ID: they belong to no set of the category.
    REQUIRE(AnalysisDofSetTable::INFO.category == CardCategory::Set);
    REQUIRE(AnalysisDofSetTable::INFO.idSpace == base::IdSpace::None);
    REQUIRE(UserDofSetTable::INFO.idSpace == base::IdSpace::None);
    requireRoundTrip(model);

    // Set names preserve their exact case and punctuation.
    REQUIRE(cardAt<UserDofSetTable>(readBulk("UserDofSet;u6;1;123")).setName == "u6");
    const Model named = readBulk("UserDofSet;\"  Mixed, set  \";1;123");
    REQUIRE(cardAt<UserDofSetTable>(named).setName == "  Mixed, set  ");
    requireRoundTrip(named);
    // The points are node references.
    Model resolved = readBulk(grids() + "AnalysisDofSet;14;3;27;2456\nUserDofSet;U1;5;123\nOmittedDofSet;6;1");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<AnalysisDofSetTable>(resolved).dofs == Pairs{{114, 3}, {127, 2456}});
    REQUIRE(cardAt<UserDofSetTable>(resolved).dofs == Pairs{{105, 123}});
    REQUIRE(unresolvedIn(model) == 25); // none of the points is defined

    expectInvalid("AnalysisDofSet", "node1");
    expectInvalid("AnalysisDofSet;0;123", "node1");
    expectInvalid("AnalysisDofSet;1;7", "components1");
    expectInvalid("AnalysisDofSet;1;1237", "components1");
    expectInvalid("AnalysisDofSet;;123", "node1");
    expectInvalid("FixedBoundaryDofSet;1;1;2;x", "components2");
    expectInvalid("FreeBoundaryDofSet;1;1;0", "node2");
    expectInvalid("GeneralizedDofSet;1;1;2;9", "components2");
    expectInvalid("UserDofSet", "setName");
    expectInvalid("UserDofSet;U1", "node1");
    expectInvalid("UserDofSet;U1;1;7", "components1");
    oor::testing::requireModelFileRoundTrip(readModelText("UserDofSet;U1234567890;1;1", true));
}

TEST_CASE("the DOF set cards of the 1 form list points with THRU and write ranges only on request", "[model][cards]")
{
    Model model = readBulk(
        "AnalysisDofSet1;3;1;Through;8\n"
        "AnalysisDofSet1;123;1;5;Through;9;20;22;25;30;31\n"
        "AnalysisDofSet1;;100;200\n"
        "FixedBoundaryDofSet1;3;1;Through;8\n"
        "FreeBoundaryDofSet1;123;4;5\n"
        "GeneralizedDofSet1;0;12001;Through;12030\n"
        "OmittedDofSet1;3;1;Through;8\n"
        "UserDofSet1;SB;123;1;2;3;4;5;6\n"
        "UserDofSet1;U2;123;5710;Through;5716\n"
        "UserDofSet1;ZEROU6;123;5");
    REQUIRE(cardAt<AnalysisDofSet1Table>(model, 0).nodes == std::vector<IdRun>{{1, 8}});
    REQUIRE(cardAt<AnalysisDofSet1Table>(model, 1).nodes ==
            std::vector<IdRun>{{1, 1}, {5, 9}, {20, 20}, {22, 22}, {25, 25}, {30, 30}, {31, 31}});
    REQUIRE((cardAt<AnalysisDofSet1Table>(model, 2).components == 0 &&
             cardAt<AnalysisDofSet1Table>(model, 2).nodes.size() == 2));
    REQUIRE(cardAt<FixedBoundaryDofSet1Table>(model).nodes == std::vector<IdRun>{{1, 8}});
    REQUIRE(cardAt<FreeBoundaryDofSet1Table>(model).components == 123);
    REQUIRE((cardAt<GeneralizedDofSet1Table>(model).components == 0 &&
             cardAt<GeneralizedDofSet1Table>(model).nodes[0].last == 12030));
    REQUIRE(cardAt<OmittedDofSet1Table>(model).components == 3);
    REQUIRE(cardAt<UserDofSet1Table>(model, 0).setName == "SB");
    REQUIRE(cardAt<UserDofSet1Table>(model, 1).nodes == std::vector<IdRun>{{5710, 5716}});
    REQUIRE(cardAt<UserDofSet1Table>(model, 2).setName == "ZEROU6");
    const Model named = readBulk("UserDofSet1;\"  Mixed, set  \";123;1;2");
    REQUIRE(cardAt<UserDofSet1Table>(named).setName == "  Mixed, set  ");
    requireRoundTrip(named);
    requireRoundTrip(model);
    // Lists retain their order and membership.
    const Model listed = readBulk("AnalysisDofSet1;123;1;2;3;4;5\nAnalysisDofSet1;123;7;8\nAnalysisDofSet1;123;1;3;5");

    oor::testing::requireModelFileRoundTrip(listed);
    // A range is never split over two lines and its THRU is never the first field of a line.
    const Model split = readBulk("AnalysisDofSet1;123;1;3;5;7;9;11;13;Through;20;30");

    oor::testing::requireModelFileRoundTrip(split);
    REQUIRE(cardAt<AnalysisDofSet1Table>(readModelText(oor::testing::writeModelText(split), true)).nodes ==
            cardAt<AnalysisDofSet1Table>(split).nodes);
    // The ends of a range are nodes that may be missing; a single point must exist.
    Model nodes = readBulk(grids() + "AnalysisDofSet1;123;5;Through;50\nFixedBoundaryDofSet1;3;41");
    REQUIRE(unresolvedIn(nodes) == 1);
    Renumbering::apply(nodes, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<AnalysisDofSet1Table>(nodes).nodes == std::vector<IdRun>{{105, 150}});

    expectInvalid("AnalysisDofSet1;123;1;Through", "Through");
    expectInvalid("AnalysisDofSet1;123;Through;5", "Through");
    expectInvalid("AnalysisDofSet1;123;5;Through;3", "nodes");
    expectInvalid("AnalysisDofSet1;123", "nodes");
    expectInvalid("AnalysisDofSet1;17;1", "components");
    expectInvalid("AnalysisDofSet1;12;0", "nodes");
    expectInvalid("FixedBoundaryDofSet1;12;1;Through;5;Through;9", "Through");
    expectInvalid("UserDofSet1;;123;1", "setName");
    expectInvalid("UserDofSet1;U1;123", "nodes");
    expectInvalid("UserDofSet1;U1;7;1", "components");
}

TEST_CASE("BNDGRID lists grid points with the components of their prescribed motion", "[model][cards]")
{
    const Model model = readBulk("ShapeBoundaryNodeSet;123;5\nShapeBoundaryNodeSet;123456;1;Through;10;20;30");
    REQUIRE(cardAt<ShapeBoundaryNodeSetTable>(model, 0).components == 123);
    REQUIRE(cardAt<ShapeBoundaryNodeSetTable>(model, 1).nodes == std::vector<IdRun>{{1, 10}, {20, 20}, {30, 30}});
    REQUIRE(ShapeBoundaryNodeSetTable::INFO.category == CardCategory::Set);
    requireRoundTrip(model);
    expectInvalid("ShapeBoundaryNodeSet;;5", "components");
    expectInvalid("ShapeBoundaryNodeSet;0;5", "components");
    expectInvalid("ShapeBoundaryNodeSet;123", "nodes");
    expectInvalid("ShapeBoundaryNodeSet;123;0", "nodes");
}

TEST_CASE("SET1 is a set of IDs, with THRU ranges, SKIN or negative IDs, found by the targets of the sets",
          "[model][cards]")
{
    Model model = readBulk(
        "IdListSet1;3;27;54;81;108;13;14;15;16\n"
        "IdListSet1;6;24;35;Through;47;63;Through;72;11;58\n"
        "IdListSet1;7;SkinElems\n"
        "IdListSet1;8;100\n"
        "IdListSet1;40030;-8;-12;-15\n"
        "IdListSet1;3;200");
    REQUIRE(cardAt<IdListSet1Table>(model, 0).ids.size() == 8);
    REQUIRE(cardAt<IdListSet1Table>(model, 1).ids ==
            std::vector<IdRun>{{24, 24}, {35, 47}, {63, 72}, {11, 11}, {58, 58}});
    REQUIRE((cardAt<IdListSet1Table>(model, 2).skin && cardAt<IdListSet1Table>(model, 2).ids.empty()));
    REQUIRE(cardAt<IdListSet1Table>(model, 4).ids == std::vector<IdRun>{{-8, -8}, {-12, -12}, {-15, -15}});
    REQUIRE(IdListSet1Table::INFO.category == CardCategory::Set);
    REQUIRE(IdListSet1Table::INFO.idSpace == base::IdSpace::Group);
    // A reference to a Bulk Data set resolves to the SET1 (or SET3) that has the ID; several cards may share it.
    const std::span<const CardCategory> categories = categoriesOfSet(ReferenceTarget::Set);
    REQUIRE((categories.size() == 1 && categories.front() == CardCategory::Set));
    REQUIRE(resolveReference(model, ReferenceTarget::Set, 6).has_value());
    REQUIRE(cardsOfSet(model, CardCategory::Set, 3).size() == 2);
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::Set, 99).has_value());
    REQUIRE(setIdsOfCategory(model, CardCategory::Set).size() == 5);
    requireRoundTrip(model);
    // The members are not references: a renumbering of the nodes leaves them alone, that of the sets renumbers the
    // SIDs.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<IdListSet1Table>(model, 1).ids[1].first == 35);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    REQUIRE(cardAt<IdListSet1Table>(model, 1).id == 1006);
    REQUIRE(cardAt<IdListSet1Table>(model, 5).id == 1003);
    // An explicit list retains its members.
    const Model listed = readBulk("IdListSet1;3;1;2;3;4;5;9");
    oor::testing::requireModelFileRoundTrip(listed);

    expectInvalid("IdListSet1;0;1", "id");
    expectInvalid("IdListSet1;1", "ids");
    expectInvalid("IdListSet1;1;0", "ids");
    expectInvalid("IdListSet1;1;SkinElems;5", "skin");
    expectInvalid("IdListSet1;1;5;Through;3", "ids");
    expectInvalid("IdListSet1;1;Through;3", "Through");
}

TEST_CASE("SET3 says what its IDs are and they are references of that kind", "[model][cards]")
{
    Model model = readBulk(grids() +
                           "ShellQuad4;5;7;1;2;3;4\n"
                           "ShellProp;7;1;0.1\n"
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "TypedListSet3;1;GeomPoint;12;14;16;19;22;25\n"
                           "TypedListSet3;33;GeomPoint;24;Through;58\n"
                           "TypedListSet3;5;Elements;5;6\n"
                           "TypedListSet3;6;Node;1;Through;10\n"
                           "TypedListSet3;7;Property;7;8\n"
                           "TypedListSet3;8;RigidElemsIncluded;5;200\n"
                           "TypedListSet3;9;RigidElemsExcluded;100\n"
                           "TypedListSet3;10;Module;1;2\n"
                           "TypedListSet3;11;Node;0");
    REQUIRE(cardAt<TypedListSet3Table>(model, 0).kind == TypedListSet3Kind::Point);
    REQUIRE(cardAt<TypedListSet3Table>(model, 1).ids == std::vector<IdRun>{{24, 58}});
    REQUIRE(cardAt<TypedListSet3Table>(model, 5).kind == TypedListSet3Kind::RigidIncluded);
    REQUIRE(cardAt<TypedListSet3Table>(model, 6).kind == TypedListSet3Kind::RigidExcluded);
    REQUIRE(cardAt<TypedListSet3Table>(model, 7).kind == TypedListSet3Kind::Module);
    REQUIRE(TypedListSet3Format::kindName(TypedListSet3Kind::RigidIncluded) == "RigidElemsIncluded");
    REQUIRE(TypedListSet3Format::kindName(TypedListSet3Kind::Grid) == "Node");
    // Points and modules are not cards of the model; elements, rigid elements, grid points and properties must exist.
    // The missing ones: the element 6, the RBEin 200 and the RBEex 100, the property 8.
    REQUIRE(unresolvedIn(model) == 4);
    RecordingVisitor visitor;
    model.table<TypedListSet3Table>().visitReferences(3, visitor);
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange});
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<TypedListSet3Table>(model, 2).ids == std::vector<IdRun>{{1005, 1005}, {1006, 1006}});
    REQUIRE(cardAt<TypedListSet3Table>(model, 3).ids == std::vector<IdRun>{{1, 10}});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(cardAt<TypedListSet3Table>(model, 3).ids == std::vector<IdRun>{{1001, 1010}});
    REQUIRE(cardAt<TypedListSet3Table>(model, 0).ids.size() == 6); // the points stay as they are
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE(cardAt<TypedListSet3Table>(model, 4).ids == std::vector<IdRun>{{107, 107}, {108, 108}});
    requireRoundTrip(model);
    // The word of DES is a neutral name, case-sensitive.
    REQUIRE(cardAt<TypedListSet3Table>(readBulk("TypedListSet3;1;RigidElemsExcluded;5")).kind ==
            TypedListSet3Kind::RigidExcluded);
    expectInvalid("TypedListSet3;1;rigidElemsExcluded;5", "kind");

    expectInvalid("TypedListSet3;1;FOO;1", "kind");
    expectInvalid("TypedListSet3;1;Node", "ids");
    expectInvalid("TypedListSet3;1;Node;-1", "ids");
    expectInvalid("TypedListSet3;0;Node;1", "id");
    expectInvalid("TypedListSet3;1;;1", "kind");
    expectInvalid("TypedListSet3;1;Node;5;Through;3", "ids");
}

TEST_CASE("SEQGP gives sequence numbers, whole or real, to up to four points", "[model][cards]")
{
    Model model = readBulk(grids() + "NodeSequence;6481;12.7;17;0.3;1;2.4;8;100\nNodeSequence;3;10");
    const NodeSequenceCard card = cardAt<NodeSequenceTable>(model, 0);
    REQUIRE(card.points.size() == 4);
    REQUIRE((card.points[0].node == 6481 && card.points[0].sequence == Approx(12.7)));
    REQUIRE(card.points[3].sequence == 100.0);
    REQUIRE(NodeSequenceTable::INFO.category == CardCategory::Other);
    REQUIRE(NodeSequenceTable::INFO.idSpace == base::IdSpace::None);
    requireRoundTrip(model);
    // A whole sequence number is written as an integer, a fractional one as a real.

    // The points are nodes; the sequence numbers are not references.
    REQUIRE(unresolvedIn(model) == 1); // 6481
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<NodeSequenceTable>(model, 1).points[0].node == 103);
    REQUIRE(cardAt<NodeSequenceTable>(model, 1).points[0].sequence == 10.0);

    expectInvalid("NodeSequence;1", "sequenceId1");
    expectInvalid("NodeSequence;1;0.", "sequenceId1");
    expectInvalid("NodeSequence;1;-5", "sequenceId1");
    expectInvalid("NodeSequence;0;5", "node1");
    expectInvalid("NodeSequence;1;2;3", "sequenceId2");
    expectInvalid("NodeSequence;1;1;2;2;3;3;4;4;5;5", "node1");
    expectInvalid("NodeSequence;1;x", "sequenceId1");
}
