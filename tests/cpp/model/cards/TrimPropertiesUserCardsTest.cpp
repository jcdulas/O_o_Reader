// Checks of the trim, property and user cards: the perforated shell property PSHLPF, the distributions of property
// data over elements PDISTB and PDISTBM, the user service cards ELEMUDS and THPAD of a PBUSH2D, and the trim component
// cards ACPEMCP, ACPMCP1, TRMCPL and TCOMPG. Per card: the fields in their format reference positions, the defaults,
// the references it lists and what each one points to, the renumbering of the spaces it owns or names, the refused
// values and the round trip through the three formats. The generic tests of CardRoundTripTest.cpp read, write and copy
// every sample; this file covers what each card means.

#include "oor/model/cards/properties/perforatedShellProp/PerforatedShellPropTable.hpp"
#include "oor/model/cards/properties/propDistribution/PropDistributionTable.hpp"
#include "oor/model/cards/properties/propMatDistribution/PropMatDistributionTable.hpp"
#include "oor/model/cards/properties/tiltingPadBearingProp/TiltingPadBearingPropTable.hpp"
#include "oor/model/cards/properties/userElemService/UserElemServiceTable.hpp"
#include "oor/model/cards/thermal/trimComponentCoupling/TrimComponentCouplingTable.hpp"
#include "oor/model/cards/thermal/trimComponentCoupling1D/TrimComponentCoupling1DTable.hpp"
#include "oor/model/cards/thermal/trimCouplingTolerances/TrimCouplingTolerancesTable.hpp"
#include "oor/model/cards/thermal/trimLayeredComponent/TrimLayeredComponentTable.hpp"
#include "support/ExplicitTestSupport.hpp"
#include "support/SlotRecord.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The fields and targets of the references a card lists, as a visitor sees them.
struct Visited
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
};

template <typename TableT>
Visited visitedBy(Model& model, std::uint32_t row)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return Visited{visitor.fields, visitor.targets};
}

// The references a card places in a part of the deck (ReferenceVisitor::visitInPart), with the part they name.
struct PartReferenceRecorder final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    std::vector<std::string> parts;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        parts.emplace_back();
        return id;
    }
    base::EntityId visitInPart(std::string_view field, ReferenceTarget target, base::EntityId id,
                               std::string_view partKind, std::int64_t partId) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        parts.push_back(std::string(partKind) + " " + std::to_string(partId));
        return id;
    }
};

template <typename TableT>
PartReferenceRecorder partReferencesOf(Model& model, std::uint32_t row)
{
    PartReferenceRecorder visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor;
}

// A material, four grid points, a shell property and three shell elements: what the elements of a PDISTB name.
const std::string SHELLS_TEXT =
    "IsoMat;1;2.1E+11;;0.3\n"
    "Node;1\n"
    "Node;2\n"
    "Node;3\n"
    "Node;4\n"
    "ShellProp;7400;1;0.1\n"
    "ShellQuad4;7205;7400;1;2;3;4\n"
    "ShellQuad4;7224;7400;1;2;3;4\n"
    "ShellQuad4;7362;7400;1;2;3;4\n";

// The IdListSet1 or TypedListSet3 records with the IDs `first` to `last` that the trim component cards name: `name` is
// "IdListSet1;" or "TypedListSet3;", `members` what follows the ID.
std::string setCards(int first, int last, const std::string& name, const std::string& members)
{
    std::string text;
    for (int id = first; id <= last; ++id)
        text += name + std::to_string(id) + members;
    return text;
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// PSHLPF
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PSHLPF is a property of a perforated shell: a material, a hole pattern, and a flag that defaults to 1",
          "[model][cards][pshlpf]")
{
    REQUIRE((PerforatedShellPropTable::INFO.category == CardCategory::Property &&
             PerforatedShellPropTable::INFO.idSpace == base::IdSpace::Property));
    Model model = readBulk(
        "IsoMat;2;2.1E+11;;0.3\n"
        "PerforatedShellProp;1;2;6.5E-4;0.0125;0.0025;Square;1.15;1.6E-5\n"
        "PerforatedShellProp;5;2;0.002;0.015;0.003;Triangular;1.2;1.8E-5;0\n"
        "PerforatedShellProp;6;2;0.001;0.02;0.004;Hexagonal;1.2;1.8E-5\n");
    const PerforatedShellPropCard perforated = cardAt<PerforatedShellPropTable>(model, 0);
    REQUIRE((perforated.id == 1 && perforated.matId == 2 && perforated.thickness == 6.5e-4 &&
             perforated.holeSpacing == 1.25e-2 && perforated.holeRadius == 2.5e-3));
    REQUIRE((perforated.topology == PerforatedShellPropTopology::Square && perforated.fluidDensity == 1.15 &&
             perforated.fluidViscosity == 1.6e-5));
    REQUIRE(perforated.homogenization == 1); // blank: the default, written as a blank
    const PerforatedShellPropCard off = cardAt<PerforatedShellPropTable>(model, 1);
    REQUIRE((off.topology == PerforatedShellPropTopology::Tria &&
             off.homogenization == 0)); // the word in any case; HOMG on the second line
    REQUIRE(cardAt<PerforatedShellPropTable>(model, 2).topology == PerforatedShellPropTopology::Hexa);
    requireRoundTrip(model);

    // The default is not written; a zero is.

    // The material is the only reference, and a renumbering of the materials and of the properties follows into the
    // card.
    const Visited visited = visitedBy<PerforatedShellPropTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"matId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("PerforatedShellProp;1;2;0.1;0.1;0.1;Square;1.;1.");
    REQUIRE(unresolvedIn(lacking) == 1);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<PerforatedShellPropTable>(model, 0).matId == 1002 &&
             cardAt<PerforatedShellPropTable>(model, 2).matId == 1002));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<PerforatedShellPropTable>(model, 0).id == 1001 &&
             cardAt<PerforatedShellPropTable>(model, 1).id == 1005));
    REQUIRE(model.locate(base::IdSpace::Property, 1005).has_value());
    REQUIRE_FALSE(model.locate(base::IdSpace::Property, 5).has_value());
    REQUIRE(unresolvedIn(model) == 0);

    // A shell element names it like any property.
    Model shells = readBulk(
        "IsoMat;2;2.1E+11;;0.3\n"
        "Node;1\n"
        "Node;2\n"
        "Node;3\n"
        "Node;4\n"
        "ShellQuad4;10;1;1;2;3;4\n"
        "PerforatedShellProp;1;2;0.1;0.1;0.1;Square;1.;1.");
    REQUIRE(unresolvedIn(shells) == 0);

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    PerforatedShellPropTable& table = model.table<PerforatedShellPropTable>();
    PerforatedShellPropCard edited = table.card(2);
    edited.topology = PerforatedShellPropTopology::Square;
    edited.homogenization = 0;
    REQUIRE_FALSE(PerforatedShellPropFormat::check(edited).has_value());
    table.store(2, edited);
    REQUIRE((cardAt<PerforatedShellPropTable>(model, 2).topology == PerforatedShellPropTopology::Square &&
             cardAt<PerforatedShellPropTable>(model, 2).homogenization == 0));
    edited.homogenization = 2;
    REQUIRE(PerforatedShellPropFormat::check(edited).has_value());
    edited.homogenization = 1;
    edited.fluidViscosity = 0.0;
    REQUIRE(PerforatedShellPropFormat::check(edited).has_value());

    // A property ID is unique among all the properties, and a deleted card leaves its ID free.
    REQUIRE_THROWS_AS(readBulk("PerforatedShellProp;1;2;0.1;0.1;0.1;Square;1.;1.\nShellProp;1;2;0.1"), base::DeckError);
    const std::size_t before = model.cardCount();
    model.remove(*model.locate(base::IdSpace::Property, 1005));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_FALSE(model.locate(base::IdSpace::Property, 1005).has_value());

    expectInvalid("PerforatedShellProp;0;2;1.;1.;1.;Square;1.;1.", "id");
    expectInvalid("PerforatedShellProp;1;;1.;1.;1.;Square;1.;1.", "matId"); // required
    expectInvalid("PerforatedShellProp;1;-2;1.;1.;1.;Square;1.;1.", "matId");
    expectInvalid("PerforatedShellProp;1;2;;1.;1.;Square;1.;1.", "thickness"); // none of the real fields has a default
    expectInvalid("PerforatedShellProp;1;2;0.;1.;1.;Square;1.;1.", "thickness");
    expectInvalid("PerforatedShellProp;1;2;1.;0.;1.;Square;1.;1.", "holeSpacing");
    expectInvalid("PerforatedShellProp;1;2;1.;1.;-1.;Square;1.;1.", "holeRadius");
    expectInvalid("PerforatedShellProp;1;2;1.;1.;1.;ROUND;1.;1.", "topology");
    expectInvalid("PerforatedShellProp;1;2;1.;1.;1.;;1.;1.", "topology"); // required
    expectInvalid("PerforatedShellProp;1;2;1.;1.;1.;Square;0.;1.", "fluidDensity");
    expectInvalid("PerforatedShellProp;1;2;1.;1.;1.;Square;1.;abc", "fluidViscosity");
    expectInvalid("PerforatedShellProp;1;2;1.;1.;1.;Square;1.;1.;2", "homogenization");
}

