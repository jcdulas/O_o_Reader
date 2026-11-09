// Checks of the beam, solid, spring and bush properties and of the MAT2 and MAT8 materials: their variants, defaults,
// errors and references.

#include "oor/model/cards/materials/anisoMat2D/AnisoMat2DTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"
#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
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

TEST_CASE("PBAR defaults every blank to 0.0 and keeps a blank shear factor apart", "[model][cards]")
{
    const Model model = readBulk("BarProp;1;5;3.4;2.6;4.81\nBarProp;2;5;3.4;2.6;4.81;;;;;;;;;;;0.5;0.6;0.1");
    const BarPropCard minimal = cardAt<BarPropTable>(model, 0);
    REQUIRE(minimal.area == 3.4);
    REQUIRE(minimal.torsionConstant == 0.0);
    REQUIRE(minimal.nonStructMass == 0.0);
    REQUIRE(minimal.stressPointC == std::array<double, 2>{});
    REQUIRE(minimal.shearFactor1 == BarPropCard::NO_SHEAR_FACTOR);
    REQUIRE(minimal.shearFactor2 == BarPropCard::NO_SHEAR_FACTOR);
    REQUIRE(minimal.productInertia12 == 0.0);
    const BarPropCard shear = cardAt<BarPropTable>(model, 1);
    REQUIRE(shear.shearFactor1 == 0.5);
    REQUIRE(shear.shearFactor2 == 0.6);
    REQUIRE(shear.productInertia12 == 0.1);
}

TEST_CASE("PBAR keeps K1 and K2 blank for no area, and refuses what the format reference refuses", "[model][cards]")
{
    REQUIRE_NOTHROW(readBulk("BarProp;1;5;0.;1.;1."));
    REQUIRE_THROWS_AS(readBulk("BarProp;1;5;0.;1.;1.;;;;;;;;;;;0.5"), base::DeckError);  // K1 with A = 0
    REQUIRE_THROWS_AS(readBulk("BarProp;1;5;1.;-1."), base::DeckError);                  // I1 negative
    REQUIRE_THROWS_AS(readBulk("BarProp;1;5;1.;1.;1.;;;;;;;;;;;-1."), base::DeckError);  // K1 negative
    REQUIRE_THROWS_AS(readBulk("BarProp;1;5;1.;1.;1.;;;;;;;;;;;;;2."), base::DeckError); // I12 too large
    REQUIRE_THROWS_AS(readBulk("BarProp;1;;1."), base::DeckError);                       // no material
}

