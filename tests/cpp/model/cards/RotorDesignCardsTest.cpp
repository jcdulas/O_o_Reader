// Checks of the rotor and design region cards: the rotor cards (ROTOR, ROTORAX, ROTORSE, RSPINR, RSPINT, REFROT,
// REFRTSE, ROTBENT, CAMPBLL), the discrete values and the design regions (DDVAL, BEADVAR, TOMVAR, DVLREL1, DVPSURF),
// and what the new kinds of reference (Rotor, DiscreteValueSet) gave to the cards that were there before them (ROTORG,
// RGYRO, ROTHYBD, DESVAR). Per card: the fields in their format reference positions, the defaults, the variants, the
// references it lists and what each one points to, the renumbering of the spaces it owns or names, the refused values
// and the round trip through the three formats. The generic tests of CardRoundTripTest.cpp read, write and copy every
// sample; this file covers what each card means.

#include "oor/model/cards/dynamics/rotorHybridDamping/RotorHybridDampingTable.hpp"
#include "oor/model/cards/optimisation/designBeadRegion/DesignBeadRegionTable.hpp"
#include "oor/model/cards/optimisation/designControlSurface/DesignControlSurfaceTable.hpp"
#include "oor/model/cards/optimisation/designDiscreteValues/DesignDiscreteValuesTable.hpp"
#include "oor/model/cards/optimisation/designLoadRelation1/DesignLoadRelation1Table.hpp"
#include "oor/model/cards/optimisation/designTopometryRegion/DesignTopometryRegionTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/cards/solution_control/campbellDiagram/CampbellDiagramTable.hpp"
#include "oor/model/cards/solution_control/referenceRotor/ReferenceRotorTable.hpp"
#include "oor/model/cards/solution_control/rotorAxisym/RotorAxisymTable.hpp"
#include "oor/model/cards/solution_control/rotorInitialBend/RotorInitialBendTable.hpp"
#include "oor/model/cards/solution_control/rotorLineNodes/RotorLineNodesTable.hpp"
#include "oor/model/cards/solution_control/rotorModel3D/RotorModel3DTable.hpp"
#include "oor/model/cards/solution_control/rotorRelativeSpin/RotorRelativeSpinTable.hpp"
#include "oor/model/cards/solution_control/rotorSuperelement/RotorSuperelementTable.hpp"
#include "oor/model/cards/solution_control/rotorTransientSpin/RotorTransientSpinTable.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsTable.hpp"
#include "oor/model/cards/solution_control/superelementReferenceRotor/SuperelementReferenceRotorTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
using Catch::Approx;

namespace
{

// One reference of a card as a visitor sees it: the field, what it points to and the ID.
struct Seen
{
    std::string field;
    ReferenceTarget target;
    base::EntityId id;

    [[nodiscard]] bool operator==(const Seen&) const noexcept = default;
};

class ListingVisitor final : public ReferenceVisitor
{
public:
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        m_seen.push_back(Seen{std::string(field), target, id});
        return id;
    }

    [[nodiscard]] const std::vector<Seen>& seen() const noexcept { return m_seen; }

private:
    std::vector<Seen> m_seen;
};

// The references a card lists, in the order it lists them.
template <typename TableT>
std::vector<Seen> referencesOf(Model& model, std::uint32_t row = 0)
{
    ListingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor.seen();
}

using Seens = std::vector<Seen>;
using Runs = std::vector<ContactRun>;

// The unresolved references of a model made for the occasion (the helper of the support file takes a named model).
std::size_t unresolvedIn(Model&& model)
{
    return oor::testing::unresolvedIn(model);
}

// The two grid points that the rotor cards of the decks name, and the elements and properties of a ROTOR.
const std::string NODES_TEXT = "Node;1;;0.;0.;0.\nNode;2;;0.;0.;1.\nNode;3;;0.;0.;2.\n";

// A model with one rotor of each kind, and a card of each kind that names a rotor: every reference resolves. The rotors
// are 11 (ROTORG), 12 (ROTOR), 13 (ROTORAX) and 14 (ROTORSE, in the superelement 5 that the SESET defines).
const std::string ROTOR_WORLD_TEXT = NODES_TEXT +
                                     "IsoMat;1;2.1E+11;;0.3\n"
                                     "RodProp;7;1;1.\n"
                                     "Rod2;20;7;1;2\n"
                                     "Rod2;21;7;2;3\n"
                                     "SuperelementInteriorNodes;5;3\n"
                                     "PointForceLoad;9;1;;1.;0.;0.;1.\n"
                                     "TableDynamic1;40;;;0.;1.;1.;2.;EndTable\n"
                                     "RotorLineNodes;11;1;2\n"
                                     "RotorModel3D;12;;Elements;2;20;21;Axis;2;1;3\n"
                                     "RotorAxisym;13;Node;1;2\n"
                                     "RotorSuperelement;14;5\n"
                                     "RotordynamicParams;100;SynchronousWhirl;11;RevolutionsPerMinute;1200.;4800.\n"
                                     "RotorRelativeSpin;12;1;3;RevolutionsPerMinute;;2.5\n"
                                     "RotorTransientSpin;13;1;2;RevolutionsPerMinute;40\n"
                                     "ReferenceRotor;100;11;12;13\n"
                                     "SuperelementReferenceRotor;100;11;;13;;12;;14;5\n"
                                     "RotorInitialBend;200;12;9;BearingDeflectionProfile;1;1;3\n";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// The rotor space and what names it
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("the ROTORG, ROTOR, ROTORAX and ROTORSE cards are one space of rotors that the other rotor cards name",
          "[model][cards][rotor]")
{
    REQUIRE((RotorLineNodesTable::INFO.category == CardCategory::Rotor &&
             RotorLineNodesTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE((RotorModel3DTable::INFO.category == CardCategory::Rotor &&
             RotorModel3DTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE((RotorAxisymTable::INFO.category == CardCategory::Rotor &&
             RotorAxisymTable::INFO.idSpace == base::IdSpace::Group));
    REQUIRE((RotorSuperelementTable::INFO.category == CardCategory::Rotor &&
             RotorSuperelementTable::INFO.idSpace == base::IdSpace::Group));

    Model model = readBulk(ROTOR_WORLD_TEXT);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(referencesOf<RotordynamicParamsTable>(model) == Seens{{"referenceRotorId", ReferenceTarget::Rotor, 11}});
    REQUIRE(referencesOf<RotorRelativeSpinTable>(model).front() == Seen{"rotorId", ReferenceTarget::Rotor, 12});
    REQUIRE(referencesOf<RotorTransientSpinTable>(model).front() == Seen{"rotorId", ReferenceTarget::Rotor, 13});
    REQUIRE(referencesOf<RotorInitialBendTable>(model).front() == Seen{"rotorId", ReferenceTarget::Rotor, 12});

    // A rotor that no card defines is reported, whatever the kind of card that names it.
    REQUIRE(unresolvedIn(readBulk("Node;1\nNode;2\nRotorRelativeSpin;12;1;2;RevolutionsPerMinute;;2.5")) == 1);
    REQUIRE(unresolvedIn(readBulk("RotordynamicParams;100;SynchronousWhirl;11;RevolutionsPerMinute")) == 1);
    REQUIRE(unresolvedIn(readBulk("RotorHybridDamping;12;15")) == 2); // the rotor and the HYBDAMP

    // Renumbering the rotors moves the four kinds of card and every card that names one, and nothing else.
    REQUIRE(Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(1000)) == 4);
    REQUIRE((cardAt<RotorLineNodesTable>(model).id == 1011 && cardAt<RotorModel3DTable>(model).id == 1012));
    REQUIRE((cardAt<RotorAxisymTable>(model).id == 1013 && cardAt<RotorSuperelementTable>(model).id == 1014));
    REQUIRE(cardAt<RotordynamicParamsTable>(model).referenceRotorId == 1011);
    REQUIRE(cardAt<RotordynamicParamsTable>(model).id == 100); // the RGYRO set is not a rotor
    REQUIRE(cardAt<RotorRelativeSpinTable>(model).rotorId == 1012);
    REQUIRE(cardAt<RotorTransientSpinTable>(model).rotorId == 1013);
    REQUIRE(
        (cardAt<ReferenceRotorTable>(model).referenceRotorId == 1011 && cardAt<ReferenceRotorTable>(model).id == 100));
    REQUIRE(cardAt<ReferenceRotorTable>(model).rotors == Runs{ContactRun{1012, 1012, 1}, ContactRun{1013, 1013, 1}});
    REQUIRE(cardAt<RotorInitialBendTable>(model).rotorId == 1012);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("an RGYRO names its reference rotor, and a rotor of another superelement is not looked up",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "Node;1\nNode;2\nRotorLineNodes;5;1;2\nRotordynamicParams;1;SynchronousWhirl;5;"
        "RevolutionsPerMinute\nRotordynamicParams;2;SynchronousWhirl;7;RevolutionsPerMinute;;;;10");
    REQUIRE(referencesOf<RotordynamicParamsTable>(model, 0) == Seens{{"referenceRotorId", ReferenceTarget::Rotor, 5}});
    REQUIRE(referencesOf<RotordynamicParamsTable>(model, 1).empty()); // ROTRSEID 10: the rotor is a card of that part
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("RotordynamicParams;1;SynchronousWhirl;7;RevolutionsPerMinute")) == 1);

    Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(100));
    REQUIRE(cardAt<RotordynamicParamsTable>(model, 0).referenceRotorId == 105);
    REQUIRE(cardAt<RotordynamicParamsTable>(model, 1).referenceRotorId ==
            7); // not a rotor of the residual structure: left alone
    requireRoundTrip(model);
}

TEST_CASE("a ROTHYBD names the rotors that take a hybrid damping", "[model][cards][rotor]")
{
    Model model =
        readBulk("Node;1\nNode;2\nRotorLineNodes;1;1;2\nRotorLineNodes;2;1;2\nRotorHybridDamping;1;201;2;202");
    REQUIRE(referencesOf<RotorHybridDampingTable>(model) ==
            Seens{{"rotorId", ReferenceTarget::Rotor, 1},
                  {"hybridDampingId", ReferenceTarget::HybridDamping, 201},
                  {"rotorId", ReferenceTarget::Rotor, 2},
                  {"hybridDampingId", ReferenceTarget::HybridDamping, 202}});
    REQUIRE(unresolvedIn(model) == 2); // the two HYBDAMP cards the deck leaves out
    Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(10));
    REQUIRE((cardAt<RotorHybridDampingTable>(model).pairs ==
             std::vector<RotorHybridPair>{{11, 201}, {12, 202}})); // the HYBDAMP numbers are another space
}