// ---------------------------------------------------------------------------------------------------------------------
// PDISTB
// ---------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// PDISTBM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PDISTBM gives the material that replaces one material of a property for each element",
          "[model][cards][pdistbm]")
{
    REQUIRE((PropMatDistributionTable::INFO.category == CardCategory::Other &&
             PropMatDistributionTable::INFO.idSpace == base::IdSpace::PerCardType));
    const Model cards = readBulk(
        "PropMatDistribution;60017;ShellProp;7400;2;7205;1600\n"
        "PropMatDistribution;2;BarProp;703\n"
        "PropMatDistribution;3;CompProp;702;0;301;10;302;11;303;12;304;13;305;14\n");
    const PropMatDistributionCard replaced = cardAt<PropMatDistributionTable>(cards, 0);
    REQUIRE((replaced.id == 60017 && replaced.type == DistributedPropertyType::ShellProp && replaced.propId == 7400));
    REQUIRE((replaced.materialIndex == 2 && replaced.entries.size() == 1 &&
             replaced.entries[0] == std::pair<base::EntityId, base::EntityId>{7205, 1600}));
    const PropMatDistributionCard defaults = cardAt<PropMatDistributionTable>(cards, 1);
    REQUIRE(
        (defaults.type == DistributedPropertyType::BarProp && defaults.materialIndex == 1 && defaults.entries.empty()));
    const PropMatDistributionCard many = cardAt<PropMatDistributionTable>(cards, 2);
    REQUIRE((many.materialIndex == 0 && many.entries.size() == 5 && many.entries[4].second == 14));
    requireRoundTrip(cards);

    (void)0; // MIDNUM 1 is the default

    // The references: the property, then the element and the material of each pair.
    Model model = readBulk(SHELLS_TEXT +
                           "IsoMat;1600;2.1E+11;;0.3\n"
                           "IsoMat;1700;2.1E+11;;0.3\n"
                           "PropMatDistribution;10;ShellProp;7400;2;7205;1600;7224;1700\n");
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<PropMatDistributionTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"propId", "elemId", "elemMatId", "elemId", "elemMatId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Element,
                                                            ReferenceTarget::Material, ReferenceTarget::Element,
                                                            ReferenceTarget::Material});
    Model missing = readBulk("PropMatDistribution;10;ShellProp;7400;;7205;1600");
    REQUIRE(unresolvedIn(missing) == 3);

    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<PropMatDistributionTable>(model).entries[0].second == 2600 &&
             cardAt<PropMatDistributionTable>(model).entries[1].second == 2700));
    REQUIRE(cardAt<PropMatDistributionTable>(model).materialIndex == 2); // a position, not an ID
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<PropMatDistributionTable>(model).entries[0].first == 8205 &&
             cardAt<PropMatDistributionTable>(model).entries[1].first == 8224));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<PropMatDistributionTable>(model).propId == 8400 && cardAt<PropMatDistributionTable>(model).id == 10));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("PropMatDistribution;1;RodProp;2\nPropMatDistribution;1;RodProp;3"), base::DeckError);

    PropMatDistributionTable& table = model.table<PropMatDistributionTable>();
    PropMatDistributionCard edited = table.card(0);
    edited.materialIndex = 3;
    edited.entries.emplace_back(8362, 2600);
    REQUIRE_FALSE(PropMatDistributionFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE((cardAt<PropMatDistributionTable>(model).materialIndex == 3 &&
             cardAt<PropMatDistributionTable>(model).entries.size() == 3));
    edited.entries.emplace_back(8362, 2700); // the same element twice
    REQUIRE(PropMatDistributionFormat::check(edited).has_value());

    expectInvalid("PropMatDistribution;0;RodProp;7", "id");
    expectInvalid("PropMatDistribution;1;RodProp", "propId");
    expectInvalid("PropMatDistribution;1;PRODX;7", "type");
    expectInvalid("PropMatDistribution;1;;7", "type");
    expectInvalid("PropMatDistribution;1;RodProp;7;-1", "materialIndex");
    expectInvalid("PropMatDistribution;1;RodProp;7;;10", "elemMatId");
    expectInvalid("PropMatDistribution;1;RodProp;7;;;5", "elemId");
    expectInvalid("PropMatDistribution;1;RodProp;7;;10;0", "elemMatId");
    expectInvalid("PropMatDistribution;1;RodProp;7;;10;5;10;6", "elemId"); // an element once
    expectInvalid("PropMatDistribution;1;RodProp;7;;10;2.5", "elemMatId");
}