TEST_CASE("the beam and solid properties refer to their material, the solid also to a system", "[model][cards]")
{
    Model model = readBulk(
        "BarProp;1;5;1.;1.;1.\nBarShapeProp;2;5;;Rod;1.\nBeamProp;3;5;1.;1.;1.\nSolidProp;4;5;6\nSolidProp;5;5;-1");
    RecordingVisitor bar;
    model.table<BarPropTable>().visitReferences(0, bar);
    REQUIRE(bar.fields == std::vector<std::string>{"matId"});
    REQUIRE(bar.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    RecordingVisitor rod;
    model.table<BarShapePropTable>().visitReferences(0, rod);
    REQUIRE(rod.fields == std::vector<std::string>{"matId"});
    RecordingVisitor beam;
    model.table<BeamPropTable>().visitReferences(0, beam);
    REQUIRE(beam.fields == std::vector<std::string>{"matId"});
    RecordingVisitor solid;
    model.table<SolidPropTable>().visitReferences(0, solid);
    REQUIRE(solid.fields == std::vector<std::string>{"matId", "matFrameId"});
    REQUIRE(solid.targets[1] == ReferenceTarget::Coordinate);
    RecordingVisitor elementSystem; // CORDM -1 is the element system, not a card
    model.table<SolidPropTable>().visitReferences(1, elementSystem);
    REQUIRE(elementSystem.fields == std::vector<std::string>{"matId"});
}

TEST_CASE("PBARL takes the number of dimensions of its section type, and NSM comes after the last", "[model][cards]")
{
    const std::vector<std::pair<std::string, std::size_t>> sections = {
        {"Rod", 1},       {"Tube", 2},      {"Tube2", 2},
        {"ISection", 6},  {"Channel", 4},   {"TSection", 4},
        {"Box", 4},       {"Bar", 2},       {"Cross", 4},
        {"HSection", 4},  {"TSection1", 4}, {"ISection1", 4},
        {"Channel1", 4},  {"ZSection", 4},  {"Channel2", 4},
        {"TSection2", 4}, {"Box1", 6},      {"Hexagon", 3},
        {"Hat", 4},       {"Hat1", 5},      {"DoubleBoxSection", 10},
        {"LSection", 4}};
    for (const auto& [type, count] : sections)
    {
        INFO(type);
        REQUIRE(BarShapePropFormat::dimensionCount(type) == count);
        // The card with `given` dimensions 1., 2., ... and then `extra` more values (NSM is the first of them).
        const auto text = [&type](std::size_t given, std::size_t extra)
        {
            std::string card = "BarShapeProp;1;1;;" + type;
            for (std::size_t index = 0; index < given + extra; ++index)
                card += index < given ? ";" + std::to_string(index + 1) + "." : ";0.5";
            return card;
        };
        const Model model = readBulk(text(count, 1));
        const BarShapePropCard card = cardAt<BarShapePropTable>(model);
        REQUIRE(card.kind == type);
        REQUIRE(card.dims.size() == count);
        REQUIRE(card.dims.back() == static_cast<double>(count));
        REQUIRE(card.nonStructMass == 0.5);
        // One dimension too few is an error, and so is one value too many.
        if (count > 1 && type != "DoubleBoxSection")
            REQUIRE_THROWS_AS(readBulk(text(count - 1, 0)), base::DeckError);
        REQUIRE_THROWS_AS(readBulk(text(count, 2)), base::DeckError);
    }
    REQUIRE(BarShapePropFormat::dimensionCount("NOPE") == 0);
}

TEST_CASE("PBARL reads the section type in any case and refuses other groups and blank dimensions", "[model][cards]")
{
    const Model model = readBulk("BarShapeProp;1;1;StandardSectionLibrary;Tube;2.;1.");
    REQUIRE(cardAt<BarShapePropTable>(model).kind == "Tube");
    REQUIRE_THROWS_AS(readBulk("BarShapeProp;1;1;MYLIB;Tube;2.;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("BarShapeProp;1;1;;Tube;2."), base::DeckError);     // DIM2 is required
    REQUIRE_THROWS_AS(readBulk("BarShapeProp;1;1;;Tube;2.;-1."), base::DeckError); // dimensions are positive
    REQUIRE_THROWS_AS(readBulk("BarShapeProp;1;1;;;2.;1."), base::DeckError);      // no TYPE
}

TEST_CASE("PBARL DBOX gives DIM5 to DIM10 their defaults", "[model][cards]")
{
    // DIM5 to DIM7 blank take DIM4, DIM8 is given, DIM9 blank takes DIM6 (which is DIM4), DIM10 is given.
    const Model model = readBulk("BarShapeProp;1;1;;DoubleBoxSection;10.;6.;0.5;0.4;;;;0.2;;0.3");
    const BarShapePropCard card = cardAt<BarShapePropTable>(model);
    REQUIRE(card.dims == std::vector<double>{10.0, 6.0, 0.5, 0.4, 0.4, 0.4, 0.4, 0.2, 0.4, 0.3});
    // Other sections have no such defaults.
    REQUIRE_THROWS_AS(readBulk("BarShapeProp;1;1;;Box;10.;6."), base::DeckError);
}

TEST_CASE("PBEAM resolves the blank values of its stations", "[model][cards]")
{
    // End A: A=2 I1=2 I2=4 J=1 NSM=0.5. Station at 0.5: only I1=3 given. End B: A=4 and J=3 given.
    const Model model = readBulk(
        "BeamProp;1;6;2.;2.;4.;0.;1.;0.5;;;;;;;;;YesSameAsEndA;0.5;;3.;;;;;Yes;1.;4.;;;;3.;;0.1;0.2;0.3;0.4;0.5;0.6;0."
        "7;0.8");
    const BeamPropCard card = cardAt<BeamPropTable>(model);
    REQUIRE(card.stations.size() == 3);
    const BeamPropStation& middle = card.stations[1];
    REQUIRE(middle.stressOutput == BeamStressOutput::YesA);
    REQUIRE(middle.stationRatio == 0.5);
    REQUIRE(middle.area == Approx(3.0));            // half way from 2 to 4
    REQUIRE(middle.bendingInertia1 == 3.0);         // given
    REQUIRE(middle.bendingInertia2 == Approx(4.0)); // end B is blank, so equal to end A
    REQUIRE(middle.torsionConstant == Approx(2.0)); // half way from 1 to 3
    REQUIRE(middle.nonStructMass == Approx(0.5));
    const BeamPropStation& endB = card.stations[2];
    REQUIRE(endB.area == 4.0);
    REQUIRE(endB.bendingInertia1 == 2.0); // the value at end A
    REQUIRE(endB.torsionConstant == 3.0);
    REQUIRE(endB.nonStructMass == 0.5);
    REQUIRE(endB.stressPointC == std::array<double, 2>{0.1, 0.2});
    REQUIRE(endB.stressPointF == std::array<double, 2>{0.7, 0.8});
}

TEST_CASE("PBEAM keeps the stations in the order of the card and interpolates by X/XB", "[model][cards]")
{
    const Model model = readBulk("BeamProp;1;6;2.;2.;4.;0.;1.;0.5;;;;;;;;;No;1.;4.;;;;;;No;0.25;;;;;;;No;0.75");
    const BeamPropCard card = cardAt<BeamPropTable>(model);
    REQUIRE(card.stations.size() == 4);
    REQUIRE(card.stations[2].stationRatio == 0.25);
    REQUIRE(card.stations[2].area == Approx(2.5));
    REQUIRE(card.stations[3].stationRatio == 0.75);
    REQUIRE(card.stations[3].area == Approx(3.5));
    REQUIRE(card.stations[3].bendingInertia1 == Approx(2.0));
}

TEST_CASE("PBEAM tells the line of stress points at end A from the first station", "[model][cards]")
{
    const Model withLine = readBulk("BeamProp;1;6;1.;1.;1.;;;;2.;-4.");
    REQUIRE(cardAt<BeamPropTable>(withLine).stations.front().stressOutput == BeamStressOutput::Yes);
    REQUIRE(cardAt<BeamPropTable>(withLine).stations.front().stressPointC == std::array<double, 2>{2.0, -4.0});
    const Model blankLine = readBulk("BeamProp;1;6;1.;1.;1.");
    // blank means zeros
    REQUIRE(cardAt<BeamPropTable>(blankLine).stations.front().stressOutput == BeamStressOutput::Yes);
    const Model left = readBulk("BeamProp;1;6;1.;1.;1.;;;;No;1.;2.");
    REQUIRE(cardAt<BeamPropTable>(left).stations.front().stressOutput == BeamStressOutput::No);
    REQUIRE(cardAt<BeamPropTable>(left).stations.size() == 2);
    // The line cannot be left out without stations after it: the footer would be read as the stress points.
    BeamPropCard alone;
    alone.id = 1;
    alone.matId = 1;
    alone.stations.front().area = 1.0;
    alone.stations.front().stressOutput = BeamStressOutput::No;
    REQUIRE(BeamPropFormat::check(alone).has_value());
}

TEST_CASE("PBEAM footer: K1 and K2 default to 1.0, the B values to those of end A", "[model][cards]")
{
    const Model model = readBulk("BeamProp;1;6;1.;1.;1.\nBeamProp;2;6;1.;1.;1.;;;;;;;;;;;;;2.;;0.1;;0.21;;0.5;;1.;2.");
    const BeamPropCard plain = cardAt<BeamPropTable>(model, 0);
    REQUIRE(plain.shearFactor1 == 1.0);
    REQUIRE(plain.shearFactor2 == 1.0);
    REQUIRE(plain.shearRelief1 == 0.0);
    REQUIRE(plain.nsmInertiaB == 0.0);
    const BeamPropCard footer = cardAt<BeamPropTable>(model, 1);
    REQUIRE(footer.shearFactor1 == 2.0);
    REQUIRE(footer.shearFactor2 == 1.0);
    REQUIRE(footer.shearRelief1 == 0.1);
    REQUIRE(footer.nsmInertiaA == 0.21);
    REQUIRE(footer.nsmInertiaB == 0.21); // blank NSI(B) is NSI(A)
    REQUIRE(footer.warpingCoefficientA == 0.5);
    REQUIRE(footer.warpingCoefficientB == 0.5);
    REQUIRE(footer.nsmCenterYA == 1.0);
    REQUIRE(footer.nsmCenterYB == 1.0);
    REQUIRE(footer.nsmCenterZB == 2.0);
    REQUIRE(footer.neutralAxisYB == 0.0);
}

TEST_CASE("PBEAM refuses what the format reference refuses", "[model][cards]")
{
    REQUIRE_NOTHROW(readBulk("BeamProp;1;6;1.;1.;1.;;;;;;;;;;;;Yes;1.;2."));
    REQUIRE_THROWS_AS(readBulk("BeamProp;1;6;1.;1.;1.;;;;;;;;;;;;Yes;0.5;2."), base::DeckError);             // no end B
    REQUIRE_THROWS_AS(readBulk("BeamProp;1;6;1.;1.;1.;;;;;;;;;;;;No;1.;2.;;;;;;No;1.;3."), base::DeckError); // repeated
    REQUIRE_THROWS_AS(readBulk("BeamProp;1;6;1.;1.;1.;;;;;;;;;;;;No;1.5;2."), base::DeckError);              // X/XB > 1
    REQUIRE_THROWS_AS(readBulk("BeamProp;1;6;1.;1.;1.;;;;;;;;;;;;No;0.;2.;;;;;;No;1.;3."), base::DeckError); // X/XB 0
    REQUIRE_THROWS_AS(readBulk("BeamProp;1;6;1.;1."), base::DeckError); // I2 is required
    // Ten stations after end A are the most.
    std::string stations = "BeamProp;1;6;1.;1.;1.;;;";
    for (int index = 1; index <= 9; ++index)
        stations += ";No;" + std::to_string(index * 0.1) + ";;;;;;";
    REQUIRE_NOTHROW(readBulk(stations + ";No;1.;2.\n"));
    REQUIRE_THROWS_AS(readBulk(stations + ";No;0.95;;;;;;;No;1.;2.\n"), base::DeckError);
}

TEST_CASE("PBEAM writes every value of its stations, so that they do not depend on the blank fields", "[model][cards]")
{
    const Model model = readBulk("BeamProp;1;6;2.;2.;4.;0.;1.;0.5;;;;;;;;;No;0.5;;;;;;;No;1.;4.;;;;3.");

    const Model again = readModelText(oor::testing::writeModelText(model), true);
    const BeamPropCard before = cardAt<BeamPropTable>(model);
    const BeamPropCard after = cardAt<BeamPropTable>(again);
    REQUIRE(after.stations.size() == 3);
    REQUIRE(before.stations[1].area == Approx(3.0));
    REQUIRE(after.stations[1].area == Approx(3.0));
    REQUIRE(after.stations[1].torsionConstant == Approx(2.0));
    // The middle station is written with its numbers: the text has no blank field left in it.

    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("PSOLID keeps the words, and reads integer codes and short spellings as the words", "[model][cards]")
{
    const Model model = readBulk(
        "SolidProp;1;5\nSolidProp;2;5;6;TwoPoints;Node;Reduced;Poroelastic\nSolidProp;3;5;-1;3;1;1;"
        "PressureFluid\nSolidProp;4;5;;0;0;0;SolidMechanics");
    const SolidPropCard minimal = cardAt<SolidPropTable>(model, 0);
    REQUIRE(minimal.matFrameId == 0);
    REQUIRE(minimal.integration.empty()); // blank: depends on the element, so not resolved
    REQUIRE(minimal.stressLocation.empty());
    REQUIRE(minimal.integrationOrder.empty());
    REQUIRE(minimal.elementFunction == "SolidMechanics");
    const SolidPropCard words = cardAt<SolidPropTable>(model, 1);
    REQUIRE(words.matFrameId == 6);
    REQUIRE(words.integration == "TwoPoints");
    REQUIRE(words.elementFunction == "Poroelastic");
    const SolidPropCard codes = cardAt<SolidPropTable>(model, 2);
    REQUIRE(codes.matFrameId == -1);
    REQUIRE(codes.integration == "ThreePoints");
    REQUIRE(codes.stressLocation == "Gauss");
    REQUIRE(codes.integrationOrder == "Full");
    REQUIRE(codes.elementFunction == "PressureFluid");
    const SolidPropCard zeros = cardAt<SolidPropTable>(model, 3);
    REQUIRE(zeros.integration == "BubbleFunctions");
    REQUIRE(zeros.stressLocation == "Node");
    REQUIRE(zeros.integrationOrder == "Reduced");
    REQUIRE(zeros.elementFunction == "SolidMechanics");
}

TEST_CASE("PSOLID refuses values its fields do not take", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("SolidProp;1;5;0;XX"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidProp;1;5;0;;XX"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidProp;1;5;0;;;XX"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidProp;1;5;0;;;;XX"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidProp;1;5;-3"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SolidProp;1"), base::DeckError); // MID is required
}

TEST_CASE("PELAS gives one property per spring, two for a card with two", "[model][cards]")
{
    const Model model = readBulk("SpringProp;7;5.36;0.04;6.15;23;3.08\nSpringProp;8;1.E+5");
    const SpringPropTable& table = *model.findTable<SpringPropTable>();
    REQUIRE(table.rows().aliveCount() == 3);
    REQUIRE(table.card(0).id == 7);
    REQUIRE(table.card(0).structuralDamping == 0.04);
    REQUIRE(table.card(0).stressCoefficient == 6.15);
    REQUIRE(table.card(1).id == 23);
    REQUIRE(table.card(1).stiffness == 3.08);
    REQUIRE(table.card(1).structuralDamping == 0.0);
    REQUIRE(table.card(2).stiffness == 1.0e5);
    REQUIRE(model.locate(base::IdSpace::Property, 23).has_value());
}

TEST_CASE("PELAS needs the ID and the stiffness of each spring, and shares the property IDs", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("SpringProp;1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringProp;1;1.;;;2"), base::DeckError);    // the second has no K2
    REQUIRE_THROWS_AS(readBulk("SpringProp;1;1.;;;;3."), base::DeckError);  // the second has no PID2
    REQUIRE_THROWS_AS(readBulk("SpringProp;1;1.;;;1;2."), base::DeckError); // the same ID twice
    REQUIRE_THROWS_AS(readBulk("SpringProp;1;1.\nShellProp;1;1;0.1"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("SpringProp;1;1.\nBushProp;1;Stiffness;1."), base::DeckError);
}

TEST_CASE("PBUSH reads its groups in any order and defaults what is not there", "[model][cards]")
{
    const Model model = readBulk(
        "BushProp;1;Stiffness;720.;3500.;510.;1.E+3;1.E+3;1.E+3;Damping;1.;2.\nBushProp;2;Damping;2.3\nBushProp;3");
    const BushPropCard both = cardAt<BushPropTable>(model, 0);
    REQUIRE(both.stiffness == std::array<double, 6>{720.0, 3500.0, 510.0, 1000.0, 1000.0, 1000.0});
    REQUIRE(both.damping == std::array<double, 6>{1.0, 2.0, 0.0, 0.0, 0.0, 0.0});
    REQUIRE(both.structuralDamping == std::array<double, 6>{});
    REQUIRE(both.stressFactorTranslational == 1.0);
    REQUIRE(both.strainFactorRotational == 1.0);
    REQUIRE(both.mass == 0.0);
    REQUIRE(cardAt<BushPropTable>(model, 1).stiffness == std::array<double, 6>{});
    REQUIRE(cardAt<BushPropTable>(model, 2).damping == std::array<double, 6>{});
    const Model reversed = readBulk("BushProp;1;StructuralDamping;0.05;;;;;;Mass;0.5;Stiffness;1.;2.;3.");
    REQUIRE(cardAt<BushPropTable>(reversed).mass == 0.5);
    REQUIRE(cardAt<BushPropTable>(reversed).stiffness[2] == 3.0);
}

TEST_CASE("PBUSH gives GE1 to all six directions when GE2 to GE6 are blank", "[model][cards]")
{
    const Model alone = readBulk("BushProp;1;Stiffness;1.;2.;3.;4.;5.;6.;StructuralDamping;0.05");
    REQUIRE(cardAt<BushPropTable>(alone).structuralDamping ==
            std::array<double, 6>{0.05, 0.05, 0.05, 0.05, 0.05, 0.05});
    // Any of GE2 to GE6 given, 0.0 included, makes the blank ones 0.0.
    const Model some = readBulk("BushProp;1;Stiffness;1.;2.;3.;4.;5.;6.;StructuralDamping;0.05;0.;0.02");
    REQUIRE(cardAt<BushPropTable>(some).structuralDamping == std::array<double, 6>{0.05, 0.0, 0.02, 0.0, 0.0, 0.0});
    const Model zeros = readBulk("BushProp;1;Stiffness;1.;2.;3.;4.;5.;6.;StructuralDamping;0.05;0.");
    REQUIRE(cardAt<BushPropTable>(zeros).structuralDamping == std::array<double, 6>{0.05, 0.0, 0.0, 0.0, 0.0, 0.0});
    // Written, GE keeps its six values, so that the card means the same when it is read again.

    const Model again = readModelText(oor::testing::writeModelText(zeros), true);
    REQUIRE(cardAt<BushPropTable>(again).structuralDamping == std::array<double, 6>{0.05, 0.0, 0.0, 0.0, 0.0, 0.0});
    REQUIRE(cardAt<BushPropTable>(readModelText(oor::testing::writeModelText(alone), true)).structuralDamping ==
            cardAt<BushPropTable>(alone).structuralDamping);
    oor::testing::requireModelFileRoundTrip(zeros);
    oor::testing::requireModelFileRoundTrip(alone);
}

TEST_CASE("PBUSH recovery coefficients default to 1.0 and thermal values to 0.0", "[model][cards]")
{
    const Model model =
        readBulk("BushProp;1;Stiffness;1.;;;;;;Recovery;6.4;;2.\nBushProp;2;Stiffness;1.;;;;;;Thermal;1.E-5;20.");
    const BushPropCard recovery = cardAt<BushPropTable>(model, 0);
    REQUIRE(recovery.stressFactorTranslational == 6.4);
    REQUIRE(recovery.stressFactorRotational == 1.0);
    REQUIRE(recovery.strainFactorTranslational == 2.0);
    REQUIRE(recovery.strainFactorRotational == 1.0);
    const BushPropCard thermal = cardAt<BushPropTable>(model, 1);
    REQUIRE(thermal.alphaCoefficient == 1.0e-5);
    REQUIRE(thermal.referenceTemperature == 20.0);
    REQUIRE(thermal.coincidentLength == 0.0);
}

TEST_CASE("PBUSH refuses a repeated group and a negative mass", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("BushProp;1;Stiffness;1.;;;;;;Stiffness;2."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("BushProp;1;Stiffness;1.;;;;;;Mass;-1."), base::DeckError); // negative mass
}

TEST_CASE("MAT2 reads the property matrix, the damping matrix and defaults the rest to 0.0", "[model][cards]")
{
    const Model model = readBulk(
        "AnisoMat2D;1;4700.;1800.;;4700.;;3600.\n"
        "AnisoMat2D;2;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;11.;12.;13.;14.;15.;16;17.;18.;19.;20.;21.;22.");
    const AnisoMat2DCard minimal = cardAt<AnisoMat2DTable>(model, 0);
    REQUIRE(minimal.stiffness == std::array<double, 6>{4700.0, 1800.0, 0.0, 4700.0, 0.0, 3600.0});
    REQUIRE(minimal.density == 0.0);
    REQUIRE(minimal.thermalExpansion == std::array<double, 3>{});
    REQUIRE(minimal.tensionAllowable == 0.0);
    REQUIRE(minimal.stressFrameId == 0);
    REQUIRE(minimal.structuralDampingTerms == std::array<double, 6>{});
    const AnisoMat2DCard full = cardAt<AnisoMat2DTable>(model, 1);
    REQUIRE(full.stiffness == std::array<double, 6>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    REQUIRE(full.density == 7.0);
    REQUIRE(full.thermalExpansion == std::array<double, 3>{8.0, 9.0, 10.0});
    REQUIRE(full.referenceTemperature == 11.0);
    REQUIRE(full.structuralDamping == 12.0);
    REQUIRE(full.tensionAllowable == 13.0);
    REQUIRE(full.compressionAllowable == 14.0);
    REQUIRE(full.shearAllowable == 15.0);
    REQUIRE(full.stressFrameId == 16);
    REQUIRE(full.structuralDampingTerms == std::array<double, 6>{17.0, 18.0, 19.0, 20.0, 21.0, 22.0});
}

TEST_CASE("MAT8 resolves Xc and Yc and keeps a blank transverse shear blank", "[model][cards]")
{
    const Model model = readBulk(
        "OrthoMat2D;1;2.4E+7;1.2E+6;0.28;1.6E+6;;;0.061;2.3E-5;1.2E-6;140.;1.2E+4;;300.;;1400.\n"
        "OrthoMat2D;2;2.4E+7;1.2E+6;0.28;1.6E+6;2.5E+6;1.1E+6;;;;;1.2E+4;1.7E+4;300.;900.");
    const OrthoMat2DCard blanks = cardAt<OrthoMat2DTable>(model, 0);
    REQUIRE(blanks.shearModulus12 == 1.6e6);
    REQUIRE_FALSE(blanks.shearModulus1Z.has_value()); // blank: an infinite modulus, not a number
    REQUIRE_FALSE(blanks.shearModulus2Z.has_value());
    REQUIRE(blanks.tensionStrength1 == 1.2e4);
    REQUIRE(blanks.compressionStrength1 == 1.2e4); // Xc defaults to Xt
    REQUIRE(blanks.tensionStrength2 == 300.0);
    REQUIRE(blanks.compressionStrength2 == 300.0); // Yc defaults to Yt
    REQUIRE(blanks.shearStrength == 1400.0);
    REQUIRE(blanks.strainAllowables == 0.0);
    REQUIRE_FALSE(blanks.hashinUnidirectional.has_value());
    const OrthoMat2DCard given = cardAt<OrthoMat2DTable>(model, 1);
    REQUIRE(given.shearModulus1Z == 2.5e6);
    REQUIRE(given.compressionStrength1 == 1.7e4);
    REQUIRE(given.compressionStrength2 == 900.0);
}

TEST_CASE("MAT8 reads the Hashin groups with their blank values resolved", "[model][cards]")
{
    const Model model = readBulk(
        "OrthoMat2D;1;2.4E+7;1.2E+6;0.28;;;;;;;;;;;;;;;;HashinCriterion;1010.;;33.;;1011.;;HashinTape;2001.;;35.;;2003."
        ";1.;1004.;1007."
        ";;HashinFabricFailure;3001.;;3003.;;1005.;;1006.");
    const OrthoMat2DCard card = cardAt<OrthoMat2DTable>(model);
    REQUIRE(card.hashinUnidirectional.has_value());
    REQUIRE(*card.hashinUnidirectional == std::array<double, 6>{1010.0, 1010.0, 33.0, 33.0, 1011.0, 1011.0});
    REQUIRE(card.hashinTape.has_value());
    // HT11 is given, HT12 repeats it.
    REQUIRE(*card.hashinTape == std::array<double, 9>{2001.0, 2001.0, 35.0, 35.0, 2003.0, 1.0, 1004.0, 1007.0, 1007.0});
    REQUIRE(card.hashinFabric.has_value());
    // HFB6 repeats HFB5, HFB11 and HFB12 repeat HFB10.
    REQUIRE(*card.hashinFabric ==
            std::array<double, 9>{3001.0, 3001.0, 3003.0, 3003.0, 1005.0, 1005.0, 1006.0, 1006.0, 1006.0});
    const Model without = readBulk("OrthoMat2D;1;2.4E+7;1.2E+6;0.28");
    REQUIRE_FALSE(cardAt<OrthoMat2DTable>(without).hashinUnidirectional.has_value());
    REQUIRE_FALSE(cardAt<OrthoMat2DTable>(without).hashinFabric.has_value());
}

TEST_CASE("MAT8 refuses what the format reference refuses", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("OrthoMat2D;1;;1.E+6"), base::DeckError);           // E1 is required
    REQUIRE_THROWS_AS(readBulk("OrthoMat2D;1;1.;-1."), base::DeckError);           // E2 positive
    REQUIRE_THROWS_AS(readBulk("OrthoMat2D;100000000;1.;1.E+6"), base::DeckError); // the ID is below 100,000,000
    // HF1 has no default; a field next to the groups that is not one of them is an error.
    REQUIRE_THROWS_AS(readBulk("OrthoMat2D;1;1.;1.;0.3;;;;;;;;;;;;;;;;HashinCriterion;;;33."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("OrthoMat2D;1;1.;1.;0.3;;;;;;;;;;;;;;;;;;5."), base::DeckError);
}

TEST_CASE("MAT1, MAT2 and MAT8 share one ID space, and a MAT2 refers to its system", "[model][cards]")
{
    REQUIRE_THROWS_AS(readBulk("IsoMat;5;2.E+11;;0.3\nAnisoMat2D;5;1."), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AnisoMat2D;5;1.\nOrthoMat2D;5;1.;1.;0.3"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("IsoMat;5;2.E+11;;0.3\nAnisoMat2D;6;1.\nOrthoMat2D;7;1.;1.;0.3"));
    Model model = readBulk("AnisoMat2D;1;1.;;;;;;;;;;;;;;;12\nAnisoMat2D;2;1.");
    RecordingVisitor withSystem;
    model.table<AnisoMat2DTable>().visitReferences(0, withSystem);
    REQUIRE(withSystem.fields == std::vector<std::string>{"stressFrameId"});
    REQUIRE(withSystem.targets[0] == ReferenceTarget::Coordinate);
    RecordingVisitor without;
    model.table<AnisoMat2DTable>().visitReferences(1, without);
    REQUIRE(without.fields.empty());
}