TEST_CASE("an edited ROTOR, REFRTSE, DDVAL, TOMVAR or DVPSURF is checked again, stored, given another ID and deleted",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "RotorModel3D;12;;Elements;1;20;Axis;2;1;3\n"
        "SuperelementReferenceRotor;100;11;;13\n"
        "DesignDiscreteValues;22;500.;1.E+3\n"
        "DesignTopometryRegion;7;ShellProp;5;thickness;10.\n"
        "DesignControlSurface;5;FLAP;1;2;0.5\n");
    const auto where = [](std::uint16_t type) { return CardLocation{type, 0}; };

    // The check of the format is what the Python setters apply before they store: a bad edit changes nothing.
    RotorModel3DCard rotor = cardAt<RotorModel3DTable>(model);
    rotor.axis = {1};
    REQUIRE(RotorModel3DFormat::check(rotor).has_value());
    rotor.axis = {1, 3, 4};
    REQUIRE_FALSE(RotorModel3DFormat::check(rotor).has_value());
    model.table<RotorModel3DTable>().store(0, rotor);
    REQUIRE(cardAt<RotorModel3DTable>(model).axis == std::vector<base::EntityId>{1, 3, 4});
    model.setId(where(typeIndexOf<RotorModel3DTable>()), 99);
    REQUIRE(cardAt<RotorModel3DTable>(model).id == 99); // the number is in the value and in its column
    REQUIRE(model.table<RotorModel3DTable>().idColumn().get(0) == 99);

    SuperelementReferenceRotorCard refrtse = cardAt<SuperelementReferenceRotorTable>(model);
    refrtse.rotors.push_back(RotorInSuperelement{14, -1});
    REQUIRE(SuperelementReferenceRotorFormat::check(refrtse).has_value());
    refrtse.rotors.back().superelement = 2;
    REQUIRE_FALSE(SuperelementReferenceRotorFormat::check(refrtse).has_value());

    DesignDiscreteValuesCard values = cardAt<DesignDiscreteValuesTable>(model);
    values.values.push_back(DesignDiscreteValuesRun{1.0, 2.0, -0.5});
    REQUIRE(DesignDiscreteValuesFormat::check(values).has_value()); // the increment goes the wrong way
    values.values.back().increment = 0.5;
    REQUIRE_FALSE(DesignDiscreteValuesFormat::check(values).has_value());
    model.table<DesignDiscreteValuesTable>().store(0, values);
    REQUIRE(designDiscreteValuesValues(cardAt<DesignDiscreteValuesTable>(model)) ==
            std::vector<double>{500.0, 1000.0, 1.0, 1.5, 2.0});

    // The bounds are resolved when the card is read, not when it is edited: a new XINIT leaves them as they are.
    DesignTopometryRegionCard tomvar = cardAt<DesignTopometryRegionTable>(model);
    REQUIRE((tomvar.lowerBound == 5.0 && tomvar.upperBound == 15.0));
    tomvar.initialValue = 20.0;
    REQUIRE((tomvar.lowerBound == 5.0 && tomvar.upperBound == 15.0));
    tomvar.lowerBound.reset();
    tomvar.resolveDefaultBounds();
    REQUIRE(tomvar.lowerBound == 10.0);
    tomvar.field = DesignField{"0"};
    REQUIRE(DesignTopometryRegionFormat::check(tomvar).has_value());

    // A deleted card is gone from its table, and a model that was edited writes and reads back.
    model.remove(where(typeIndexOf<DesignControlSurfaceTable>()));
    REQUIRE(model.findTable<DesignControlSurfaceTable>()->rows().aliveCount() == 0);
    model.remove(where(typeIndexOf<DesignDiscreteValuesTable>()));
    REQUIRE(model.findTable<DesignDiscreteValuesTable>()->rows().aliveCount() == 0);
    requireRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// ROTOR
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a ROTOR lists elements, properties and the axis of rotation, each on a line of its own",
          "[model][cards][rotor]")
{
    Model model = readBulk(NODES_TEXT +
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "RodProp;3;1;1.\n"
                           "RodProp;4;1;1.\n"
                           "Rod2;10;3;1;2\n"
                           "Rod2;11;3;1;2\n"
                           "Rod2;12;3;2;3\n"
                           "RotorModel3D;10;Rotation;Elements;3;10;Through;12;Property;3;1;Through;5;Axis;2;1;2\n"
                           "RotorModel3D;11;;Property;1;3;Axis;2;2;3\n"
                           "RotorModel3D;12;FixedEnd;Elements;11;1;2;3;4;5;6;7;8;Through;20;30;Axis;3;1;2;3\n");
    const RotorModel3DCard example = cardAt<RotorModel3DTable>(model, 0);
    REQUIRE((example.id == 10 && example.frame == RotorModel3DFrame::Rotating));
    REQUIRE(example.elements == Runs{ContactRun{10, 12, 1}});
    REQUIRE(example.properties == Runs{ContactRun{1, 5, 1}});
    REQUIRE(example.axis == std::vector<base::EntityId>{1, 2});
    // A blank FRAME is FIX; a card with properties only has no ELEM line.
    const RotorModel3DCard properties = cardAt<RotorModel3DTable>(model, 1);
    REQUIRE((properties.frame == RotorModel3DFrame::Fixed && properties.elements.empty()));
    REQUIRE(properties.properties == Runs{ContactRun{3, 3, 1}});
    // The list goes on over the lines after the keyword, with a blank first field; a range is never split in two.
    const RotorModel3DCard longList = cardAt<RotorModel3DTable>(model, 2);
    REQUIRE(longList.frame == RotorModel3DFrame::Fixed);
    REQUIRE(longList.elements == Runs{ContactRun{1, 1, 1}, ContactRun{2, 2, 1}, ContactRun{3, 3, 1},
                                      ContactRun{4, 4, 1}, ContactRun{5, 5, 1}, ContactRun{6, 6, 1},
                                      ContactRun{7, 7, 1}, ContactRun{8, 20, 1}, ContactRun{30, 30, 1}});
    requireRoundTrip(model);

    // The references: the ends of a range are allowed not to exist, a single ID is not.
    REQUIRE(referencesOf<RotorModel3DTable>(model, 0) == Seens{{"ids", ReferenceTarget::ElementInRange, 10},
                                                               {"ids", ReferenceTarget::ElementInRange, 12},
                                                               {"ids", ReferenceTarget::PropertyInRange, 1},
                                                               {"ids", ReferenceTarget::PropertyInRange, 5},
                                                               {"ids", ReferenceTarget::Node, 1},
                                                               {"ids", ReferenceTarget::Node, 2}});
    REQUIRE(referencesOf<RotorModel3DTable>(model, 1) == Seens{{"ids", ReferenceTarget::Property, 3},
                                                               {"ids", ReferenceTarget::Node, 2},
                                                               {"ids", ReferenceTarget::Node, 3}});
    Model small = readBulk(NODES_TEXT +
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "RodProp;3;1;1.\n"
                           "Rod2;10;3;1;2\n"
                           "Rod2;12;3;2;3\n"
                           "RotorModel3D;10;;Elements;3;10;Through;12;Property;1;3;Axis;2;1;2\n");
    REQUIRE(unresolvedIn(small) == 0);
    REQUIRE(unresolvedIn(readBulk("RotorModel3D;10;;Elements;1;10;Axis;2;1;2")) == 3);

    // Each space renumbers its own IDs, the ends of the range included.
    Renumbering::apply(small, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE(cardAt<RotorModel3DTable>(small).elements == Runs{ContactRun{1010, 1012, 1}});
    Renumbering::apply(small, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE(cardAt<RotorModel3DTable>(small).properties == Runs{ContactRun{103, 103, 1}});
    Renumbering::apply(small, RenumberSpace::Node, Renumbering::offset(50));
    REQUIRE(cardAt<RotorModel3DTable>(small).axis == std::vector<base::EntityId>{51, 52});
    REQUIRE(unresolvedIn(small) == 0);
}

TEST_CASE("a ROTOR refuses an order, a layout or a value the format reference does not allow", "[model][cards][rotor]")
{
    expectInvalid("RotorModel3D;0;;Elements;1;10;Axis;2;1;2", "id");
    expectInvalid("RotorModel3D;10;SPIN;Elements;1;10;Axis;2;1;2", "frame");
    expectInvalid("RotorModel3D;10;;Axis;2;1;2", "listKind");        // elements or properties are needed
    expectInvalid("RotorModel3D;10;;Elements;1;10", "ids");          // the axis is needed
    expectInvalid("RotorModel3D;10;;Elements;1;10;Axis;1;1", "ids"); // and has two grid points at least
    expectInvalid("RotorModel3D;10;;Property;1;3;Elements;1;10;Axis;2;1;2",
                  "listKind");                                                   // ELEM first when both are given
    expectInvalid("RotorModel3D;10;;Elements;;Axis;2;1;2", "ids");               // a keyword with nothing after it
    expectInvalid("RotorModel3D;10;;Elements;3;10;Through;5;Axis;2;1;2", "ids"); // a range goes up
    expectInvalid("RotorModel3D;10;;Elements;5;10;Through;12;By;2;Axis;2;1;2", "ids"); // the format has no BY here
    expectInvalid("RotorModel3D;10;;Elements;1;10;Axis;3;1;Through;5", "ids");         // the axis is a list of grids
    expectInvalid("RotorModel3D;10;;Elements;1;0;Axis;2;1;2", "ids");
    expectInvalid("RotorModel3D;10;;Elements;1;10;Axis;2;1;0", "ids");
}

// ---------------------------------------------------------------------------------------------------------------------
// ROTORAX
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a ROTORAX lists elements, properties or grid points of an axisymmetric rotor", "[model][cards][rotor]")
{
    Model model = readBulk(NODES_TEXT +
                           "Node;52\n"
                           "Node;67\n"
                           "Node;71\n"
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "RodProp;10;1;1.\n"
                           "Rod2;5;10;1;2\n"
                           "Rod2;25;10;2;3\n"
                           "RotorAxisym;100;Elements;5;Through;25\n"
                           "RotorAxisym;10;Property;10\n"
                           "RotorAxisym;15;Node;52;67;71\n"
                           "RotorAxisym;100;Node;52;71\n"
                           "RotorAxisym;101;Elements;100;Through;200;By;10\n"
                           "RotorAxisym;102;Elements;1;2;3;4;5;6;7;8;9;Through;20\n");
    REQUIRE((RotorAxisymTable::INFO.category == CardCategory::Rotor &&
             RotorAxisymTable::INFO.idSpace == base::IdSpace::Group));
    const RotorAxisymCard elements = cardAt<RotorAxisymTable>(model, 0);
    REQUIRE((elements.id == 100 && elements.listType == RotorAxisymListType::Element));
    REQUIRE(elements.ids == Runs{ContactRun{5, 25, 1}});
    REQUIRE(cardAt<RotorAxisymTable>(model, 1).listType == RotorAxisymListType::Property);
    const RotorAxisymCard grids = cardAt<RotorAxisymTable>(model, 2);
    REQUIRE((grids.id == 15 && grids.listType == RotorAxisymListType::Grid));
    REQUIRE(grids.ids == Runs{ContactRun{52, 52, 1}, ContactRun{67, 67, 1}, ContactRun{71, 71, 1}});
    // Several cards make one rotor: the ROTORID is shared, no ID is repeated in error.
    REQUIRE(cardAt<RotorAxisymTable>(model, 3).id == 100);
    const RotorAxisymCard stepped = cardAt<RotorAxisymTable>(model, 4);
    REQUIRE(stepped.ids == Runs{ContactRun{100, 200, 10}}); // BY is kept
    const RotorAxisymCard longList = cardAt<RotorAxisymTable>(model, 5);
    REQUIRE(longList.ids.size() == 9);
    REQUIRE((longList.ids[6] == ContactRun{7, 7, 1} && longList.ids[8] == ContactRun{9, 20, 1}));
    requireRoundTrip(model);

    REQUIRE(referencesOf<RotorAxisymTable>(model, 0) ==
            Seens{{"nodeIds", ReferenceTarget::ElementInRange, 5}, {"nodeIds", ReferenceTarget::ElementInRange, 25}});
    REQUIRE(referencesOf<RotorAxisymTable>(model, 1) == Seens{{"nodeIds", ReferenceTarget::Property, 10}});
    REQUIRE(referencesOf<RotorAxisymTable>(model, 2) == Seens{{"nodeIds", ReferenceTarget::Node, 52},
                                                              {"nodeIds", ReferenceTarget::Node, 67},
                                                              {"nodeIds", ReferenceTarget::Node, 71}});

    // Each kind of list follows the renumbering of what it names.
    Model small = readBulk(NODES_TEXT +
                           "IsoMat;1;2.1E+11;;0.3\n"
                           "RodProp;10;1;1.\n"
                           "Rod2;5;10;1;2\n"
                           "Rod2;25;10;2;3\n"
                           "RotorAxisym;100;Elements;5;Through;25\n"
                           "RotorAxisym;10;Property;10\n"
                           "RotorAxisym;15;Node;1;3\n");
    REQUIRE(unresolvedIn(small) == 0);
    Renumbering::apply(small, RenumberSpace::Element, Renumbering::offset(1000));
    Renumbering::apply(small, RenumberSpace::Property, Renumbering::offset(100));
    Renumbering::apply(small, RenumberSpace::Node, Renumbering::offset(50));
    REQUIRE(cardAt<RotorAxisymTable>(small, 0).ids == Runs{ContactRun{1005, 1025, 1}});
    REQUIRE(cardAt<RotorAxisymTable>(small, 1).ids == Runs{ContactRun{110, 110, 1}});
    REQUIRE(cardAt<RotorAxisymTable>(small, 2).ids == Runs{ContactRun{51, 51, 1}, ContactRun{53, 53, 1}});
    REQUIRE(unresolvedIn(small) == 0);
    REQUIRE(unresolvedIn(readBulk("RotorAxisym;15;Node;1;3")) == 2);
}

TEST_CASE("a ROTORAX refuses a list the format reference does not allow", "[model][cards][rotor]")
{
    expectInvalid("RotorAxisym;0;Elements;5", "id");
    expectInvalid("RotorAxisym;1", "listType");
    expectInvalid("RotorAxisym;1;CELL;5", "listType");
    expectInvalid("RotorAxisym;1;Elements", "nodeIds");
    expectInvalid("RotorAxisym;1;Elements;0", "nodeIds");
    expectInvalid("RotorAxisym;1;Node;50", "nodeIds"); // two grid points at least
    expectInvalid("RotorAxisym;1;Elements;9;Through;5", "nodeIds");
    expectInvalid("RotorAxisym;1;Elements;5;Through;9;By;0", "By");
    expectInvalid("RotorAxisym;1;Elements;5;Through;9;By;-2", "By");
    expectInvalid("RotorAxisym;1;Elements;5;By;2", "By");
    expectInvalid("RotorAxisym;1;Elements;5;Through", "Through");
    expectInvalid("RotorAxisym;1;Elements;x", "nodeIds");
    REQUIRE_NOTHROW(readBulk("RotorAxisym;1;Node;50;Through;60")); // a range of eleven points
}

// ---------------------------------------------------------------------------------------------------------------------
// ROTORSE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a ROTORSE names a rotor and the superelement that holds it, and SEOPT is 1 when blank",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "Node;5;;0.;0.;0.\n"
        "Node;6;;0.;0.;1.\n"
        "SuperelementInteriorNodes;1;5\n"
        "SuperelementInteriorNodes;2;6\n"
        "RotorSuperelement;10;1\n"
        "RotorSuperelement;11;2;2\n"
        "RotorSuperelement;10;2;1\n");
    REQUIRE((RotorSuperelementTable::INFO.category == CardCategory::Rotor &&
             RotorSuperelementTable::INFO.idSpace == base::IdSpace::Group));
    const RotorSuperelementCard first = cardAt<RotorSuperelementTable>(model, 0);
    REQUIRE((first.id == 10 && first.superelementId == 1 && first.superelementOption == 1));
    REQUIRE((cardAt<RotorSuperelementTable>(model, 1).superelementOption == 2 &&
             cardAt<RotorSuperelementTable>(model, 1).superelementId == 2));
    REQUIRE(cardAt<RotorSuperelementTable>(model, 2).id == 10); // several cards may share a rotor
    requireRoundTrip(model);

    // The superelement is looked up as a superelement number: the SESET defines 1 and 2.
    REQUIRE(referencesOf<RotorSuperelementTable>(model) == Seens{{"superelementId", ReferenceTarget::Superelement, 1}});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("RotorSuperelement;10;9")) == 1);
    Renumbering::apply(model, RenumberSpace::Superelement, Renumbering::offset(100));
    REQUIRE((cardAt<RotorSuperelementTable>(model, 0).superelementId == 101 &&
             cardAt<RotorSuperelementTable>(model, 1).superelementId == 102));
    REQUIRE(cardAt<RotorSuperelementTable>(model, 0).id == 10); // the rotor number is another space
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("RotorSuperelement;0;1", "id");
    expectInvalid("RotorSuperelement;10", "superelementId");
    expectInvalid("RotorSuperelement;10;0", "superelementId"); // the residual structure has no rotor superelement
    expectInvalid("RotorSuperelement;10;-1", "superelementId");
    expectInvalid("RotorSuperelement;10;1;3", "superelementOption");
    expectInvalid("RotorSuperelement;10;1;0", "superelementOption");
}