// ---------------------------------------------------------------------------------------------------------------------
// ELEMUDS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ELEMUDS extends a PBUSH2D with the user service that computes it and the data the service is given",
          "[model][cards][elemuds]")
{
    REQUIRE((UserElemServiceTable::INFO.category == CardCategory::Property &&
             UserElemServiceTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "BushProp2D;17;1.;1.\n"
        "UserElemService;17;BushProp2D;BRGSVC;THPAD;Frequency\n"
        "BushProp2D;18;1.;1.\n"
        "UserElemService;18;BushProp2D;GRP1\n"
        "BushProp2D;19;1.;1.\n"
        "UserElemService;19;BushProp2D;grp2;UMS;NoMassDependence;60.;IntegerBlock;9;1;2;3;4;5;6;7;8;9;RealBlock;2;1.5;"
        "2.5;TextBlock;2;ALPHA;BETA\n"
        "BushProp2D;20;1.;1.\n"
        "UserElemService;20;BushProp2D;GRP3;;Frequency;25.;TextBlock;8;A;B;C;D;E;F;G;H;RealBlock;1;0.5;IntegerBlock;3;-"
        "5;0;5\n");
    const UserElemServiceCard service = cardAt<UserElemServiceTable>(model, 0);
    REQUIRE((service.id == 17 && service.propType == UserElemServicePropertyType::BushProp2D &&
             service.groupName == "BRGSVC"));
    REQUIRE((service.userRoutineName == "THPAD" && service.dependence == UserElemServiceDependence::Frequency &&
             !service.nominalSpeed.has_value()));
    REQUIRE((service.integerData.empty() && service.realData.empty() && service.textData.empty()));
    const UserElemServiceCard minimal = cardAt<UserElemServiceTable>(model, 1);
    REQUIRE((minimal.groupName == "GRP1" && minimal.userRoutineName.empty() &&
             minimal.dependence == UserElemServiceDependence::Nominal));
    const UserElemServiceCard full = cardAt<UserElemServiceTable>(model, 2);
    REQUIRE((full.groupName == "grp2" && full.userRoutineName == "UMS" &&
             full.nominalSpeed == 60.0)); // the names are kept as written
    REQUIRE(full.integerData == std::vector<std::int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    REQUIRE(full.realData == std::vector<double>{1.5, 2.5});
    REQUIRE(full.textData == std::vector<std::string>{"ALPHA", "BETA"});
    // The sections come in any order, each on a line of its own, and the lines after the first of a section go on with
    // it.
    const UserElemServiceCard reordered = cardAt<UserElemServiceTable>(model, 3);
    REQUIRE((reordered.dependence == UserElemServiceDependence::Frequency && reordered.nominalSpeed == 25.0 &&
             reordered.userRoutineName.empty()));
    REQUIRE(reordered.textData == std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "H"});
    REQUIRE((reordered.realData == std::vector<double>{0.5} &&
             reordered.integerData == std::vector<std::int32_t>{-5, 0, 5}));
    requireRoundTrip(model);

    // Written in the order of the format reference: INT, REAL, CHAR, each on a line of its own, seven values to a line.

    // The PID is the one of the PBUSH2D that the card extends: it is a reference, and a renumbering of the properties
    // moves both. The ELEMUDS has an ID space of its own: the PBUSH2D 17 and the ELEMUDS 17 coexist.
    const Visited visited = visitedBy<UserElemServiceTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"id"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<UserElemServiceTable>(model, 0).id == 1017 && cardAt<UserElemServiceTable>(model, 3).id == 1020));
    REQUIRE(model.table<UserElemServiceTable>().idAtRow(2) == 1019);
    REQUIRE(unresolvedIn(model) == 0);
    Model orphan = readBulk("UserElemService;17;BushProp2D;GRP1");
    REQUIRE(unresolvedIn(orphan) == 1);
    REQUIRE_THROWS_AS(readBulk("UserElemService;17;BushProp2D;GRP1\nUserElemService;17;BushProp2D;GRP2"),
                      base::DeckError);

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    UserElemServiceTable& table = model.table<UserElemServiceTable>();
    UserElemServiceCard edited = table.card(1);
    edited.userRoutineName = "THPAD";
    edited.integerData = {7};
    edited.textData = {"ONE"};
    REQUIRE_FALSE(UserElemServiceFormat::check(edited).has_value());
    table.store(1, edited);
    REQUIRE((cardAt<UserElemServiceTable>(model, 1).userRoutineName == "THPAD" &&
             cardAt<UserElemServiceTable>(model, 1).integerData.size() == 1));
    requireRoundTrip(model);
    edited.textData = {"RealBlock"}; // would open a section when the card is read again
    REQUIRE(UserElemServiceFormat::check(edited).has_value());
    edited.textData = {"two words"};
    REQUIRE_FALSE(UserElemServiceFormat::check(edited).has_value());

    expectInvalid("UserElemService;0;BushProp2D;G", "id");
    expectInvalid("UserElemService;1;PBUSH1D;G", "propType"); // PBUSH2D is the one property the format supports
    expectInvalid("UserElemService;1;;G", "propType");
    expectInvalid("UserElemService;1;BushProp2D", "groupName"); // no default
    oor::testing::requireModelFileRoundTrip(readModelText("UserElemService;1;BushProp2D;GROUPNAME12", true));
    REQUIRE_NOTHROW(readBulk("UserElemService;1;BushProp2D;G;UNAMETOOLONG")); // the full routine name is preserved
    expectInvalid("UserElemService;1;BushProp2D;G;U;SLOW", "dependence");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;abc", "nominalSpeed");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;IntegerBlock;1;1;IntegerBlock;1;2",
                  "integerData"); // a section once
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;RealBlock;1;1.;RealBlock;1;2.", "realData");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;TextBlock;1;A;TextBlock;1;B", "textData");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;IntegerBlock;1;1.5", "integerData");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;RealBlock;1;abc", "realData");
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;TextBlock;1;IntegerBlock",
                  "textData"); // a section word is not a value
    expectInvalid("UserElemService;1;BushProp2D;G;U;;;TextBlock;1;RealBlock", "textData");
}

// ---------------------------------------------------------------------------------------------------------------------
// THPAD
// ---------------------------------------------------------------------------------------------------------------------