// ---------------------------------------------------------------------------------------------------------------------
// RSPINR and RSPINT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an RSPINR gives the relative spin rate as a constant or a DDVAL, and the damping of the rotor",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "RotorRelativeSpin;120;1101;1102;RevolutionsPerMinute;130;;6;0.03;2;4;900.;1400.;1100.\n"
        "RotorRelativeSpin;40;2;1;Frequency;;2.;;0.1\n"
        "RotorRelativeSpin;50;4;5;RevolutionsPerMinute;;1.5\n"
        "RotorRelativeSpin;60;7;8;RevolutionsPerMinute;7\n"
        "RotorRelativeSpin;60;7;8;RevolutionsPerMinute;7\n");
    REQUIRE((RotorRelativeSpinTable::INFO.category == CardCategory::Other &&
             RotorRelativeSpinTable::INFO.idSpace == base::IdSpace::None));
    const RotorRelativeSpinCard example = cardAt<RotorRelativeSpinTable>(model, 0);
    REQUIRE((example.rotorId == 120 && example.nodeA == 1101 && example.nodeB == 1102));
    REQUIRE((example.speedUnit == RotorSpeedUnit::Rpm && example.rotorSuperelementId == 6));
    REQUIRE((example.spinTableId.isTable() && example.spinTableId.table == 130));
    REQUIRE(
        (example.node == Approx(0.03) && example.massDampingFactor == 2.0 && example.stiffnessDampingFactor == 4.0));
    REQUIRE((example.viscousDampingFrequency == 900.0 && example.structuralDampingFrequency == 1400.0 &&
             example.hybridDampingFrequency == 1100.0));
    const RotorRelativeSpinCard constant = cardAt<RotorRelativeSpinTable>(model, 1);
    REQUIRE(constant.speedUnit == RotorSpeedUnit::Freq);
    REQUIRE((!constant.spinTableId.isTable() && constant.spinTableId.constant == 2.0)); // a real is a constant rate
    REQUIRE(
        (constant.node == Approx(0.1) && constant.rotorSuperelementId == 0 && constant.hybridDampingFrequency == 0.0));
    REQUIRE(
        (cardAt<RotorRelativeSpinTable>(model, 2).speedUnit == RotorSpeedUnit::Rpm && // the words are read in any case
         cardAt<RotorRelativeSpinTable>(model, 2).spinTableId.constant == 1.5));
    REQUIRE(cardAt<RotorRelativeSpinTable>(model, 3).spinTableId.table == 7);
    requireRoundTrip(model);
    // The cards have no ID of their own: the same rotor and superelement twice is the business of a check.
    REQUIRE(rowCount<RotorRelativeSpinTable>(model) == 5);

    expectInvalid("RotorRelativeSpin;0;1;2;RevolutionsPerMinute;;1.", "rotorId");
    expectInvalid("RotorRelativeSpin;1;0;2;RevolutionsPerMinute;;1.", "nodeA");
    expectInvalid("RotorRelativeSpin;1;2;0;RevolutionsPerMinute;;1.", "nodeB");
    expectInvalid("RotorRelativeSpin;1;2;3;;;1.", "speedUnit");
    expectInvalid("RotorRelativeSpin;1;2;3;HZ;;1.", "speedUnit");
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute", "spinTableId");
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute;abc", "spinTableId");
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute;0",
                  "spinTableId"); // an integer is the number of a DDVAL
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute;-4", "spinTableId");
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute;;1.;-1", "rotorSuperelementId");
    expectInvalid("RotorRelativeSpin;1;2;3;RevolutionsPerMinute;;1.;;abc", "node");
}

TEST_CASE("an RSPINR looks up its rotor and its grid points only when the rotor is in the residual structure",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "RotorRelativeSpin;120;1101;1102;RevolutionsPerMinute;130;;6\nRotorRelativeSpin;40;2;1;Frequency;;2."
        "\nRotorRelativeSpin;41;2;1;Frequency;"
        "60\n");
    // ROTRSEID 6: the rotor and its grid points are cards of the part of the superelement; the DDVAL is not.
    REQUIRE(referencesOf<RotorRelativeSpinTable>(model, 0) ==
            Seens{{"spinTableId", ReferenceTarget::DiscreteValueSet, 130}});
    REQUIRE(referencesOf<RotorRelativeSpinTable>(model, 1) == Seens{{"rotorId", ReferenceTarget::Rotor, 40},
                                                                    {"nodeA", ReferenceTarget::Node, 2},
                                                                    {"nodeB", ReferenceTarget::Node, 1}});
    REQUIRE(referencesOf<RotorRelativeSpinTable>(model, 2).size() == 4); // ... and the DDVAL of an integer rate

    Model resolved = readBulk(
        "Node;1\nNode;2\nRotorLineNodes;40;1;2\nDesignDiscreteValues;60;100.;200.\nRotorRelativeSpin;40;2;1;Frequency;"
        "60\n");
    REQUIRE(unresolvedIn(resolved) == 0);
    REQUIRE(unresolvedIn(readBulk("RotorRelativeSpin;40;2;1;Frequency;60")) == 4);
    REQUIRE(unresolvedIn(readBulk("RotorRelativeSpin;40;2;1;Frequency;;2.")) == 3);
    REQUIRE(unresolvedIn(readBulk("RotorRelativeSpin;40;2;1;Frequency;;2.;3")) == 0);

    Renumbering::apply(resolved, RenumberSpace::DiscreteValueSet, Renumbering::offset(50));
    REQUIRE(cardAt<RotorRelativeSpinTable>(resolved).spinTableId.table == 110);
    REQUIRE(cardAt<DesignDiscreteValuesTable>(resolved).id == 110);
    Renumbering::apply(resolved, RenumberSpace::Rotor, Renumbering::offset(1000));
    REQUIRE(cardAt<RotorRelativeSpinTable>(resolved).rotorId == 1040);
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<RotorRelativeSpinTable>(resolved).nodeA == 102 &&
             cardAt<RotorRelativeSpinTable>(resolved).nodeB == 101));
    REQUIRE(unresolvedIn(resolved) == 0);
    // A rotor of another superelement keeps its number.
    Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(1000));
    REQUIRE(cardAt<RotorRelativeSpinTable>(model, 0).rotorId == 120);
}

TEST_CASE("an RSPINT gives the spin rate over time as a TABLED1 and an output point", "[model][cards][rotor]")
{
    Model model = readBulk(
        "RotorTransientSpin;120;1101;1102;RevolutionsPerMinute;1105;;;0.02;0.03;0.004;900.;1400.;1100.\n"
        "RotorTransientSpin;101;5;6;Frequency;20;7;3;0.05\n");
    REQUIRE((RotorTransientSpinTable::INFO.category == CardCategory::Other &&
             RotorTransientSpinTable::INFO.idSpace == base::IdSpace::None));
    const RotorTransientSpinCard example = cardAt<RotorTransientSpinTable>(model, 0);
    REQUIRE((example.rotorId == 120 && example.nodeA == 1101 && example.nodeB == 1102));
    REQUIRE((example.speedUnit == RotorSpeedUnit::Rpm && example.spinTableId == 1105 && example.speedOutput == 0));
    REQUIRE((example.rotorSuperelementId == 0 && example.node == Approx(0.02) &&
             example.massDampingFactor == Approx(0.03)));
    REQUIRE((example.stiffnessDampingFactor == Approx(0.004) && example.viscousDampingFrequency == 900.0 &&
             example.structuralDampingFrequency == 1400.0));
    REQUIRE(example.hybridDampingFrequency == 1100.0);
    const RotorTransientSpinCard second = cardAt<RotorTransientSpinTable>(model, 1);
    REQUIRE((second.speedUnit == RotorSpeedUnit::Freq && second.spinTableId == 20 && second.speedOutput == 7));
    REQUIRE((second.rotorSuperelementId == 3 && second.node == Approx(0.05)));
    requireRoundTrip(model);

    // The TABLED1 and the EPOINT are looked up whatever the superelement; the rotor and the grid points only in the
    // residual structure.
    REQUIRE(referencesOf<RotorTransientSpinTable>(model, 0) ==
            Seens{{"rotorId", ReferenceTarget::Rotor, 120},
                  {"nodeA", ReferenceTarget::Node, 1101},
                  {"nodeB", ReferenceTarget::Node, 1102},
                  {"spinTableId", ReferenceTarget::DynamicTable, 1105}});
    REQUIRE(referencesOf<RotorTransientSpinTable>(model, 1) ==
            Seens{{"spinTableId", ReferenceTarget::DynamicTable, 20}, {"speedOutput", ReferenceTarget::Node, 7}});
    Model resolved = readBulk(
        "Node;5\n"
        "Node;6\n"
        "ExtraPoint;7\n"
        "RotorLineNodes;101;5;6\n"
        "TableDynamic1;20;;;0.;1.;1.;2.;EndTable\n"
        "RotorTransientSpin;101;5;6;Frequency;20;7\n");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<RotorTransientSpinTable>(resolved).spinTableId == 120);
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE((cardAt<RotorTransientSpinTable>(resolved).speedOutput == 1007 &&
             cardAt<RotorTransientSpinTable>(resolved).nodeA == 1005));
    REQUIRE(unresolvedIn(resolved) == 0);

    expectInvalid("RotorTransientSpin;0;1;2;RevolutionsPerMinute;5", "rotorId");
    expectInvalid("RotorTransientSpin;1;0;2;RevolutionsPerMinute;5", "nodeA");
    expectInvalid("RotorTransientSpin;1;2;0;RevolutionsPerMinute;5", "nodeB");
    expectInvalid("RotorTransientSpin;1;2;3;;5", "speedUnit");
    expectInvalid("RotorTransientSpin;1;2;3;RevolutionsPerMinute", "spinTableId");
    expectInvalid("RotorTransientSpin;1;2;3;RevolutionsPerMinute;1.5", "spinTableId"); // an integer: a TABLED1
    expectInvalid("RotorTransientSpin;1;2;3;RevolutionsPerMinute;0", "spinTableId");
    expectInvalid("RotorTransientSpin;1;2;3;RevolutionsPerMinute;5;-1", "speedOutput");
    expectInvalid("RotorTransientSpin;1;2;3;RevolutionsPerMinute;5;;-1", "rotorSuperelementId");
}

// ---------------------------------------------------------------------------------------------------------------------
// REFROT and REFRTSE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a REFROT names the reference rotor and the rotors that follow it", "[model][cards][rotor]")
{
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "RotorLineNodes;10;1;2\n"
        "RotorLineNodes;20;1;2\n"
        "RotorLineNodes;30;1;2\n"
        "RotorLineNodes;40;1;2\n"
        "ReferenceRotor;100;10;20;37;3;4;65;86;97;4102;110\n"
        "ReferenceRotor;100;10;20;Through;40;By;10\n"
        "ReferenceRotor;101;5;6\n"
        "ReferenceRotor;102;10;20;Through;30\n");
    REQUIRE((ReferenceRotorTable::INFO.category == CardCategory::Other &&
             ReferenceRotorTable::INFO.idSpace == base::IdSpace::Group));
    const ReferenceRotorCard example = cardAt<ReferenceRotorTable>(model, 0);
    REQUIRE((example.id == 100 && example.referenceRotorId == 10));
    REQUIRE(example.rotors.size() == 9);
    REQUIRE((example.rotors.front() == ContactRun{20, 20, 1} && example.rotors.back() == ContactRun{110, 110, 1}));
    REQUIRE(cardAt<ReferenceRotorTable>(model, 1).id == 100); // several cards of one RGYRO set
    REQUIRE(cardAt<ReferenceRotorTable>(model, 1).rotors == Runs{ContactRun{20, 40, 10}});
    REQUIRE(cardAt<ReferenceRotorTable>(model, 3).rotors == Runs{ContactRun{20, 30, 1}}); // BY is 1 when blank
    requireRoundTrip(model);

    REQUIRE(referencesOf<ReferenceRotorTable>(model, 1) == Seens{{"referenceRotorId", ReferenceTarget::Rotor, 10},
                                                                 {"nonFollowingRotorIds", ReferenceTarget::Rotor, 20},
                                                                 {"nonFollowingRotorIds", ReferenceTarget::Rotor, 40}});
    REQUIRE(referencesOf<ReferenceRotorTable>(model, 0).size() == 10);
    // The RID is the number of an RGYRO set: it is not looked up.
    REQUIRE(unresolvedIn(readBulk(
                "Node;1\nNode;2\nRotorLineNodes;10;1;2\nRotorLineNodes;20;1;2\nReferenceRotor;999;10;20")) == 0);
    REQUIRE(unresolvedIn(readBulk("ReferenceRotor;100;10;20")) == 2);

    Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(1000));
    REQUIRE(cardAt<ReferenceRotorTable>(model, 1).referenceRotorId == 1010);
    REQUIRE(cardAt<ReferenceRotorTable>(model, 1).rotors == Runs{ContactRun{1020, 1040, 10}}); // the increment is kept
    REQUIRE(cardAt<ReferenceRotorTable>(model, 1).id == 100);
    REQUIRE(cardAt<RotorLineNodesTable>(model, 0).id == 1010);

    expectInvalid("ReferenceRotor;0;10;20", "id");
    expectInvalid("ReferenceRotor;100;0;20", "referenceRotorId");
    expectInvalid("ReferenceRotor;100", "referenceRotorId");
    expectInvalid("ReferenceRotor;100;10", "nonFollowingRotorIds");
    expectInvalid("ReferenceRotor;100;10;0", "nonFollowingRotorIds");
    expectInvalid("ReferenceRotor;100;10;30;Through;20", "nonFollowingRotorIds");
    expectInvalid("ReferenceRotor;100;10;20;Through;40;By;0", "By");
    expectInvalid("ReferenceRotor;100;10;x", "nonFollowingRotorIds");
}