namespace
{

// A tilting pad bearing of five pads and two speed and load cases, with a blank cell where the layout has one, a pad
// arc written as an integer and the pivot line at its defaults; the test adds the PBUSH2D and the ELEMUDS of the same
// number.
const std::string TILTING_PAD_FIVE_PADS =
    "TiltingPadBearingProp;200;1500.;Five pad journal bearing of the project test rig;Load between "
    "pads;1.75;0.0025;2.6;2.2;2.9E+7;6.2E-6;6.4E-6;6.6E-6;5;24;0;56.;0.55;0.25;36.;0.0012;0.;0.;0.;56.;0.55;0.25;108.;"
    "0.0012;0.;0.;0.;56.;0.55;0.25;180.;0.0012;0.;0.;0.;56;0.55;0.25;252.;0.0012;0.;0.;0.;56.;0.55;0.25;324.;0.0012;0.;"
    "0.;0.;1.8E-6;7.2E-4;110.;160.;0.;160.;4.5;0.;8.2E-5;175.;104.;3.9E-6;200.;7.2E-7;0.8;70.;0.002;0.15;0.1;3.;80.;90."
    ";1;1;0;1;0;1;0;0;1;4;1;1;0;0.;0.;100.;0.;2;100;50;1800.;0.;642.5;1.;1.;0.;1.;3600.;0.;642.5;1.;1.;0.;1.\n";

// The same card with another RID.
const std::string TILTING_PAD_FIXED =
    "TiltingPadBearingProp;201;1500.;Five pad journal bearing of the project test rig;Load between "
    "pads;1.75;0.0025;2.6;2.2;2.9E+7;6.2E-6;6.4E-6;6.6E-6;5;24;0;56.;0.55;0.25;36.;0.0012;0.;0.;0.;56.;0.55;0.25;108.;"
    "0.0012;0.;0.;0.;56.;0.55;0.25;180.;0.0012;0.;0.;0.;56;0.55;0.25;252.;0.0012;0.;0.;0.;56.;0.55;0.25;324.;0.0012;0.;"
    "0.;0.;1.8E-6;7.2E-4;110.;160.;0.;160.;4.5;0.;8.2E-5;175.;104.;3.9E-6;200.;7.2E-7;0.8;70.;0.002;0.15;0.1;3.;80.;90."
    ";1;1;0;1;0;1;0;0;1;4;1;1;0;0.;0.;100.;0.;2;100;50;1800.;0.;642.5;1.;1.;0.;1.;3600.;0.;642.5;1.;1.;0.;1.\n";

// The eight slots of one line of the card given as the text of a line of eight fields after a comma (",1,,2"): the
// tests below replace one line to give a fault.
std::vector<std::string> lineSlots(const std::string& line)
{
    std::vector<std::string> slots;
    std::size_t start = line.find(',');
    while (start != std::string::npos)
    {
        const std::size_t end = line.find(',', start + 1);
        slots.push_back(line.substr(start + 1, end == std::string::npos ? std::string::npos : end - start - 1));
        start = end;
    }
    slots.resize(std::max<std::size_t>(slots.size(), 8));
    return slots;
}

// A complete card of one pad and one case whose lines of slots can be replaced to give a fault: the three lines of the
// head, the counts, the pad, the thermal and fluid lines, the solver and flag lines, the pivot line, the control line
// and the case.
std::string smallThpad(const std::string& counts = ",1", const std::string& pad = ",1.",
                       const std::string& solver = ",", const std::string& flags = ",",
                       const std::string& control = ",1", const std::string& caseLine = ",1.")
{
    std::vector<std::string> slots = lineSlots(",1");
    for (const std::string& line :
         {std::string(","), std::string(","), std::string(","), counts, pad, std::string(","), std::string(","), solver,
          flags, std::string(","), std::string(","), control, caseLine})
    {
        const std::vector<std::string> more = lineSlots(line);
        slots.insert(slots.end(), more.begin(), more.end());
    }
    return slotRecord("TiltingPadBearingProp", slots);
}

} // namespace