TEST_CASE("a REFRTSE names the reference rotor and the rotors of any superelement that follow it",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "RotorLineNodes;10;1;2\n"
        "RotorLineNodes;2;1;2\n"
        "RotorLineNodes;37;1;2\n"
        "RotorLineNodes;86;1;2\n"
        "SuperelementReferenceRotor;100;10;20;37;1;2;0;86;10;4102;110\n"
        "SuperelementReferenceRotor;101;10;;37;;86\n"
        "SuperelementReferenceRotor;101;5;;6\n");
    REQUIRE((SuperelementReferenceRotorTable::INFO.category == CardCategory::Other &&
             SuperelementReferenceRotorTable::INFO.idSpace == base::IdSpace::Group));
    const SuperelementReferenceRotorCard example = cardAt<SuperelementReferenceRotorTable>(model, 0);
    REQUIRE((example.id == 100 && example.referenceRotorId == 10 && example.referenceSuperelementId == 20));
    // The third pair is cut by the end of the line: its rotor is in field 8 and its superelement in field 9.
    REQUIRE(example.rotors == std::vector<RotorInSuperelement>{{37, 1}, {2, 0}, {86, 10}, {4102, 110}});
    const SuperelementReferenceRotorCard blanks = cardAt<SuperelementReferenceRotorTable>(model, 1);
    REQUIRE(
        (blanks.referenceSuperelementId == 0 && blanks.rotors == std::vector<RotorInSuperelement>{{37, 0}, {86, 0}}));
    requireRoundTrip(model);

    // Only the rotors of the residual structure are looked up.
    REQUIRE(referencesOf<SuperelementReferenceRotorTable>(model, 0) ==
            Seens{{"nonFollowingRotorIds", ReferenceTarget::Rotor, 2}});
    REQUIRE(referencesOf<SuperelementReferenceRotorTable>(model, 1) ==
            Seens{{"referenceRotorId", ReferenceTarget::Rotor, 10},
                  {"nonFollowingRotorIds", ReferenceTarget::Rotor, 37},
                  {"nonFollowingRotorIds", ReferenceTarget::Rotor, 86}});
    REQUIRE(unresolvedIn(model) == 2); // the rotors 5 and 6 of the last card
    Renumbering::apply(model, RenumberSpace::Rotor, Renumbering::offset(1000));
    REQUIRE(cardAt<SuperelementReferenceRotorTable>(model, 0).referenceRotorId ==
            10); // in the superelement 20: not renumbered
    REQUIRE(cardAt<SuperelementReferenceRotorTable>(model, 0).rotors ==
            std::vector<RotorInSuperelement>{{37, 1}, {1002, 0}, {86, 10}, {4102, 110}});
    REQUIRE(cardAt<SuperelementReferenceRotorTable>(model, 1).referenceRotorId == 1010);
    REQUIRE(cardAt<SuperelementReferenceRotorTable>(model, 1).id == 101);

    expectInvalid("SuperelementReferenceRotor;0;10;;5", "id");
    expectInvalid("SuperelementReferenceRotor;100;0;;5", "referenceRotorId");
    expectInvalid("SuperelementReferenceRotor;100;10;-1;5", "referenceSuperelementId");
    expectInvalid("SuperelementReferenceRotor;100;10", "nonFollowingRotorIds");
    expectInvalid("SuperelementReferenceRotor;100;10;;0", "nonFollowingRotorIds");
    expectInvalid("SuperelementReferenceRotor;100;10;;;3", "nonFollowingRotorIds"); // a superelement with no rotor
    expectInvalid("SuperelementReferenceRotor;100;10;;5;-2", "nonFollowingSuperelementIds");
    expectInvalid("SuperelementReferenceRotor;100;10;;x", "nonFollowingRotorIds");
}

// ---------------------------------------------------------------------------------------------------------------------
// ROTBENT
// ---------------------------------------------------------------------------------------------------------------------

namespace
{

// Two cards: the first with a vector, a kink and three pairs of grid points, the second with offsets and kinks of
// four rows each, the bearings and the vector last.
const std::string ROTOR_INITIAL_BEND_TEXT =
    "RotorInitialBend;103;10;97;UnbalanceFrame;1.;0.;0.;KinkDeformation;1;25.;0.05;120.;BearingDeflectionProfile;3;1;"
    "121;41;141;71;171\n"
    "RotorInitialBend;210;10;1;Offset;4;4.15;0.001;20.;16.75;0.002;110.;27.25;0.003;200.;36.85;0.004;290.;"
    "KinkDeformation;4;11.25;"
    "0.05;40.;23.75;0.1;130.;34.25;0.15;220.;46.75;0.2;310.;BearingDeflectionProfile;3;13;113;19;119;37;137;"
    "UnbalanceFrame;0.;-1.;2.\n";

} // namespace

TEST_CASE(
    "a ROTBENT has a row for the plane vector, one for each kink and offset and one for each pair of bearing grids",
    "[model][cards][rotor]")
{
    Model model = readBulk(ROTOR_INITIAL_BEND_TEXT + "RotorInitialBend;300;10;2;BearingDeflectionProfile;1;1;2\n");
    REQUIRE((RotorInitialBendTable::INFO.category == CardCategory::Other &&
             RotorInitialBendTable::INFO.idSpace == base::IdSpace::PerCardType));
    const RotorInitialBendCard first = cardAt<RotorInitialBendTable>(model, 0);
    REQUIRE((first.id == 103 && first.rotorId == 10 && first.loadCombinationId == 97));
    REQUIRE(first.planeVector.has_value());
    REQUIRE((first.planeVector->components == std::array<double, 3>{1.0, 0.0, 0.0}));
    REQUIRE(first.kinks == std::vector<RotorKink>{{25.0, 0.05, 120.0}});
    REQUIRE(first.offsets.empty());
    REQUIRE(first.bearings == std::vector<RotorBearingPair>{{1, 121}, {41, 141}, {71, 171}});
    const RotorInitialBendCard second = cardAt<RotorInitialBendTable>(model, 1);
    REQUIRE((second.id == 210 && second.loadCombinationId == 1));
    REQUIRE(second.offsets.size() == 4);
    REQUIRE(
        (second.offsets[0] == RotorOffset{4.15, 0.001, 20.0} && second.offsets[3] == RotorOffset{36.85, 0.004, 290.0}));
    REQUIRE(second.kinks.size() == 4);
    REQUIRE((second.kinks[1].station == Approx(23.75) && second.kinks[1].angle == Approx(0.1)));
    REQUIRE(second.bearings.size() == 3);
    REQUIRE((second.planeVector.has_value() && second.planeVector->components[1] == -1.0)); // the vector may come last
    const RotorInitialBendCard minimal = cardAt<RotorInitialBendTable>(model, 2);
    REQUIRE((!minimal.planeVector.has_value() && minimal.kinks.empty() && minimal.offsets.empty()));
    requireRoundTrip(model);
    // The keywords may come in any order: the card is the same, and it is written in one order.
    const Model reordered = readBulk(
        "RotorInitialBend;5;10;1;KinkDeformation;1;1.;2.;3.;Offset;1;4.;5.;6.;BearingDeflectionProfile;1;1;2\n");
    const Model swapped = readBulk(
        "RotorInitialBend;5;10;1;BearingDeflectionProfile;1;1;2;Offset;1;4.;5.;6.;KinkDeformation;1;1.;2.;3.\n");
    REQUIRE(cardAt<RotorInitialBendTable>(reordered) == cardAt<RotorInitialBendTable>(swapped));
    oor::testing::requireModelFileRoundTrip(reordered);

    // The rotor, the load set and the grid points of the pairs.
    REQUIRE(referencesOf<RotorInitialBendTable>(model, 2) == Seens{{"rotorId", ReferenceTarget::Rotor, 10},
                                                                   {"loadId", ReferenceTarget::LoadCombinationSet, 2},
                                                                   {"rotorNode", ReferenceTarget::Node, 1},
                                                                   {"statorNode", ReferenceTarget::Node, 2}});
    REQUIRE(referencesOf<RotorInitialBendTable>(model, 0).size() == 8);
    Model resolved = readBulk(
        "Node;1\nNode;2\nRotorLineNodes;10;1;2\nPointForceLoad;2;1;;1.;0.;0.;1.\nRotorInitialBend;300;10;2;"
        "BearingDeflectionProfile;1;1;"
        "2\n");
    REQUIRE(unresolvedIn(resolved) == 0);
    REQUIRE(unresolvedIn(readBulk("RotorInitialBend;300;10;2;BearingDeflectionProfile;1;1;2")) == 4);
    Renumbering::apply(resolved, RenumberSpace::Rotor, Renumbering::offset(1000));
    Renumbering::apply(resolved, RenumberSpace::LoadCombinationSet, Renumbering::offset(100));
    Renumbering::apply(resolved, RenumberSpace::Node, Renumbering::offset(10));
    const RotorInitialBendCard moved = cardAt<RotorInitialBendTable>(resolved);
    REQUIRE((moved.rotorId == 1010 && moved.loadCombinationId == 102 && moved.id == 300));
    REQUIRE(moved.bearings == std::vector<RotorBearingPair>{{11, 12}});
    REQUIRE(unresolvedIn(resolved) == 0);
}

TEST_CASE("a ROTBENT refuses a row, a keyword or a value the format reference does not allow", "[model][cards][rotor]")
{
    expectInvalid("RotorInitialBend;0;10;99;BearingDeflectionProfile;1;1;2", "bendTableId");
    expectInvalid("RotorInitialBend;1;0;99;BearingDeflectionProfile;1;1;2", "rotorId");
    expectInvalid("RotorInitialBend;1;10;0;BearingDeflectionProfile;1;1;2", "loadId");
    expectInvalid("RotorInitialBend;1;10;99;KinkDeformation;1;1.;2.;3.",
                  "rotorNode"); // the paired grid points are required
    expectInvalid(
        "RotorInitialBend;1;10;99;KinkDeformation;1;1.;2.;3.;KinkDeformation;1;4.;5.;6.;BearingDeflectionProfile;1;1;2",
        "lineKeyword"); // a keyword is given once
    expectInvalid("RotorInitialBend;1;10;99;KinkDeformation;1;1.;2.;;BearingDeflectionProfile;1;1;2", "phase");
    expectInvalid("RotorInitialBend;1;10;99;KinkDeformation;1;1.;abc;3.;BearingDeflectionProfile;1;1;2", "angle");
    expectInvalid("RotorInitialBend;1;10;99;BearingDeflectionProfile;1;1", "statorNode");
    expectInvalid("RotorInitialBend;1;10;99;BearingDeflectionProfile;1;0;2", "rotorNode");
}

// ---------------------------------------------------------------------------------------------------------------------
// CAMPBLL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CAMPBLL has its defaults for the tracking of the modes and keeps a blank VPARM and TYPE blank",
          "[model][cards][rotor]")
{
    Model model = readBulk(
        "CampbellDiagram;14;Speed;26;RevolutionsPerMinute;1;0.8;1;;1\n"
        "CampbellDiagram;16;Speed;5;Frequency\n"
        "CampbellDiagram;17;Speed;5;RevolutionsPerMinute;2;0.9;;12\n"
        "CampbellDiagram;18;;5\n");
    REQUIRE((CampbellDiagramTable::INFO.category == CardCategory::Other &&
             CampbellDiagramTable::INFO.idSpace == base::IdSpace::PerCardType));
    const CampbellDiagramCard example = cardAt<CampbellDiagramTable>(model, 0);
    REQUIRE((example.id == 14 && example.varyingParameter == "Speed" && example.discreteValuesId == 26));
    REQUIRE(example.type == RotorSpeedUnit::Rpm);
    REQUIRE((example.modeTrackingMethod == 1 && example.correlationThreshold == Approx(0.8) &&
             example.trackingSwitch == 1 && example.printCorrelation == 1));
    REQUIRE_FALSE(example.trackedModeCount.has_value());
    const CampbellDiagramCard defaults = cardAt<CampbellDiagramTable>(model, 1);
    REQUIRE((defaults.type == RotorSpeedUnit::Freq && defaults.modeTrackingMethod == 0 &&
             defaults.correlationThreshold == Approx(0.7)));
    REQUIRE((defaults.trackingSwitch == 0 && defaults.printCorrelation == 0 && !defaults.trackedModeCount.has_value()));
    const CampbellDiagramCard modes = cardAt<CampbellDiagramTable>(model, 2);
    REQUIRE((modes.varyingParameter == "Speed" && modes.type == RotorSpeedUnit::Rpm));
    REQUIRE(
        (modes.modeTrackingMethod == 2 && modes.correlationThreshold == Approx(0.9) && modes.trackedModeCount == 12));
    const CampbellDiagramCard blank = cardAt<CampbellDiagramTable>(model, 3);
    REQUIRE((blank.varyingParameter.empty() && !blank.type.has_value() && blank.discreteValuesId == 5));
    requireRoundTrip(model);
    // The defaults are written as blanks: the card of defaults has one line.
    const std::string text = writeModelText(readBulk("CampbellDiagram;16;Speed;5;Frequency"));
    REQUIRE(text.find(".7") == std::string::npos);
    REQUIRE_THROWS_AS(
        readBulk("CampbellDiagram;1;Speed;5;RevolutionsPerMinute\nCampbellDiagram;1;Speed;6;RevolutionsPerMinute"),
        base::DeckError);

    REQUIRE(referencesOf<CampbellDiagramTable>(model) ==
            Seens{{"discreteValuesId", ReferenceTarget::DiscreteValueSet, 26}});
    REQUIRE(unresolvedIn(model) == 4); // the DDVAL cards 26 and 5 (three cards name 5): none exists
    Model resolved = readBulk("DesignDiscreteValues;26;500.;1.E+3\nCampbellDiagram;14;Speed;26;RevolutionsPerMinute\n");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::DiscreteValueSet, Renumbering::offset(100));
    REQUIRE(cardAt<CampbellDiagramTable>(resolved).discreteValuesId == 126);
    REQUIRE(cardAt<CampbellDiagramTable>(resolved).id == 14);

    expectInvalid("CampbellDiagram;0;Speed;5;RevolutionsPerMinute", "id");
    expectInvalid("CampbellDiagram;1;ROTOR;5;RevolutionsPerMinute", "varyingParameter");
    expectInvalid("CampbellDiagram;1;Speed;0;RevolutionsPerMinute", "discreteValuesId");
    expectInvalid("CampbellDiagram;1;Speed", "discreteValuesId");
    expectInvalid("CampbellDiagram;1;Speed;5;HZ", "speedUnit");
    expectInvalid("CampbellDiagram;1;Speed;5;RevolutionsPerMinute;3", "modeTrackingMethod");
    expectInvalid("CampbellDiagram;1;Speed;5;RevolutionsPerMinute;;abc", "correlationThreshold");
    expectInvalid("CampbellDiagram;1;Speed;5;RevolutionsPerMinute;;;2", "trackingSwitch");
    expectInvalid("CampbellDiagram;1;Speed;5;RevolutionsPerMinute;;;;0", "trackedModeCount");
    expectInvalid("CampbellDiagram;1;Speed;5;RevolutionsPerMinute;;;;;2", "printCorrelation");
}

// ---------------------------------------------------------------------------------------------------------------------
// DDVAL and the DESVAR that names it
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a DDVAL lists values and THRU BY ranges, which it keeps as written and expands in order",
          "[model][cards][design]")
{
    Model model = readBulk(
        "DesignDiscreteValues;120;0.15;0.25;0.35;0.55;0.65;0.45;;0.75;Through;1.5;By;0.25;;;;1.6;2.4\n"
        "DesignDiscreteValues;111;1.;Through;0.;By;-0.25\n"
        "DesignDiscreteValues;112;5.\n"
        "DesignDiscreteValues;113;0.;Through;1.;By;0.3\n");
    REQUIRE((DesignDiscreteValuesTable::INFO.category == CardCategory::DiscreteValueSet &&
             DesignDiscreteValuesTable::INFO.idSpace == base::IdSpace::PerCardType));
    const DesignDiscreteValuesCard example = cardAt<DesignDiscreteValuesTable>(model, 0);
    REQUIRE(example.id == 120);
    REQUIRE(example.values.size() == 9);
    REQUIRE((!example.values[0].isRange() && example.values[0].first == 0.15 && example.values[5].first == 0.45));
    REQUIRE((example.values[6].isRange() && example.values[6].first == 0.75 && example.values[6].last == 1.5));
    REQUIRE(example.values[6].increment == 0.25);
    // The expansion: the six values, 0.75 to 1.5 by 0.25 (four values, the last one always included), the two after.
    const std::vector<double> values = designDiscreteValuesValues(example);
    REQUIRE(values.size() == 12);
    REQUIRE((values[5] == 0.45 && values[6] == Approx(0.75) && values[7] == Approx(1.0) && values[9] == 1.5));
    REQUIRE((values[10] == 1.6 && values[11] == 2.4)); // not sorted: the values keep the order of the card
    // A range may go down (a negative increment).
    REQUIRE(designDiscreteValuesValues(cardAt<DesignDiscreteValuesTable>(model, 1)) ==
            std::vector<double>{1.0, 0.75, 0.5, 0.25, 0.0});
    REQUIRE(designDiscreteValuesValues(cardAt<DesignDiscreteValuesTable>(model, 2)) == std::vector<double>{5.0});
    // A range that is not evenly divisible by its increment still ends at its last value.
    const std::vector<double> uneven = designDiscreteValuesValues(cardAt<DesignDiscreteValuesTable>(model, 3));
    REQUIRE(uneven.size() == 5);
    REQUIRE((uneven[1] == Approx(0.3) && uneven[3] == Approx(0.9) && uneven.back() == 1.0));
    requireRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("DesignDiscreteValues;1;1.\nDesignDiscreteValues;1;2."), base::DeckError);
    // The DDVAL names nothing.
    REQUIRE(referencesOf<DesignDiscreteValuesTable>(model).empty());
    // Values over several lines come back, and a range that does not fit in the end of a line goes to the next one.
    const Model spreadList = readBulk("DesignDiscreteValues;114;1.;2.;3.;4.;5.;6.;7.;10.;Through;20.;By;2.;30.\n");
    REQUIRE(cardAt<DesignDiscreteValuesTable>(spreadList).values.size() == 9);
    requireRoundTrip(spreadList);
    const Model late = readBulk("DesignDiscreteValues;115;1.;2.;3.;4.;5.;;;10.;Through;20.;By;2.");
    REQUIRE(cardAt<DesignDiscreteValuesTable>(late).values.size() == 6);
    requireRoundTrip(late);

    expectInvalid("DesignDiscreteValues;0;1.", "id");
    expectInvalid("DesignDiscreteValues;1", "values");
    expectInvalid("DesignDiscreteValues;1;abc", "values");
    expectInvalid("DesignDiscreteValues;1;Through;2.", "values");
    expectInvalid("DesignDiscreteValues;1;1.;By;2.", "values");
    expectInvalid("DesignDiscreteValues;1;1.;Through", "through");
    expectInvalid("DesignDiscreteValues;1;1.;Through;2.", "stepKeyword"); // the format draws no range without BY
    expectInvalid("DesignDiscreteValues;1;1.;Through;2.;By", "stepKeyword");
    expectInvalid("DesignDiscreteValues;1;1.;Through;2.;By;0.", "increment");
    expectInvalid("DesignDiscreteValues;1;1.;Through;2.;By;-0.5",
                  "increment"); // the increment goes the way the range does
    expectInvalid("DesignDiscreteValues;1;2.;Through;1.;By;0.5", "increment");
    expectInvalid("DesignDiscreteValues;1;0.;Through;1.;By;1.E-9", "increment"); // a billion values
}

TEST_CASE("a DESVAR names the DDVAL that lists its discrete values", "[model][cards][design]")
{
    Model model = readBulk("DesignVar;1;A;1.;;;;7\nDesignVar;2;B;1.\nDesignDiscreteValues;7;1.;2.;3.\n");
    REQUIRE(referencesOf<DesignVarTable>(model, 0) ==
            Seens{{"discreteValuesId", ReferenceTarget::DiscreteValueSet, 7}});
    REQUIRE(referencesOf<DesignVarTable>(model, 1).empty()); // a continuous variable
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(readBulk("DesignVar;1;A;1.;;;;7")) == 1);
    Renumbering::apply(model, RenumberSpace::DiscreteValueSet, Renumbering::offset(100));
    REQUIRE(cardAt<DesignVarTable>(model, 0).discreteValuesId == 107);
    REQUIRE(cardAt<DesignDiscreteValuesTable>(model).id == 107);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// BEADVAR
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a BEADVAR has a property, the size of a bead, and optional DESVAR and GRID lines", "[model][cards][design]")
{
    Model model =
        readBulk(NODES_TEXT +
                 "IsoMat;1;2.1E+11;;0.3\n"
                 "ShellProp;101;1;0.1\n"
                 "RectFrame2;5\n"
                 "IdListSet1;100;1;2\n"
                 "IdListSet1;200;3\n"
                 "DesignBeadRegion;10;ShellProp;101;12.;18.;65.\n"
                 "DesignBeadRegion;11;ShellProp;101;12.;18.;65.;No;None;DesignVar;2.;3.;4.;5;-1.;1.;0.1;Nodes;100;200\n"
                 "DesignBeadRegion;12;ShearPanelProp;101;1.;2.;45.;;;Nodes;100;;DesignVar\n"
                 "DesignBeadRegion;13;CompProp;101;1.;1.;45.;;;DesignVar;ElementNormals;;;;0.;2.\n");
    REQUIRE((DesignBeadRegionTable::INFO.category == CardCategory::Other &&
             DesignBeadRegionTable::INFO.idSpace == base::IdSpace::PerCardType));
    const DesignBeadRegionCard norm = cardAt<DesignBeadRegionTable>(model, 0);
    REQUIRE((norm.id == 10 && norm.propType == "ShellProp" && norm.propId == 101));
    REQUIRE((norm.minWidth == 12.0 && norm.maxHeight == 18.0 && norm.angle == 65.0));
    REQUIRE((norm.bufferZone == BeadBuffer::Yes && norm.skippedNodes == BeadSkip::Both)); // the defaults
    REQUIRE((!norm.design.has_value() && !norm.grids.has_value()));
    const DesignBeadRegionCard full = cardAt<DesignBeadRegionTable>(model, 1);
    REQUIRE((full.bufferZone == BeadBuffer::No && full.skippedNodes == BeadSkip::None));
    REQUIRE(full.design.has_value());
    REQUIRE(full.design->direction == std::array<double, 3>{2.0, 3.0, 4.0});
    REQUIRE((full.design->frameId == 5 && full.design->lowerBound == -1.0 && full.design->upperBound == 1.0));
    REQUIRE(full.design->designVarMoveLimit == Approx(0.1));
    REQUIRE((full.grids.has_value() && full.grids->excludedNodeSetId == 100 && full.grids->addedNodeSetId == 200));
    // The lines come in either order; a DESVAR line with nothing on it has the defaults: the normals, XLB 0, XUB 1.
    const DesignBeadRegionCard reversed = cardAt<DesignBeadRegionTable>(model, 2);
    REQUIRE(reversed.propType == "ShearPanelProp");
    REQUIRE((reversed.grids.has_value() && reversed.grids->excludedNodeSetId == 100 &&
             reversed.grids->addedNodeSetId == 0));
    REQUIRE(reversed.design.has_value());
    REQUIRE((!reversed.design->direction.has_value() && reversed.design->frameId == 0 &&
             reversed.design->lowerBound == 0.0));
    REQUIRE((reversed.design->upperBound == 1.0 && reversed.design->designVarMoveLimit == Approx(0.2)));
    // The word NORM is the same as a blank.
    const DesignBeadRegionCard named = cardAt<DesignBeadRegionTable>(model, 3);
    REQUIRE((!named.design->direction.has_value() && named.design->upperBound == 2.0));
    requireRoundTrip(model);
    REQUIRE(unresolvedIn(model) == 0);

    REQUIRE(referencesOf<DesignBeadRegionTable>(model, 0) == Seens{{"propId", ReferenceTarget::Property, 101}});
    REQUIRE(referencesOf<DesignBeadRegionTable>(model, 1) == Seens{{"propId", ReferenceTarget::Property, 101},
                                                                   {"frameId", ReferenceTarget::Coordinate, 5},
                                                                   {"excludedNodeSetId", ReferenceTarget::Set, 100},
                                                                   {"addedNodeSetId", ReferenceTarget::Set, 200}});
    REQUIRE(referencesOf<DesignBeadRegionTable>(model, 2) ==
            Seens{{"propId", ReferenceTarget::Property, 101}, {"excludedNodeSetId", ReferenceTarget::Set, 100}});
    REQUIRE(unresolvedIn(readBulk("DesignBeadRegion;10;ShellProp;101;1.;1.;45.")) == 1);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(50));
    const DesignBeadRegionCard moved = cardAt<DesignBeadRegionTable>(model, 1);
    REQUIRE((moved.propId == 1101 && moved.design->frameId == 15 && moved.grids->excludedNodeSetId == 150 &&
             moved.grids->addedNodeSetId == 250));
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("a BEADVAR refuses a layout or a value the format reference does not allow", "[model][cards][design]")
{
    expectInvalid("DesignBeadRegion;0;ShellProp;101;1.;1.;45.", "id");
    expectInvalid("DesignBeadRegion;1;BarProp;101;1.;1.;45.", "propType");
    expectInvalid("DesignBeadRegion;1;;101;1.;1.;45.", "propType");
    expectInvalid("DesignBeadRegion;1;ShellProp;0;1.;1.;45.", "propId");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;0.;1.;45.", "minWidth");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;0.;45.", "maxHeight");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;0.", "angle");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;90.", "angle");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.", "angle");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;MAYBE", "bufferZone");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;SOMETIMES", "skippedNodes");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;1.;2.",
                  "directionZ"); // a vector has three components
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;ElementNormals;2.",
                  "directionY");                                                                     // and NORM none
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;;;;;2.;1.", "lowerBound"); // XLB below XUB
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;;;;;;;0.", "designVarMoveLimit");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;;;;-1", "frameId");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;Nodes;-1", "excludedNodeSetId");
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;Nodes;1;;Nodes;2", "lineKeyword"); // each line once
    expectInvalid("DesignBeadRegion;1;ShellProp;101;1.;1.;45.;;;DesignVar;;;;;;;;DesignVar", "lineKeyword");
}