TEST_CASE("a THPAD holds the grid of a tilting pad bearing: titles, counts, a line for each pad and each case",
          "[model][cards][thpad]")
{
    REQUIRE((TiltingPadBearingPropTable::INFO.category == CardCategory::Property &&
             TiltingPadBearingPropTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk("BushProp2D;200;1.;1.\nUserElemService;200;BushProp2D;GR1;THPAD;Frequency\n" +
                           TILTING_PAD_FIVE_PADS + TILTING_PAD_FIXED);
    const TiltingPadBearingPropCard card = cardAt<TiltingPadBearingPropTable>(model, 0);
    REQUIRE((card.id == 200 && card.timeLimit == 1500.0));
    REQUIRE(card.title1 == "Five pad journal bearing of the project test rig");
    REQUIRE(card.title2 == "Load between pads");
    // The pad modulus is the fifth value of its line, before the three expansion coefficients.
    REQUIRE((card.journalRadius == 1.75 && card.radialClearance == 0.0025 && card.padOuterRadius == 2.6 &&
             card.bearingLength == 2.2));
    REQUIRE((card.padModulus == 2.9e7 && card.padExpansion == 6.2e-6 && card.journalExpansion == 6.4e-6 &&
             card.housingExpansion == 6.6e-6));
    REQUIRE((card.elementCount == 24 && card.eccentricityInput == 0 && card.pads.size() == 5));
    REQUIRE((card.pads[0][0] == 56.0 && card.pads[0][1] == 0.55 && card.pads[0][2] == 0.25 && card.pads[0][3] == 36.0));
    REQUIRE((card.pads[0][4] == 0.0012 && card.pads[0][5] == 0.0 && card.pads[0][7] == 0.0));
    REQUIRE((card.pads[3][0] == 56.0 && card.pads[3][3] == 252.0 && card.pads[4][3] == 324.0));
    REQUIRE((card.oilConductivity == 1.8e-6 && card.padConductivity == 0.00072 && card.padBackTemperature == 110.0 &&
             card.journalTemperature == 160.0));
    REQUIRE((card.sumpPressure == 0.0 && card.inletTemperature == 160.0 && card.cavitationConductivity == 4.5 &&
             card.cavitationSpecificHeat == 0.0));
    REQUIRE((card.oilDensity == 8.2e-5 && card.oilSpecificHeat == 175.0 && card.viscosityTemperatureA == 104.0 &&
             card.viscosityA == 3.9e-6));
    REQUIRE((card.viscosityTemperatureB == 200.0 && card.viscosityB == 7.2e-7 && card.sumpMixingFactor == 0.8 &&
             card.manufacturingTemperature == 70.0));
    REQUIRE((card.convergenceTolerance == 0.002 && card.initialPositionX == 0.15 && card.initialPositionY == 0.1 &&
             card.axialNodeCount == 3.0));
    REQUIRE((card.pressureRelaxation == 80.0 && card.temperatureRelaxation == 90.0));
    REQUIRE((card.journalThermalModel == 1 && card.inletThermalBoundary == 1 && card.turbulence == 0 &&
             card.dimensionalOutput == 1 && card.padConduction == 0));
    REQUIRE((card.padBackThermal == 1 && card.journalThermalGradient == 0));
    REQUIRE((card.cavitation == 0 && card.crossCoupling == 1 && card.padDeformation == 4 &&
             card.pivotFlexibility == 1 && card.unitSystem == 1));
    REQUIRE(card.floodedSump == 0);
    REQUIRE((card.pivotRotationStiffness == 0.0 && card.initialPivotDeflection == 0.0 &&
             card.hotOilCarryOver == 100.0 && card.coldOilCarryOver == 0.0));
    REQUIRE((card.maxIterations == 100.0 && card.stiffnessSmoothing == 50.0 && card.cases.size() == 2));
    REQUIRE((card.cases[0][0] == 1800.0 && card.cases[0][2] == 642.5 && card.cases[0][6] == 1.0));
    REQUIRE((card.cases[1][0] == 3600.0 && card.cases[1][3] == 1.0));

    // The second card is the same (another RID).
    TiltingPadBearingPropCard fixed = cardAt<TiltingPadBearingPropTable>(model, 1);
    REQUIRE(fixed.id == 201);
    fixed.id = card.id;
    REQUIRE(fixed == card);
    requireRoundTrip(model);

    // Defaults are not written (the pivot line is blank, a line of its own that the writer marks with a plus sign), the
    // blank cells of the grid stay blank, and the text of a title keeps its blanks in the small format.

    // The RID is the number of the PBUSH2D and of the ELEMUDS: the three coexist, are resolved, and follow a
    // renumbering.
    const Visited visited = visitedBy<TiltingPadBearingPropTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"id"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    REQUIRE(unresolvedIn(model) == 1); // the second card names a PBUSH2D 201 that does not exist
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<TiltingPadBearingPropTable>(model, 0).id == 1200 && cardAt<UserElemServiceTable>(model).id == 1200));
    REQUIRE(model.table<TiltingPadBearingPropTable>().idAtRow(0) == 1200);
    REQUIRE_THROWS_AS(readBulk(TILTING_PAD_FIVE_PADS + TILTING_PAD_FIVE_PADS), base::DeckError);

    // The smallest card has one pad and one case, and the defaults of the format reference.
    const Model minimal = readBulk(smallThpad());
    const TiltingPadBearingPropCard least = cardAt<TiltingPadBearingPropTable>(minimal);
    REQUIRE((least.pads.size() == 1 && least.pads[0][0] == 1.0 && !least.pads[0][1].has_value()));
    REQUIRE(
        (least.cases.size() == 1 && least.cases[0][0] == 1.0 && least.title1.empty() && !least.timeLimit.has_value()));
    REQUIRE((least.pivotRotationStiffness == 0.0 && least.initialPivotDeflection == 0.0 &&
             least.hotOilCarryOver == 100.0 && least.coldOilCarryOver == 0.0));
    REQUIRE((least.maxIterations == 100.0 && !least.stiffnessSmoothing.has_value() && !least.elementCount.has_value() &&
             !least.journalThermalModel.has_value()));
    requireRoundTrip(minimal);

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    TiltingPadBearingPropTable& table = model.table<TiltingPadBearingPropTable>();
    TiltingPadBearingPropCard edited = table.card(0);
    edited.pads.push_back(edited.pads.back()); // a sixth pad moves every line that follows
    edited.cases[0][1] = 5.0;
    edited.hotOilCarryOver = 90.0;
    edited.title2 = "Another title";
    REQUIRE_FALSE(TiltingPadBearingPropFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE((cardAt<TiltingPadBearingPropTable>(model).pads.size() == 6 &&
             cardAt<TiltingPadBearingPropTable>(model).hotOilCarryOver == 90.0));
    requireRoundTrip(model);
    edited.pads.clear();
    REQUIRE(TiltingPadBearingPropFormat::check(edited).has_value()); // NPADS is the length of the list, at least one
    edited.pads.resize(1);
    edited.cases.clear();
    REQUIRE(TiltingPadBearingPropFormat::check(edited).has_value());
    edited.cases.resize(1);
    edited.title1 = "a, b";
    REQUIRE_FALSE(TiltingPadBearingPropFormat::check(edited).has_value());
    table.store(0, edited);
    requireRoundTrip(model);
    edited.title1 = std::string(65, 'x');
    REQUIRE_FALSE(TiltingPadBearingPropFormat::check(edited).has_value());
    table.store(0, edited);
    requireRoundTrip(model);
    edited.title1 = " leading blank";
    REQUIRE_FALSE(TiltingPadBearingPropFormat::check(edited).has_value());
    table.store(0, edited);
    requireRoundTrip(model);
    edited.title1.clear();
    edited.padDeformation = 3;
    REQUIRE(TiltingPadBearingPropFormat::check(edited).has_value());

    std::string zeroRid = smallThpad();
    zeroRid.replace(zeroRid.find(";1;"), 3, ";0;"); // the RID 1 becomes 0
    expectInvalid(zeroRid, "id");
    expectInvalid("TiltingPadBearingProp;1;x", "timeLimit");
    expectInvalid("TiltingPadBearingProp;1;;;;;;;;;;;;0", "padCount");
    expectInvalid("TiltingPadBearingProp;1", "padCount"); // required
    expectInvalid("TiltingPadBearingProp;1;;;;;;;;;;;;2000000000", "padCount");
    expectInvalid(smallThpad(",1,,2"), "eccentricityInput");
    expectInvalid(smallThpad(",1,-3"), "elementCount");
    expectInvalid(smallThpad(",1", ",x"), "padArc1");
    expectInvalid(smallThpad(",1", ",1.", ",", ",3"), "journalThermalModel");
    expectInvalid(smallThpad(",1", ",1.", ",", ",,3"), "inletThermalBoundary");
    expectInvalid(smallThpad(",1", ",1.", ",x"), "convergenceTolerance");
    expectInvalid(smallThpad(",1", ",1.", ",", ",", ",0", ","), "caseCount"); // no case line then
    expectInvalid(smallThpad(",1", ",1.", ",", ",", ",", ","), "caseCount");  // required
    expectInvalid(smallThpad(",1", ",1.", ",", ",", ",1,x"), "maxIterations");
    expectInvalid(smallThpad(",1", ",1.", ",", ",", ",1", ",x"), "speed1");
}

// ---------------------------------------------------------------------------------------------------------------------
// ACPEMCP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ACPEMCP names a set of grids for each interface and each zero constraint of a trim component",
          "[model][cards][acpemcp]")
{
    REQUIRE((TrimComponentCouplingTable::INFO.category == CardCategory::Other &&
             TrimComponentCouplingTable::INFO.idSpace == base::IdSpace::Group));
    // the sets are in the part of the trim component, [model;TrimComponent;TID].
    Model model = readBulk(
        "[model]\n"
        "TrimComponentCoupling;1;;31;;33\n"
        "TrimComponentCoupling;7;10;11;12;13;3;1;14;15;16;17;18;19;20;21;5;0.8;0.5\n"
        "TrimComponentCoupling;8\n"
        "TrimComponentCoupling;7;10;;;;;;;;;;;;;;2\n"
        "[model;TrimComponent;1]\n" +
        setCards(31, 33, "IdListSet1;", ";1;2\n") + "[model;TrimComponent;7]\n" +
        setCards(10, 21, "IdListSet1;", ";1;2\n"));
    const TrimComponentCouplingCard twoSets = cardAt<TrimComponentCouplingTable>(model, 0);
    REQUIRE((twoSets.id == 1 && twoSets.gluedSetId == 0 && twoSets.slidingSetId == 31 && twoSets.openSetId == 0 &&
             twoSets.imperviousSetId == 33));
    REQUIRE((twoSets.outOfCoreBlockCount == 1 && twoSets.singlePrecisionFlag == 0 && twoSets.regionId == 0));
    REQUIRE((twoSets.structureCompressionRatio == 1.0 && twoSets.cavityCompressionRatio == 1.0)); // the defaults
    const TrimComponentCouplingCard full = cardAt<TrimComponentCouplingTable>(model, 1);
    REQUIRE((full.id == 7 && full.gluedSetId == 10 && full.slidingSetId == 11 && full.openSetId == 12 &&
             full.imperviousSetId == 13));
    REQUIRE((full.outOfCoreBlockCount == 3 && full.singlePrecisionFlag == 1 && full.airGapSetId == 14));
    REQUIRE((full.fixedTranslationXSetId == 15 && full.fixedTranslationYSetId == 16 &&
             full.fixedTranslationZSetId == 17 && full.fixedRotationXSetId == 18 && full.fixedRotationYSetId == 19));
    REQUIRE((full.fixedRotationZSetId == 20 && full.fixedPressureSetId == 21 && full.regionId == 5 &&
             full.structureCompressionRatio == 0.8 && full.cavityCompressionRatio == 0.5));
    // A trim component has several cards, told apart by their region: the TID is shared.
    REQUIRE(cardAt<TrimComponentCouplingTable>(model, 3).id == 7);
    REQUIRE((cardAt<TrimComponentCouplingTable>(model, 3).regionId == 2 &&
             cardAt<TrimComponentCouplingTable>(model, 2).gluedSetId == 0));
    requireRoundTrip(model);

    // The twelve sets, in the order of the fields, each in the part of the trim component; a blank one is not visited.
    const PartReferenceRecorder visited = partReferencesOf<TrimComponentCouplingTable>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"gluedSetId", "slidingSetId", "openSetId", "imperviousSetId",
                                                       "airGapSetId", "fixedTranslationXSetId",
                                                       "fixedTranslationYSetId", "fixedTranslationZSetId",
                                                       "fixedRotationXSetId", "fixedRotationYSetId",
                                                       "fixedRotationZSetId", "fixedPressureSetId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(12, ReferenceTarget::Set));
    REQUIRE(visited.parts == std::vector<std::string>(12, "TrimComponent 7"));
    REQUIRE(partReferencesOf<TrimComponentCouplingTable>(model, 0).fields ==
            std::vector<std::string>{"slidingSetId", "imperviousSetId"});
    REQUIRE(partReferencesOf<TrimComponentCouplingTable>(model, 2).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    // A set the part does not have, or a part the deck does not have, is unresolved.
    Model missing = readBulk(
        "[model]\nTrimComponentCoupling;7;10;11;;;;;;12\nTrimComponentCoupling;9;10\n[model;TrimComponent;7]"
        "\nIdListSet1;10;1;2");
    REQUIRE(unresolvedIn(missing) == 3);

    // A renumbering of the sets of a trim part follows into the card; the trim component and its region are not ones.
    Renumbering::apply(*model.findPart("TrimComponent", 7), RenumberSpace::Set, Renumbering::offset(100));
    const TrimComponentCouplingCard moved = cardAt<TrimComponentCouplingTable>(model, 1);
    REQUIRE((moved.gluedSetId == 110 && moved.airGapSetId == 114 && moved.fixedRotationZSetId == 120 &&
             moved.fixedPressureSetId == 121));
    REQUIRE((moved.id == 7 && moved.regionId == 5 && moved.outOfCoreBlockCount == 3));
    REQUIRE(unresolvedIn(model) == 0);

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    TrimComponentCouplingTable& table = model.table<TrimComponentCouplingTable>();
    TrimComponentCouplingCard edited = table.card(2);
    edited.openSetId = 112;
    edited.structureCompressionRatio = 0.25;
    REQUIRE_FALSE(TrimComponentCouplingFormat::check(edited).has_value());
    table.store(2, edited);
    REQUIRE((cardAt<TrimComponentCouplingTable>(model, 2).openSetId == 112 &&
             cardAt<TrimComponentCouplingTable>(model, 2).structureCompressionRatio == 0.25));
    edited.structureCompressionRatio = 1.5;
    REQUIRE(TrimComponentCouplingFormat::check(edited).has_value());
    const std::size_t before = model.cardCount();
    model.remove(CardLocation{typeIndexOf<TrimComponentCouplingTable>(), 3});
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("TrimComponentCoupling;0", "id");
    expectInvalid("TrimComponentCoupling", "id"); // required
    expectInvalid("TrimComponentCoupling;1;-5", "gluedSetId");
    expectInvalid("TrimComponentCoupling;1;x", "gluedSetId");
    expectInvalid("TrimComponentCoupling;1;;;;;0", "outOfCoreBlockCount");
    expectInvalid("TrimComponentCoupling;1;;;;;;-1", "singlePrecisionFlag");
    expectInvalid("TrimComponentCoupling;1;;;;;;;;;;;;;;;-4", "regionId");
    expectInvalid("TrimComponentCoupling;1;;;;;;;;-3", "fixedTranslationXSetId");
    expectInvalid("TrimComponentCoupling;1;;;;;;;;;;;;;;;;0.", "structureCompressionRatio");
    expectInvalid("TrimComponentCoupling;1;;;;;;;;;;;;;;;;1.5", "structureCompressionRatio");
    expectInvalid("TrimComponentCoupling;1;;;;;;;;;;;;;;;;;-0.1", "cavityCompressionRatio");
}

// ---------------------------------------------------------------------------------------------------------------------
// ACPMCP1
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ACPMCP1 names a set of surfaces for each interface of a 1D analytical trim component",
          "[model][cards][acpmcp1]")
{
    REQUIRE((TrimComponentCoupling1DTable::INFO.category == CardCategory::Other &&
             TrimComponentCoupling1DTable::INFO.idSpace == base::IdSpace::Group));
    // The TypedListSet3 records list the grid points 1 and 2, which a SET3 names as nodes: they exist.
    Model model = readBulk(
        "[model]\n"
        "TrimComponentCoupling1D;1;30;;32;;;OneDimensional\n"
        "TrimComponentCoupling1D;7;10;11;12;13;3;OneDimensional\n"
        "TrimComponentCoupling1D;8\n"
        "TrimComponentCoupling1D;7;14;;;;2\n"
        "[model;TrimComponent;1]\n"
        "Node;1\n"
        "Node;2\n" +
        setCards(30, 32, "TypedListSet3;", ";Node;1;2\n") + "[model;TrimComponent;7]\nNode;1\nNode;2\n" +
        setCards(10, 14, "TypedListSet3;", ";Node;1;2\n"));
    // The format draws RID in the last cell of the second line and METHOD in the first of the third.
    const TrimComponentCoupling1DCard twoSets = cardAt<TrimComponentCoupling1DTable>(model, 0);
    REQUIRE((twoSets.id == 1 && twoSets.gluedSetId == 30 && twoSets.slidingSetId == 0 && twoSets.openSetId == 32 &&
             twoSets.imperviousSetId == 0));
    REQUIRE((twoSets.regionId == 0 && twoSets.method == TrimComponentCoupling1DMethod::OneDimensional));
    const TrimComponentCoupling1DCard full = cardAt<TrimComponentCoupling1DTable>(model, 1);
    REQUIRE((full.gluedSetId == 10 && full.slidingSetId == 11 && full.openSetId == 12 && full.imperviousSetId == 13 &&
             full.regionId == 3));
    REQUIRE(full.method == TrimComponentCoupling1DMethod::OneDimensional);            // any case
    REQUIRE_FALSE(cardAt<TrimComponentCoupling1DTable>(model, 2).method.has_value()); // a blank METHOD stays blank
    REQUIRE((cardAt<TrimComponentCoupling1DTable>(model, 3).id == 7 &&
             cardAt<TrimComponentCoupling1DTable>(model, 3).regionId == 2));
    requireRoundTrip(model);

    // The same card read alone.
    const Model fixed = readBulk("TrimComponentCoupling1D;1;30;;32;;;OneDimensional\n");
    REQUIRE(cardAt<TrimComponentCoupling1DTable>(fixed) == twoSets);

    const PartReferenceRecorder visited = partReferencesOf<TrimComponentCoupling1DTable>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>{"gluedSetId", "slidingSetId", "openSetId", "imperviousSetId"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::Set));
    REQUIRE(visited.parts == std::vector<std::string>(4, "TrimComponent 7"));
    REQUIRE(partReferencesOf<TrimComponentCoupling1DTable>(model, 0).fields ==
            std::vector<std::string>{"gluedSetId", "openSetId"});
    REQUIRE(partReferencesOf<TrimComponentCoupling1DTable>(model, 2).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(*model.findPart("TrimComponent", 7), RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE((cardAt<TrimComponentCoupling1DTable>(model, 1).gluedSetId == 110 &&
             cardAt<TrimComponentCoupling1DTable>(model, 1).imperviousSetId == 113));
    REQUIRE((cardAt<TrimComponentCoupling1DTable>(model, 1).id == 7 &&
             cardAt<TrimComponentCoupling1DTable>(model, 1).regionId == 3));
    REQUIRE(unresolvedIn(model) == 0);

    TrimComponentCoupling1DTable& table = model.table<TrimComponentCoupling1DTable>();
    TrimComponentCoupling1DCard edited = table.card(2);
    edited.method = TrimComponentCoupling1DMethod::OneDimensional;
    edited.openSetId = 112;
    REQUIRE_FALSE(TrimComponentCoupling1DFormat::check(edited).has_value());
    table.store(2, edited);
    REQUIRE((cardAt<TrimComponentCoupling1DTable>(model, 2).method.has_value() &&
             cardAt<TrimComponentCoupling1DTable>(model, 2).openSetId == 112));
    requireRoundTrip(model);
    edited.regionId = -1;
    REQUIRE(TrimComponentCoupling1DFormat::check(edited).has_value());

    expectInvalid("TrimComponentCoupling1D;0", "id");
    expectInvalid("TrimComponentCoupling1D;1;-1", "gluedSetId");
    expectInvalid("TrimComponentCoupling1D;1;;;;;-2", "regionId");
    expectInvalid("TrimComponentCoupling1D;1;;;;;;2", "methodName"); // the first cell of the third line
    expectInvalid("TrimComponentCoupling1D;1;;;;;;2D", "methodName");
}

// ---------------------------------------------------------------------------------------------------------------------
// TRMCPL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TRMCPL gives the tolerances of one interface coupling type, each projection defaulting to the one before",
          "[model][cards][trmcpl]")
{
    REQUIRE((TrimCouplingTolerancesTable::INFO.category == CardCategory::Other &&
             TrimCouplingTolerancesTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(
        "TrimCouplingTolerances;1;SolidGlued;0.15;4\n"
        "TrimCouplingTolerances;2;TrimSurface\n"
        "TrimCouplingTolerances;3;Interface;0.2;0.05;0.04;0.03;0.02;7\n"
        "TrimCouplingTolerances;4;SolidAirGap;;;;0.5\n"
        "TrimCouplingTolerances;3;SolidOpen\n"
        "TrimCouplingTolerances;3;SimplePerforation;;;0.5\n"
        "TrimCouplingTolerances;5;SolidSlide;;;;;;3\n");
    const TrimCouplingTolerancesCard glued = cardAt<TrimCouplingTolerancesTable>(model, 0);
    REQUIRE((glued.id == 1 && glued.couplingKind == TrimCouplingTolerancesType::Sglued &&
             glued.relativeTolerance == 0.15 && glued.regionId == 0));
    // GAPTOL2 to GAPTOL4 default to the tolerance before them: they are stored resolved and always written.
    REQUIRE((glued.gapTolerance1 == 4.0 && glued.gapTolerance2 == 4.0 && glued.gapTolerance3 == 4.0 &&
             glued.gapTolerance4 == 4.0));
    const TrimCouplingTolerancesCard defaults = cardAt<TrimCouplingTolerancesTable>(model, 1);
    REQUIRE((defaults.couplingKind == TrimCouplingTolerancesType::Trmsur && defaults.relativeTolerance == 0.1 &&
             defaults.gapTolerance1 == 0.01));
    REQUIRE((defaults.gapTolerance2 == 0.01 && defaults.gapTolerance3 == 0.01 && defaults.gapTolerance4 == 0.01));
    const TrimCouplingTolerancesCard full = cardAt<TrimCouplingTolerancesTable>(model, 2);
    REQUIRE((full.couplingKind == TrimCouplingTolerancesType::Interf && full.relativeTolerance == 0.2 &&
             full.gapTolerance1 == 0.05 && full.gapTolerance2 == 0.04));
    REQUIRE((full.gapTolerance3 == 0.03 && full.gapTolerance4 == 0.02 && full.regionId == 7));
    const TrimCouplingTolerancesCard chain = cardAt<TrimCouplingTolerancesTable>(model, 3);
    REQUIRE((chain.couplingKind == TrimCouplingTolerancesType::Sairgap && chain.gapTolerance1 == 0.01 &&
             chain.gapTolerance2 == 0.01));
    REQUIRE((chain.gapTolerance3 == 0.5 && chain.gapTolerance4 == 0.5));
    REQUIRE((cardAt<TrimCouplingTolerancesTable>(model, 5).gapTolerance1 == 0.01 &&
             cardAt<TrimCouplingTolerancesTable>(model, 5).gapTolerance2 == 0.5));
    REQUIRE((cardAt<TrimCouplingTolerancesTable>(model, 5).gapTolerance4 == 0.5 &&
             cardAt<TrimCouplingTolerancesTable>(model, 6).regionId == 3));
    // One card for each coupling type of a component: the TID is shared.
    REQUIRE((cardAt<TrimCouplingTolerancesTable>(model, 4).id == 3 &&
             cardAt<TrimCouplingTolerancesTable>(model, 5).id == 3));
    requireRoundTrip(model);

    (void)0; // PLTOL is the default

    // The card names no other card.
    REQUIRE(visitedBy<TrimCouplingTolerancesTable>(model, 2).fields.empty());
    REQUIRE(unresolvedIn(model) == 0);

    // The columns the query engine sees.
    const TrimCouplingTolerancesTable& table = model.table<TrimCouplingTolerancesTable>();
    REQUIRE((table.idAt(0) == 1 && table.idAt(2) == 3));
    TrimCouplingTolerancesTable& editable = model.table<TrimCouplingTolerancesTable>();
    REQUIRE((editable.idColumn().get(3) == 4 &&
             editable.couplingKindColumn().get(3) == static_cast<std::uint8_t>(TrimCouplingTolerancesType::Sairgap)));
    REQUIRE((editable.gapTolerance3Column().get(3) == 0.5 && editable.regionIdColumn().get(6) == 3));

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    TrimCouplingTolerancesCard edited = editable.card(1);
    edited.couplingKind = TrimCouplingTolerancesType::Simper;
    edited.relativeTolerance = 0.3;
    REQUIRE_FALSE(TrimCouplingTolerancesFormat::check(edited).has_value());
    editable.store(1, edited);
    REQUIRE((cardAt<TrimCouplingTolerancesTable>(model, 1).couplingKind == TrimCouplingTolerancesType::Simper &&
             cardAt<TrimCouplingTolerancesTable>(model, 1).relativeTolerance == 0.3));
    edited.gapTolerance3 = 0.0;
    REQUIRE(TrimCouplingTolerancesFormat::check(edited).has_value());

    expectInvalid("TrimCouplingTolerances;0;SolidGlued", "id");
    expectInvalid("TrimCouplingTolerances;1", "couplingKind"); // required
    expectInvalid("TrimCouplingTolerances;1;SGLUE", "couplingKind");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;0.", "relativeTolerance");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;;-1.", "gapTolerance1");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;;;0.", "gapTolerance2");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;;;;0.", "gapTolerance3");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;;;;;-2.", "gapTolerance4");
    expectInvalid("TrimCouplingTolerances;1;SolidGlued;;;;;;-1", "regionId");
}