// ---------------------------------------------------------------------------------------------------------------------
// TOMVAR
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TOMVAR resolves its bounds from XINIT and has a line for each of its optional parts",
          "[model][cards][design]")
{
    Model model = readBulk(
        "DesignTopometryRegion;1;ShellProp;5;thickness;8.;;;;Stress;6.\n"
        "DesignTopometryRegion;10;ShellProp;100;e;2.E+5;1.;2.E+6\n"
        "DesignTopometryRegion;11;BarProp;7;area\n"
        "DesignTopometryRegion;12;ShellProp;9;thickness;2.;;;0.3;DesignVarLink;1;0.5;2.;DesignDiscreteValues;50;Stress;"
        "100.;"
        "Symmetry;3;PlaneXy;PlaneYz;;ZDirection;4;11;;2;12;"
        "13\n"
        "DesignTopometryRegion;13;SolidProp;9;e;;;;;Symmetry;;PlaneXy;;;;;1;;8;2;3;4;5;6;7;8;9\n");
    REQUIRE((DesignTopometryRegionTable::INFO.category == CardCategory::Other &&
             DesignTopometryRegionTable::INFO.idSpace == base::IdSpace::PerCardType));
    const DesignTopometryRegionCard thickness = cardAt<DesignTopometryRegionTable>(model, 0);
    REQUIRE((thickness.id == 1 && thickness.kind == "ShellProp" && thickness.propId == 5 &&
             thickness.field.name == "thickness"));
    // A blank XLB is 0.5 and a blank XUB 1.5 times XINIT (the format): stored resolved, so they are written.
    REQUIRE((thickness.initialValue == 8.0 && thickness.lowerBound == 4.0 && thickness.upperBound == 12.0));
    REQUIRE((thickness.designVarMoveLimit == 0.5 && thickness.stressLimit == 6.0)); // a blank DELXV is 0.5
    REQUIRE((!thickness.link.has_value() && thickness.discreteValuesId == 0 && !thickness.symmetry.has_value()));
    const DesignTopometryRegionCard modulus = cardAt<DesignTopometryRegionTable>(model, 1);
    REQUIRE((modulus.initialValue == 2.0e5 && modulus.lowerBound == 1.0 && modulus.upperBound == 2.0e6)); // given: kept
    const DesignTopometryRegionCard byPosition = cardAt<DesignTopometryRegionTable>(model, 2);
    REQUIRE((byPosition.field.name == "area"));
    REQUIRE((!byPosition.initialValue.has_value() && !byPosition.lowerBound.has_value() &&
             !byPosition.upperBound.has_value()));
    const DesignTopometryRegionCard every = cardAt<DesignTopometryRegionTable>(model, 3);
    REQUIRE((every.kind == "ShellProp" && every.field.name == "thickness")); // the neutral field name
    REQUIRE((every.initialValue == 2.0 && every.lowerBound == 1.0 && every.upperBound == 3.0 &&
             every.designVarMoveLimit == Approx(0.3)));
    REQUIRE(every.link.has_value());
    REQUIRE((every.link->tableId == 1 && every.link->constant0 == 0.5 && every.link->coefficient1 == 2.0));
    REQUIRE((every.discreteValuesId == 50 && every.stressLimit == 100.0));
    REQUIRE(every.symmetry.has_value());
    REQUIRE((every.symmetry->frameId == 3 && every.symmetry->mirrorPlanes[0] == "PlaneXy" &&
             every.symmetry->mirrorPlanes[1] == "PlaneYz"));
    REQUIRE((every.symmetry->mirrorPlanes[2].empty() && every.symmetry->cyclicAxis == "ZDirection" &&
             every.symmetry->cyclicSegments == 4));
    REQUIRE(every.symmetry->symmetricProperties == std::vector<base::EntityId>{11, 12, 13});
    // The properties go on over the lines after the SYM line, seven to a line.
    REQUIRE(cardAt<DesignTopometryRegionTable>(model, 4).symmetry->symmetricProperties ==
            std::vector<base::EntityId>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    requireRoundTrip(model);
    // The resolved bounds are written: the second field after XINIT is not blank.
    const std::string text = writeModelText(readBulk("DesignTopometryRegion;1;ShellProp;5;thickness;8."));
    REQUIRE(text.find(";thickness;8.;4.;12.") != std::string::npos);
    // A DELXV that is the default is a blank, one that is not stays.
    REQUIRE(writeModelText(readBulk("DesignTopometryRegion;1;ShellProp;5;thickness;;;;0.5")).find(".5") ==
            std::string::npos);
    REQUIRE(writeModelText(readBulk("DesignTopometryRegion;1;ShellProp;5;thickness;;;;0.25")).find(".25") !=
            std::string::npos);

    REQUIRE(referencesOf<DesignTopometryRegionTable>(model, 0) == Seens{{"propId", ReferenceTarget::Property, 5}});
    REQUIRE(referencesOf<DesignTopometryRegionTable>(model, 3) ==
            Seens{{"propId", ReferenceTarget::Property, 9},
                  {"discreteValuesId", ReferenceTarget::DiscreteValueSet, 50},
                  {"frameId", ReferenceTarget::Coordinate, 3},
                  {"symmetryPropId", ReferenceTarget::Property, 11},
                  {"symmetryPropId", ReferenceTarget::Property, 12},
                  {"symmetryPropId", ReferenceTarget::Property, 13}});
    // The TID of a DLINK line is a TOMVAR: a plain number.
    Model world = readBulk(
        "IsoMat;1;2.1E+11;;0.3\n"
        "ShellProp;9;1;0.1\n"
        "ShellProp;11;1;0.1\n"
        "RectFrame2;3\n"
        "DesignDiscreteValues;50;1.;2.\n"
        "DesignTopometryRegion;12;ShellProp;9;thickness;2.;;;;DesignVarLink;1;0.5;2.;DesignDiscreteValues;50;Symmetry;"
        "3;"
        "PlaneXy;;;;;11\n");
    REQUIRE(unresolvedIn(world) == 0);
    Renumbering::apply(world, RenumberSpace::Property, Renumbering::offset(1000));
    Renumbering::apply(world, RenumberSpace::DiscreteValueSet, Renumbering::offset(100));
    Renumbering::apply(world, RenumberSpace::Coordinate, Renumbering::offset(10));
    const DesignTopometryRegionCard moved = cardAt<DesignTopometryRegionTable>(world);
    REQUIRE((moved.propId == 1009 && moved.discreteValuesId == 150 && moved.symmetry->frameId == 13));
    REQUIRE((moved.symmetry->symmetricProperties == std::vector<base::EntityId>{1011} && moved.link->tableId == 1));
    REQUIRE(unresolvedIn(world) == 0);
}

TEST_CASE("a TOMVAR refuses a layout or a value the format reference does not allow", "[model][cards][design]")
{
    expectInvalid("DesignTopometryRegion;0;ShellProp;5;thickness", "id");
    expectInvalid("DesignTopometryRegion;1;;5;t", "kind");
    expectInvalid("DesignTopometryRegion;1;ShellProp;0;thickness", "propId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5", "fieldName");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;0", "fieldName");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;0", "fieldName"); // the format says an integer greater than 0
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;abc", "initialValue");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;1.;;;0.", "designVarMoveLimit");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;1.;;;-1.", "designVarMoveLimit");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;DesignVarLink;2;0.;1.;DesignVarLink;3;0.;1.",
                  "lineKeyword");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;;1;5", "lineKeyword");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;DesignVarLink;0;1.;2.", "tableId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;DesignVarLink;2;1.", "coefficient1");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;DesignDiscreteValues", "discreteValuesId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;DesignDiscreteValues;0", "discreteValuesId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Stress", "stressLimit");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Stress;0.", "stressLimit");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;;AB", "mirrorPlanes");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;;;;;Q;3", "cyclicAxis");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;;;;;XDirection",
                  "cyclicSegments"); // the axis and the number of segments go together
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;-1", "frameId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;;;;;;;0", "symmetryPropId");
    expectInvalid("DesignTopometryRegion;1;ShellProp;5;thickness;;;;;Symmetry;;;;;;;1;;1;x", "symmetryPropId");
}

// ---------------------------------------------------------------------------------------------------------------------
// DVLREL1
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a DVLREL1 designs a value of a load, with attributes that say which load of the set",
          "[model][cards][design]")
{
    Model model = readBulk(
        "DesignLoadRelation1;10;PointForceLoad;320;directionX;;;;110;;;;;10;1.\n"
        "DesignLoadRelation1;11;PointMomentLoad;400;magnitude;-5.;50.;1.5;200;3;;;;7;1.;8;-2.;9;0.5;;;10;3.\n"
        "DesignLoadRelation1;12;LoadCombination;5;scaleFactor;;;;0;;;;;4;PropertyValue\n"
        "DesignLoadRelation1;13;LoadCombination;5;termScaleFactor(2);;;;60;;;;;4;1.\n"
        "DesignLoadRelation1;14;PointForceLoad;1;magnitude;;;;;;;;;5;2.\n");
    REQUIRE((DesignLoadRelation1Table::INFO.category == CardCategory::Other &&
             DesignLoadRelation1Table::INFO.idSpace == base::IdSpace::PerCardType));
    const DesignLoadRelation1Card example = cardAt<DesignLoadRelation1Table>(model, 0);
    REQUIRE((example.id == 10 && example.kind == "PointForceLoad" && example.setId == 320 &&
             example.fieldName == "directionX"));
    REQUIRE((example.minValue == -1.0e35 && example.maxValue == 1.0e20 && example.constant0 == 0.0)); // the defaults
    REQUIRE((example.attributes[0] == 110 && !example.attributes[1].has_value()));
    REQUIRE((example.terms.size() == 1 && example.terms[0].designVarId == 10 && example.terms[0].coefficient == 1.0));
    REQUIRE_FALSE(example.fromProperty);
    const DesignLoadRelation1Card full = cardAt<DesignLoadRelation1Table>(model, 1);
    REQUIRE((full.minValue == -5.0 && full.maxValue == 50.0 && full.constant0 == 1.5));
    REQUIRE((full.attributes[0] == 200 && full.attributes[1] == 3 && !full.attributes[2].has_value()));
    REQUIRE(full.terms.size() == 4); // the pairs go on over the lines
    REQUIRE((full.terms[1].designVarId == 8 && full.terms[1].coefficient == -2.0 && full.terms[3].designVarId == 10));
    // A 0 is kept as a 0 (the attribute of the scale S of a LOAD), and a blank stays blank; PVAL takes the coefficient
    // from the load.
    const DesignLoadRelation1Card scale = cardAt<DesignLoadRelation1Table>(model, 2);
    REQUIRE((scale.kind == "LoadCombination" && scale.fieldName == "scaleFactor" && scale.attributes[0] == 0 &&
             scale.fromProperty));
    const DesignLoadRelation1Card component = cardAt<DesignLoadRelation1Table>(model, 3);
    REQUIRE((component.kind == "LoadCombination" && component.fieldName == "termScaleFactor(2)" &&
             component.attributes[0] == 60)); // indexed term selector
    const DesignLoadRelation1Card bare = cardAt<DesignLoadRelation1Table>(model, 4);
    REQUIRE((!bare.attributes[0].has_value() && bare.terms.size() == 1)); // the attributes are optional
    requireRoundTrip(model);

    // What the attributes name depends on the TYPE; a 0 or a blank names nothing.
    REQUIRE(referencesOf<DesignLoadRelation1Table>(model, 0) ==
            Seens{{"setId", ReferenceTarget::LoadCombinationSet, 320},
                  {"attributes", ReferenceTarget::Node, 110},
                  {"designVarIds", ReferenceTarget::DesignVariable, 10}});
    REQUIRE(referencesOf<DesignLoadRelation1Table>(model, 1).size() ==
            7); // the set, the grid, the system, four variables
    REQUIRE(referencesOf<DesignLoadRelation1Table>(model, 1)[2] == Seen{"attributes", ReferenceTarget::Coordinate, 3});
    REQUIRE(
        referencesOf<DesignLoadRelation1Table>(model, 2) ==
        Seens{{"setId", ReferenceTarget::LoadCombinationSet, 5}, {"designVarIds", ReferenceTarget::DesignVariable, 4}});
    REQUIRE(referencesOf<DesignLoadRelation1Table>(model, 3) ==
            Seens{{"setId", ReferenceTarget::LoadCombinationSet, 5},
                  {"attributes", ReferenceTarget::LoadCombinationSet, 60},
                  {"designVarIds", ReferenceTarget::DesignVariable, 4}});
    REQUIRE(loadAttributeOf(example, 0) == LoadAttribute::Grid);
    REQUIRE(loadAttributeOf(example, 1) == LoadAttribute::Coordinate);
    REQUIRE(loadAttributeOf(example, 2) == LoadAttribute::Unused);
    REQUIRE(loadAttributeOf(scale, 0) == LoadAttribute::Unused);
    REQUIRE(loadAttributeOf(component, 0) == LoadAttribute::LoadCombinationSet);

    Model world = readBulk(
        "Node;110\nDesignVar;10;A;1.\nPointForceLoad;320;110;;1.;1.;0.;0.\nDesignLoadRelation1;10;PointForceLoad;320;"
        "directionX;;;;110;;"
        ";;;10;1.\n");
    REQUIRE(unresolvedIn(world) == 0);
    REQUIRE(unresolvedIn(readBulk("DesignLoadRelation1;10;PointForceLoad;320;directionX;;;;110;;;;;10;1.")) == 3);
    Renumbering::apply(world, RenumberSpace::LoadCombinationSet, Renumbering::offset(1000));
    Renumbering::apply(world, RenumberSpace::Node, Renumbering::offset(50));
    Renumbering::apply(world, RenumberSpace::DesignVariable, Renumbering::offset(5));
    const DesignLoadRelation1Card moved = cardAt<DesignLoadRelation1Table>(world);
    REQUIRE((moved.setId == 1320 && moved.attributes[0] == 160 && moved.terms[0].designVarId == 15));
    REQUIRE(unresolvedIn(world) == 0);
}

TEST_CASE("a DVLREL1 refuses a layout or a value the format reference does not allow", "[model][cards][design]")
{
    expectInvalid("DesignLoadRelation1;0;PointForceLoad;1;magnitude;;;;;;;;;5;1.", "id");
    expectInvalid("DesignLoadRelation1;1;;1;f;;;;;;;;;5;1.", "kind");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;0;magnitude;;;;;;;;;5;1.", "setId");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;;;;;;;;;;5;1.", "fieldName");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;q;;;;;;;;;5;1.",
                  "fieldName"); // the table of the format: F, N1 to N3
    expectInvalid("DesignLoadRelation1;1;PointMomentLoad;1;f;;;;;;;;;5;1.", "fieldName");
    expectInvalid("DesignLoadRelation1;1;LoadCombination;1;f;;;;;;;;;5;1.", "fieldName");
    expectInvalid("DesignLoadRelation1;1;LoadCombination;1;sx;;;;;;;;;5;1.", "fieldName");
    REQUIRE_NOTHROW(readBulk(
        "DesignLoadRelation1;1;FacePressureLoad;1;p;;;;;;;;;5;1.")); // a type the format does not list keeps its name
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;magnitude;abc;;;;;;;;5;1.", "minValue");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;magnitude;;;;-1;;;;;5;1.", "attributes");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;magnitude\n", "designVarIds"); // the continuation is required
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;magnitude;;;;;;;;;5", "coefficients");
    expectInvalid("DesignLoadRelation1;1;PointForceLoad;1;magnitude;;;;;;;;;5;PropertyValue;6;PropertyValue",
                  "coefficients"); // PVAL only for the first
}

// ---------------------------------------------------------------------------------------------------------------------
// DVPSURF
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a DVPSURF sets a control surface of a trim as a design variable, and names the surface by its label",
          "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;100;A;1.\nAeroTrim;1;0.9;100.\nDesignControlSurface;10;INBFLAP;1;100;0.02618\nDesignControlSurface;"
        "11;elev;2;3;-1.\n");
    REQUIRE((DesignControlSurfaceTable::INFO.category == CardCategory::Other &&
             DesignControlSurfaceTable::INFO.idSpace == base::IdSpace::PerCardType));
    const DesignControlSurfaceCard example = cardAt<DesignControlSurfaceTable>(model, 0);
    REQUIRE((example.id == 10 && example.controlSurfaceLabel == "INBFLAP" && example.trimId == 1 &&
             example.designVarId == 100));
    REQUIRE(example.coefficient == Approx(0.02618));
    REQUIRE(cardAt<DesignControlSurfaceTable>(model, 1).controlSurfaceLabel ==
            "elev"); // a label is kept as the deck wrote it
    requireRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("DesignControlSurface;1;A;1;1;1.\nDesignControlSurface;1;B;1;1;1."), base::DeckError);

    // The label is a name: the trim and the variable are the references.
    REQUIRE(referencesOf<DesignControlSurfaceTable>(model, 0) ==
            Seens{{"trimId", ReferenceTarget::AeroTrimSet, 1}, {"designVarId", ReferenceTarget::DesignVariable, 100}});
    REQUIRE(unresolvedIn(model) == 2); // the second card names a trim and a variable that no card defines
    Model resolved =
        readBulk("DesignVar;100;A;1.\nAeroTrim;1;0.9;100.\nDesignControlSurface;10;INBFLAP;1;100;0.02618\n");
    REQUIRE(unresolvedIn(resolved) == 0);
    Renumbering::apply(resolved, RenumberSpace::DesignVariable, Renumbering::offset(1000));
    Renumbering::apply(resolved, RenumberSpace::AeroTrimSet, Renumbering::offset(50));
    REQUIRE((cardAt<DesignControlSurfaceTable>(resolved).designVarId == 1100 &&
             cardAt<DesignControlSurfaceTable>(resolved).trimId == 51));
    REQUIRE(cardAt<DesignControlSurfaceTable>(resolved).controlSurfaceLabel == "INBFLAP");
    REQUIRE(unresolvedIn(resolved) == 0);

    expectInvalid("DesignControlSurface;0;FLAP;1;1;1.", "id");
    expectInvalid("DesignControlSurface;1;;1;1;1.", "controlSurfaceLabel");
    REQUIRE_NOTHROW(readBulk("DesignControlSurface;1;TOOLONGLBL;1;1;1."));
    expectInvalid("DesignControlSurface;1;FLAP;0;1;1.", "trimId");
    expectInvalid("DesignControlSurface;1;FLAP;1;0;1.", "designVarId");
    expectInvalid("DesignControlSurface;1;FLAP;1;1", "coefficient");
    expectInvalid("DesignControlSurface;1;FLAP;1;1;abc", "coefficient");
}