// ---------------------------------------------------------------------------------------------------------------------
// TCOMPG
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TCOMPG lists the layers of a 1D trim from the bottom, a layer to a line, and is keyed by its SET3",
          "[model][cards][tcompg]")
{
    REQUIRE((TrimLayeredComponentTable::INFO.category == CardCategory::Other &&
             TrimLayeredComponentTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "TypedListSet3;10;Node;1\n"
        "TypedListSet3;11;Node;2\n"
        "IsoMat;101;2.1E+11;;0.3\n"
        "IsoMat;102;2.1E+11;;0.3\n"
        "TrimLayeredComponent;10;1001;101;0.02;;1002;102;0.03\n"
        "TrimLayeredComponent;11;1;101;0.035;1.;2;102;0.0015;0.;3;101;0.01;2.5\n");
    const TrimLayeredComponentCard plain = cardAt<TrimLayeredComponentTable>(model, 0);
    REQUIRE((plain.id == 10 && plain.layers.size() == 2));
    REQUIRE((plain.layers[0] == TrimLayeredComponentLayer{1001, 101, 0.02, 1.0})); // a blank SCALE is 1.0
    REQUIRE((plain.layers[1] == TrimLayeredComponentLayer{1002, 102, 0.03, 1.0}));
    const TrimLayeredComponentCard scaled = cardAt<TrimLayeredComponentTable>(model, 1);
    REQUIRE((scaled.layers.size() == 3 && scaled.layers[1].scaleFactor == 0.0 && scaled.layers[2].scaleFactor == 2.5));
    // A line with nothing on it is a gap, not a layer.
    const Model gap = readBulk("TrimLayeredComponent;12;1;101;0.02;;;;;;2;102;0.03\n");
    REQUIRE(cardAt<TrimLayeredComponentTable>(gap).layers.size() == 2);
    requireRoundTrip(model);

    // Two layers in fixed columns.
    const Model columns = readBulk("TrimLayeredComponent;10;1;101;0.035;1.;2;102;0.0015;0.\n");
    REQUIRE((cardAt<TrimLayeredComponentTable>(columns).layers.size() == 2 &&
             cardAt<TrimLayeredComponentTable>(columns).layers[1].thickness == 0.0015));

    // The SET3 and the materials are references; the SET3 is also the ID of the card.
    const Visited visited = visitedBy<TrimLayeredComponentTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"id", "plyMatId", "plyMatId"});
    REQUIRE(visited.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Set, ReferenceTarget::Material, ReferenceTarget::Material});
    REQUIRE(unresolvedIn(model) == 0);
    Model missing = readBulk("TrimLayeredComponent;10;1;101;0.02");
    REQUIRE(unresolvedIn(missing) == 2);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE((cardAt<TrimLayeredComponentTable>(model, 0).id == 110 &&
             cardAt<TrimLayeredComponentTable>(model, 1).id == 111));
    REQUIRE((model.table<TrimLayeredComponentTable>().idAtRow(0) == 110 &&
             model.table<TrimLayeredComponentTable>().idAtRow(1) == 111));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<TrimLayeredComponentTable>(model, 0).layers[0].matId == 1101 &&
             cardAt<TrimLayeredComponentTable>(model, 1).layers[2].matId == 1101));
    REQUIRE((cardAt<TrimLayeredComponentTable>(model, 0).layers[0].globalPlyId ==
             1001)); // the layer numbers are the user's own
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // The format only recommends one TCOMPG for each SET3: the ID is shared, not unique.
    REQUIRE_NOTHROW(readBulk("TrimLayeredComponent;10;1;101;0.02\nTrimLayeredComponent;10;1;101;0.03"));

    // What the Python layer does when it edits a card: the values are checked, then stored in the row.
    TrimLayeredComponentTable& table = model.table<TrimLayeredComponentTable>();
    TrimLayeredComponentCard edited = table.card(0);
    edited.layers.push_back(TrimLayeredComponentLayer{1003, 1102, 0.05, 0.5});
    REQUIRE_FALSE(TrimLayeredComponentFormat::check(edited).has_value());
    table.store(0, edited);
    REQUIRE((cardAt<TrimLayeredComponentTable>(model, 0).layers.size() == 3 &&
             cardAt<TrimLayeredComponentTable>(model, 0).layers[2].scaleFactor == 0.5));
    requireRoundTrip(model);
    edited.layers.push_back(TrimLayeredComponentLayer{1001, 1102, 0.05, 0.5}); // a global layer number twice
    REQUIRE(TrimLayeredComponentFormat::check(edited).has_value());
    edited.layers.pop_back();
    edited.layers.clear();
    REQUIRE(TrimLayeredComponentFormat::check(edited).has_value()); // a card needs a layer

    expectInvalid("TrimLayeredComponent;0;1;101;0.02", "id");
    expectInvalid("TrimLayeredComponent;1", "globalPlyId1"); // at least one layer
    expectInvalid("TrimLayeredComponent;1;0;101;0.02", "globalPlyId");
    expectInvalid("TrimLayeredComponent;1;1;0;0.02", "plyMatId");
    expectInvalid("TrimLayeredComponent;1;1;;0.02", "plyMatId");
    expectInvalid("TrimLayeredComponent;1;1;101;0.", "plyThickness");
    expectInvalid("TrimLayeredComponent;1;1;101", "plyThickness");
    expectInvalid("TrimLayeredComponent;1;1;101;0.02;-1.", "plyScaleFactor");
    expectInvalid("TrimLayeredComponent;1;1;101;0.02;;1;102;0.03", "globalPlyId"); // a ply ID once
    expectInvalid("TrimLayeredComponent;1;1;101;x", "plyThickness");
}
