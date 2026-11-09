// Checks of the remaining properties: the shared section catalogue of PBARL and PBEAML, PBEAML stations, the scalar
// element properties, the plane, solid, gap, fastener, weld, bend, bush, beam and composite properties, and the
// companions of a property (PELAST, PBUSHT, PDAMPT, PFASTT): variants, field positions, defaults, references and rules.

#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropTable.hpp"
#include "oor/model/cards/properties/beamProp3/BeamProp3Table.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/bushProp1D/BushProp1DTable.hpp"
#include "oor/model/cards/properties/bushPropFreq/BushPropFreqTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropTable.hpp"
#include "oor/model/cards/properties/damperProp/DamperPropTable.hpp"
#include "oor/model/cards/properties/damperPropFreq/DamperPropFreqTable.hpp"
#include "oor/model/cards/properties/fastenerProp/FastenerPropTable.hpp"
#include "oor/model/cards/properties/fastenerPropFreq/FastenerPropFreqTable.hpp"
#include "oor/model/cards/properties/gapProp/GapPropTable.hpp"
#include "oor/model/cards/properties/heatCapacityProp5/HeatCapacityProp5Table.hpp"
#include "oor/model/cards/properties/massProp/MassPropTable.hpp"
#include "oor/model/cards/properties/planePropNonlinear/PlanePropNonlinearTable.hpp"
#include "oor/model/cards/properties/sections/SectionCatalogue.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/model/cards/properties/solidPropHyperelastic/SolidPropHyperelasticTable.hpp"
#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"
#include "oor/model/cards/properties/springPropFreq/SpringPropFreqTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/model/cards/properties/viscousDamperProp/ViscousDamperPropTable.hpp"
#include "oor/model/cards/properties/weldProp/WeldPropTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
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

template <typename TableT>
std::size_t countOfCards(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().aliveCount();
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

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

struct SectionExpectation
{
    const char* type;
    std::size_t dimensions;
};

// The dimension count of every section type of the format reference (the format of PBEAML, which also has L).
constexpr SectionExpectation SECTIONS[] = {
    {"Rod", 1},       {"Tube", 2},
    {"Tube2", 2},     {"LSection", 4},
    {"ISection", 6},  {"Channel", 4},
    {"TSection", 4},  {"Box", 4},
    {"Bar", 2},       {"Cross", 4},
    {"HSection", 4},  {"TSection1", 4},
    {"ISection1", 4}, {"Channel1", 4},
    {"ZSection", 4},  {"Channel2", 4},
    {"TSection2", 4}, {"Box1", 6},
    {"Hexagon", 3},   {"Hat", 4},
    {"Hat1", 5},      {"DoubleBoxSection", 10},
};

std::string dimensionList(std::size_t count)
{
    std::string text;
    for (std::size_t index = 0; index < count; ++index)
        text += ";" + std::to_string(index + 1) + ".5";
    return text;
}

} // namespace

TEST_CASE("the section catalogue gives the dimension count of every section type to PBARL and PBEAML", "[model][cards]")
{
    REQUIRE(sections::standardSections().size() == std::size(SECTIONS));
    for (const SectionExpectation& expected : SECTIONS)
    {
        INFO(expected.type);
        REQUIRE(sections::dimensionCount(expected.type) == expected.dimensions);
        REQUIRE(BarShapePropFormat::dimensionCount(expected.type) == expected.dimensions);
        // PBARL and PBEAML accept the type with exactly that many dimensions.
        const std::string dims = dimensionList(expected.dimensions);
        Model model = readBulk(std::string("BarShapeProp;1;1;;") + expected.type + dims);
        REQUIRE(cardAt<BarShapePropTable>(model).dims.size() == expected.dimensions);
        Model beam = readBulk(std::string("BeamShapeProp;1;1;;") + expected.type + dims);
        const BeamShapePropCard card = cardAt<BeamShapePropTable>(beam);
        REQUIRE(card.stations.size() == 1);
        REQUIRE(card.stations[0].dims.size() == expected.dimensions);
        // One dimension too few is an error at end A, an unknown type is an error.
        if (expected.dimensions > 1 && std::string(expected.type) != "DoubleBoxSection")
            expectInvalid(std::string("BeamShapeProp;1;1;;") + expected.type + dimensionList(expected.dimensions - 1),
                          "dimension" + std::to_string(expected.dimensions));
    }
    REQUIRE(sections::dimensionCount("NOPE") == 0);
    REQUIRE(sections::findSection("NOPE") == nullptr);
    expectInvalid("BeamShapeProp;1;1;;NOPE;1.", "kind");
    expectInvalid("BeamShapeProp;1;1;;;1.", "kind");
}

TEST_CASE("a PBEAML has its stations in the fields that follow each other, and what a station leaves blank is resolved",
          "[model][cards]")
{
    // End A: I section (six dimensions) and NSM blank; station 1 at X/XB .5 with everything blank; end B.
    Model model =
        readBulk("BeamShapeProp;6207;1;;ISection;10.;5.;0.5;0.5;0.5;0.5;;;0.5;;;;;;;;Yes;1.;12.;6.;0.6;0.6;0.6;0.6");
    const BeamShapePropCard card = cardAt<BeamShapePropTable>(model);
    REQUIRE(card.id == 6207);
    REQUIRE(card.matId == 1);
    REQUIRE(card.kind == "ISection");
    REQUIRE(card.stations.size() == 3);
    REQUIRE(card.stations[0].stationRatio == 0.0);
    REQUIRE(card.stations[1].stationRatio == 0.5);
    REQUIRE(card.stations[1].stressOutput);
    REQUIRE(card.stations[1].dims == std::vector<double>{11.0, 5.5, 0.55, 0.55, 0.55, 0.55}); // as at end A
    REQUIRE(card.stations[2].dims == std::vector<double>{12.0, 6.0, 0.6, 0.6, 0.6, 0.6});
    requireRoundTrip(model);

    // The written card has end B and the station with every value: reading it gives the same stations.

    const Model again = readModelText(writeModelText(model), true);
    REQUIRE(cardAt<BeamShapePropTable>(again).stations[1].dims == card.stations[1].dims);
}

TEST_CASE("a blank NSM and blank dimensions at end B are the values of end A", "[model][cards]")
{
    Model model = readBulk("BeamShapeProp;1;1;;Box;6.;1.;0.1;0.1;0.5;Yes;1.");
    const BeamShapePropCard card = cardAt<BeamShapePropTable>(model);
    REQUIRE(card.stations.size() == 2);
    REQUIRE(card.stations[1].dims == card.stations[0].dims);
    REQUIRE(card.stations[1].nonStructMass == 0.5); // as at end A
    // A station without X/XB is at 1.0, one without SO outputs its stresses.
    Model defaults = readBulk("BeamShapeProp;2;1;;Rod;1.;0.;;;3.");
    REQUIRE(cardAt<BeamShapePropTable>(defaults).stations[1].stationRatio == 1.0);
    REQUIRE(cardAt<BeamShapePropTable>(defaults).stations[1].stressOutput);
    REQUIRE(cardAt<BeamShapePropTable>(defaults).stations[1].dims[0] == 3.0);
    Model silent = readBulk("BeamShapeProp;3;1;;Rod;1.;0.;No;1.;2.");
    REQUIRE_FALSE(cardAt<BeamShapePropTable>(silent).stations[1].stressOutput);
}

TEST_CASE("a DBOX section takes its default dimensions at every station and a PBEAML can have end A only",
          "[model][cards]")
{
    Model model = readBulk("BeamShapeProp;1;1;;DoubleBoxSection;10.;6.;0.5;0.5\nBeamShapeProp;2;1;;Rod;0.05");
    const BeamShapePropCard dbox = cardAt<BeamShapePropTable>(model, 0);
    REQUIRE(dbox.stations.size() == 1);
    REQUIRE(dbox.stations[0].dims == std::vector<double>{10.0, 6.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5});
    const BeamShapePropCard rod = cardAt<BeamShapePropTable>(model, 1);
    REQUIRE(rod.stations.size() == 1);
    REQUIRE(rod.stations[0].nonStructMass == 0.0);
    // The second station of a DBOX: DIM5 to DIM8 are its own DIM4, DIM9 and DIM10 its own DIM6.
    Model two =
        readBulk("BeamShapeProp;3;1;;DoubleBoxSection;10.;6.;0.5;0.5;0.4;0.3;0.2;0.1;0.2;0.3;0.;Yes;1.;12.;7.;0.6;0.7");
    const BeamShapePropCard card = cardAt<BeamShapePropTable>(two);
    REQUIRE(card.stations[1].dims == std::vector<double>{12.0, 7.0, 0.6, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7});
}

TEST_CASE("an invalid PBEAML is reported by field", "[model][cards]")
{
    expectInvalid("BeamShapeProp;1;1;;Rod;0.", "dimension1");
    expectInvalid("BeamShapeProp;1;1;;Rod", "dimension1");
    expectInvalid("BeamShapeProp;1;1;OTHER;Rod;1.", "sectionGroup");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;MAYBE;1.;2.", "stressOutput");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;YESA;1.;2.", "stressOutput");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;Yes;1.5;2.", "stationRatio");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;Yes;-0.5;2.", "stationRatio");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;Yes;0.5;2.;0.;Yes;0.5;3.", "stationRatio");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;Yes;0.5", "stationRatio");
    expectInvalid("BeamShapeProp;1;1;;Rod;1.;0.;Yes;1.;-2.", "dimension1");
    expectInvalid("BeamShapeProp;0;1;;Rod;1.", "id");
}

TEST_CASE("a PBEAML refuses what has no value: a card of 200 dimensions, a bad end A, a changed type", "[model][cards]")
{
    BeamShapePropCard card;
    card.id = 1;
    card.matId = 1;
    card.kind = "DoubleBoxSection";
    card.stations.clear();
    for (std::size_t index = 0; index < 19; ++index)
        card.stations.push_back(BeamShapePropStation{index == 0 ? 0.0 : 0.05 * static_cast<double>(index), true,
                                                     std::vector<double>(10, 1.0), 0.0});
    REQUIRE_FALSE(BeamShapePropFormat::check(card).has_value()); // 190 dimensions
    card.stations.push_back(BeamShapePropStation{1.0, true, std::vector<double>(10, 1.0), 0.0});
    REQUIRE(BeamShapePropFormat::check(card).has_value()); // 200: too many dimensions
    card.stations.pop_back();
    card.stations[0].stressOutput = false;
    REQUIRE(BeamShapePropFormat::check(card)->field == std::string("stressOutput"));
    card.stations[0].stressOutput = true;
    card.kind = "Rod"; // the dimensions are those of a DoubleBoxSection
    REQUIRE(BeamShapePropFormat::check(card)->field == std::string("dimension1"));
}

TEST_CASE("a PBEAML refers to its material, which renumbering follows", "[model][cards]")
{
    Model model = readBulk("IsoMat;5;2.1E+11;;0.3\nBeamShapeProp;1;5;;Rod;1.");
    RecordingVisitor visitor;
    model.table<BeamShapePropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<BeamShapePropTable>(model).matId == 105);
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("PSHEAR and PTUBE: fields, defaults that depend on other fields, and rules", "[model][cards]")
{
    Model model = readBulk(
        "ShearPanelProp;1;2;0.25;0.01;0.8;0.6\n"
        "ShearPanelProp;2;2;0.5\n"
        "TubeProp;3;2;5.48\n"
        "TubeProp;4;2;5.48;0.;0.1;4.5\n"
        "TubeProp;5;2;5.48;0.25");
    const ShearPanelPropCard shear = cardAt<ShearPanelPropTable>(model, 0);
    REQUIRE(shear.matId == 2);
    REQUIRE(shear.thickness == 0.25);
    REQUIRE(shear.nonStructMass == 0.01);
    REQUIRE(shear.extensionalFactor1 == 0.8);
    REQUIRE(shear.extensionalFactor2 == 0.6);
    REQUIRE(cardAt<ShearPanelPropTable>(model, 1).extensionalFactor1 == 0.0);
    // A blank T is half of OD (a solid rod), a blank OD2 is OD; both are stored resolved, known as blanks, and written
    // blank.
    REQUIRE(cardAt<TubePropTable>(model, 0).thickness == 2.74);
    REQUIRE(cardAt<TubePropTable>(model, 0).outerDiameter2 == 5.48);
    REQUIRE(!cardAt<TubePropTable>(model, 0).thicknessGiven);
    REQUIRE(!cardAt<TubePropTable>(model, 0).outerDiameter2Given);
    REQUIRE(cardAt<TubePropTable>(model, 1).thicknessGiven);
    REQUIRE(cardAt<TubePropTable>(model, 1).outerDiameter2Given);
    REQUIRE(cardAt<TubePropTable>(model, 1).thickness == 0.0); // zero is a solid rod too, not a blank
    REQUIRE(cardAt<TubePropTable>(model, 1).outerDiameter2 == 4.5);
    REQUIRE(cardAt<TubePropTable>(model, 2).outerDiameter2 == 5.48);

    requireRoundTrip(model);
    RecordingVisitor visitor;
    model.table<ShearPanelPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    expectInvalid("ShearPanelProp;1;2", "thickness");
    expectInvalid("ShearPanelProp;1;2;-0.1", "thickness");
    expectInvalid("ShearPanelProp;1;2;0.1;;-1.", "extensionalFactor1");
    expectInvalid("ShearPanelProp;1;0;0.1", "matId");
    expectInvalid("TubeProp;1;2", "outerDiameter");
    expectInvalid("TubeProp;1;2;0.", "outerDiameter");
    expectInvalid("TubeProp;1;2;1.;-0.1", "thickness");
    expectInvalid("TubeProp;1;2;1.;;;-1.", "outerDiameter2");
}

TEST_CASE("PDAMP and PMASS take up to four properties on a card, PVISC two with a blank field between them",
          "[model][cards]")
{
    Model model = readBulk(
        "DamperProp;1;3.4;2;5.2;3;1.5;4;0.5\nMassProp;11;3.86;12;11.7;13\nViscousDamperProp;21;1.;2.;22;3.;4."
        "\nViscousDamperProp;23;5.");
    REQUIRE(countOfCards<DamperPropTable>(model) == 4);
    REQUIRE(cardAt<DamperPropTable>(model, 3).id == 4);
    REQUIRE(cardAt<DamperPropTable>(model, 3).damping == 0.5);
    REQUIRE(countOfCards<MassPropTable>(model) == 3);
    REQUIRE(cardAt<MassPropTable>(model, 1).mass == 11.7);
    REQUIRE(cardAt<MassPropTable>(model, 2).id == 13);
    REQUIRE(cardAt<MassPropTable>(model, 2).mass == 0.0); // a blank mass
    REQUIRE(countOfCards<ViscousDamperPropTable>(model) == 3);
    REQUIRE(cardAt<ViscousDamperPropTable>(model, 1).id == 22);
    REQUIRE(cardAt<ViscousDamperPropTable>(model, 1).extensionalDamping == 3.0);
    REQUIRE(cardAt<ViscousDamperPropTable>(model, 1).rotationalDamping == 4.0);
    REQUIRE(cardAt<ViscousDamperPropTable>(model, 2).rotationalDamping == 0.0);
    // Each property is written as a card of its own: the same ones are read back.
    requireRoundTrip(model);

    expectInvalid("DamperProp;1", "damping1");
    expectInvalid("DamperProp;1;2.;3", "damping2");
    expectInvalid("MassProp;0;1.", "id");
    expectInvalid("ViscousDamperProp;1;1.;2.;3", "extensionalDamping2");
    expectInvalid("ViscousDamperProp;1", "extensionalDamping1");
}

TEST_CASE("PDAMP5 refers to its material, PLPLANE and PLSOLID to their material and PLPLANE to its system",
          "[model][cards]")
{
    Model model = readBulk(
        "HeatCapacityProp5;1;3;2.5\nPlanePropNonlinear;2;4;-2;Gauss\nPlanePropNonlinear;3;4;5\nPlanePropNonlinear;4;"
        "4\nSolidPropHyperelastic;5;6\nSolidPropHyperelastic;6;6;Gauss");
    REQUIRE(cardAt<HeatCapacityProp5Table>(model).heatCapacity == 2.5);
    REQUIRE(cardAt<PlanePropNonlinearTable>(model, 0).frameId == -2);
    REQUIRE(cardAt<PlanePropNonlinearTable>(model, 0).gaussOutput);
    REQUIRE_FALSE(cardAt<PlanePropNonlinearTable>(model, 1).gaussOutput);
    REQUIRE(cardAt<PlanePropNonlinearTable>(model, 2).frameId == 0);
    REQUIRE_FALSE(cardAt<SolidPropHyperelasticTable>(model, 0).gaussOutput);
    REQUIRE(cardAt<SolidPropHyperelasticTable>(model, 1).gaussOutput);
    // A negative CID is an axes combination and the basic system is 0: only CID 5 is a reference.
    RecordingVisitor negative;
    model.table<PlanePropNonlinearTable>().visitReferences(0, negative);
    REQUIRE(negative.fields == std::vector<std::string>{"matId"});
    RecordingVisitor system;
    model.table<PlanePropNonlinearTable>().visitReferences(1, system);
    REQUIRE(system.fields == std::vector<std::string>{"matId", "frameId"});
    REQUIRE(system.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Coordinate});
    requireRoundTrip(model);
    expectInvalid("HeatCapacityProp5;1;3;0.", "heatCapacity");
    expectInvalid("HeatCapacityProp5;1;0;4.", "matId");
    expectInvalid("PlanePropNonlinear;1;2;0;XXXX", "stressLocation");
    expectInvalid("PlanePropNonlinear;1", "matId");
    expectInvalid("SolidPropHyperelastic;1;2;GRAP", "stressLocation");
    expectInvalid("SolidPropHyperelastic;1", "matId");
}

TEST_CASE("PGAP stores the defaults that depend on other fields and writes them", "[model][cards]")
{
    Model model =
        readBulk("GapProp;1\nGapProp;2;;;1.E+5;;;0.3\nGapProp;3;0.035;1.5;1.E+6;0.002;0.35;0.3;0.4;0.2;40.;0.02");
    const GapPropCard blank = cardAt<GapPropTable>(model, 0);
    REQUIRE(blank.closedStiffness == 1.0e8);
    REQUIRE(blank.openStiffness == 1.0e-14 * GapPropCard::DEFAULT_CLOSED_STIFFNESS);
    REQUIRE(blank.transverseStiffness == 0.0);
    REQUIRE(blank.maxAdjustmentRatio == 100.0);
    REQUIRE(blank.minPenetrationRatio == 0.001);
    const GapPropCard defaults = cardAt<GapPropTable>(model, 1);
    REQUIRE(defaults.openStiffness == 1.0e-14 * 1.0e5);
    REQUIRE(defaults.transverseStiffness == 0.3 * 1.0e5);
    REQUIRE(defaults.frictionCoeff2 == 0.3);
    const GapPropCard full = cardAt<GapPropTable>(model, 2);
    REQUIRE(full.initialOpening == 0.035);
    REQUIRE(full.preload == 1.5);
    REQUIRE(full.openStiffness == 2.0e-3);
    REQUIRE(full.transverseStiffness == 0.35);
    REQUIRE(full.frictionCoeff2 == 0.4);
    REQUIRE(full.maxPenetration == 0.2);
    REQUIRE(full.maxAdjustmentRatio == 40.0);
    REQUIRE(full.minPenetrationRatio == 0.02);
    requireRoundTrip(model);
    expectInvalid("GapProp;1;;-1.", "preload");
    expectInvalid("GapProp;1;;;-1.", "closedStiffness");
    expectInvalid("GapProp;1;;;1.;-1.", "openStiffness");
    expectInvalid("GapProp;1;;;1.;;-1.", "transverseStiffness");
    expectInvalid("GapProp;1;;;;;1.;-0.1", "frictionCoeff1");
    expectInvalid("GapProp;1;;;;;;;;;0.5", "maxAdjustmentRatio");
    expectInvalid("GapProp;1;;;;;;;;;;2.", "minPenetrationRatio");
}

TEST_CASE("PFAST has two lines of fields and a system that is a reference when it is positive", "[model][cards]")
{
    Model model = readBulk(
        "FastenerProp;1;3.6;-1;;0.;183250.;19730.;100.;1.E+9;1.E+9;0.5;0.01;1.E-5;20.;0.1\n"
        "FastenerProp;2;3.6;5;1;1.;2.;3.\n"
        "FastenerProp;3;3.6;;;1.;2.;3.");
    const FastenerPropCard first = cardAt<FastenerPropTable>(model, 0);
    REQUIRE(first.diameter == 3.6);
    REQUIRE(first.matFrameId == -1);
    REQUIRE(first.translationalStiffness == std::array<double, 3>{0.0, 183250.0, 19730.0});
    REQUIRE(first.rotationalStiffness == std::array<double, 3>{100.0, 1.0e9, 1.0e9});
    REQUIRE(first.mass == 0.5);
    REQUIRE(first.structuralDamping == 0.01);
    REQUIRE(first.alphaCoefficient == 1.0e-5);
    REQUIRE(first.referenceTemperature == 20.0);
    REQUIRE(first.coincidentLength == 0.1);
    REQUIRE(cardAt<FastenerPropTable>(model, 1).matFrameId == 5);
    REQUIRE(cardAt<FastenerPropTable>(model, 1).absoluteFrame == 1);
    REQUIRE(cardAt<FastenerPropTable>(model, 2).matFrameId == -1); // blank: no system
    RecordingVisitor none;
    model.table<FastenerPropTable>().visitReferences(0, none);
    REQUIRE(none.fields.empty());
    RecordingVisitor system;
    model.table<FastenerPropTable>().visitReferences(1, system);
    REQUIRE(system.fields == std::vector<std::string>{"matFrameId"});
    REQUIRE(system.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    requireRoundTrip(model);
    expectInvalid("FastenerProp;1;0.;;;1.;2.;3.", "diameter");
    expectInvalid("FastenerProp;1;1.;;;1.;2.", "translationalStiffness3");
    expectInvalid("FastenerProp;1;1.;-2;;1.;2.;3.", "matFrameId");
    expectInvalid("FastenerProp;1;1.;;2;1.;2.;3.", "absoluteFrame");
    expectInvalid("FastenerProp;1;1.;;;1.;2.;3.;;;;;;;;-1.", "coincidentLength");
}

TEST_CASE("PBEND has two forms, told by field 3, and refuses the fields of the other one", "[model][cards]")
{
    Model model = readBulk(
        "CurvedBeamProp;1;1;0.7;0.06;0.03;0.05;12.;0.4;0.3;-0.4;0.3;0.5;0.7;0.2;;;0.7;0.7;;;;0.2\n"
        "CurvedBeamProp;2;1;1;0.5;0.02;10.;10.;0.1;0.1;0.2;0.3\n"
        "CurvedBeamProp;3;1;0.7;0.06;0.03;0.05;0.;45.\n"
        "CurvedBeamProp;4;1;3;0.5;0.02");
    const CurvedBeamPropCard general = cardAt<CurvedBeamPropTable>(model, 0);
    REQUIRE(general.form == CurvedBeamPropForm::Section);
    REQUIRE(general.area == 0.7);
    REQUIRE(general.bendRadius == 12.0);
    REQUIRE(general.arcAngle == 0.4);
    REQUIRE(general.stressPoints == std::array<double, 8>{0.3, -0.4, 0.3, 0.5, 0.7, 0.2, 0.0, 0.0});
    REQUIRE(general.shearFactor1 == 0.7);
    REQUIRE(general.neutralAxisOffset == 0.2);
    const CurvedBeamPropCard pipe = cardAt<CurvedBeamPropTable>(model, 1);
    REQUIRE(pipe.form == CurvedBeamPropForm::Pipe);
    REQUIRE(pipe.flexibilityOption == 1);
    REQUIRE(pipe.meanRadius == 0.5);
    REQUIRE(pipe.thickness == 0.02);
    REQUIRE(pipe.internalPressure == 10.0);
    REQUIRE(pipe.nonStructMass == 0.1);
    REQUIRE(pipe.centroidRadialOffset == 0.2);
    REQUIRE(pipe.centerZ == 0.3);
    // RB and THETAB are left to the CBEND when blank; a deck can write RB as 0.0.
    const CurvedBeamPropCard zero = cardAt<CurvedBeamPropTable>(model, 2);
    REQUIRE(zero.bendRadius == 0.0);
    REQUIRE(zero.arcAngle == 45.0);
    const CurvedBeamPropCard blank = cardAt<CurvedBeamPropTable>(model, 3);
    REQUIRE(std::isnan(blank.bendRadius));
    REQUIRE(std::isnan(blank.arcAngle));
    REQUIRE(std::isnan(blank.internalPressure));
    REQUIRE(std::isnan(
        cardAt<CurvedBeamPropTable>(model, 2).neutralAxisOffset)); // blank: the formula of the format reference
    requireRoundTrip(model);
    // The checks of the values of the other form.
    CurvedBeamPropCard card = general;
    card.flexibilityOption = 2;
    REQUIRE(CurvedBeamPropFormat::check(card).has_value());
    card = pipe;
    card.area = 1.0;
    REQUIRE(CurvedBeamPropFormat::check(card).has_value());
    expectInvalid("CurvedBeamProp;1;1;0.8;0.07;0.04", "torsionConstant");
    expectInvalid("CurvedBeamProp;1;1;-0.8;0.07;0.04;0.04", "area");
    expectInvalid("CurvedBeamProp;1;1;0.8;0.07;0.04;0.04;-1.", "bendRadius");
    expectInvalid("CurvedBeamProp;1;1;0.8;0.07;0.04;0.04;;;;;;;;;;;-0.1", "shearFactor1");
    expectInvalid("CurvedBeamProp;1;1;4;0.5;0.02", "flexibilityOption");
    expectInvalid("CurvedBeamProp;1;1;1;0.;0.02", "meanRadius");
    expectInvalid("CurvedBeamProp;1;1;2;0.5;0.", "flexibilityOption");
    expectInvalid("CurvedBeamProp;1;1", "area");
}

TEST_CASE("PBUSH1D continuations start their lines in any order and default what depends on other fields",
          "[model][cards]")
{
    Model model = readBulk(
        "BushProp1D;1;2500.;150.;350.;0.1;0.2\n"
        "BushProp1D;2;1.E+5;1.E+3;;;;Damper;Table;206;207;;;Spring;Table;205\n"
        "BushProp1D;3;3.E+3;;;;;ShockAbsorber;Equation;1.8;;1.;;;7301;;7302;;General;;7405;;7406;;7407\n"
        "BushProp1D;4;2500.;150.;350.;;;ShockAbsorber;Table;1.8;1.4;1.;;8213");
    const BushProp1DCard linear = cardAt<BushProp1DTable>(model, 0);
    REQUIRE(linear.stiffness == 2500.0);
    REQUIRE(linear.stressFactor == 0.1);
    REQUIRE(linear.strainFactor == 0.2);
    REQUIRE_FALSE(linear.shock.present);
    const BushProp1DCard curves = cardAt<BushProp1DTable>(model, 1);
    REQUIRE(curves.spring.present);
    REQUIRE(curves.spring.type == BushProp1DDefinition::Table);
    REQUIRE(curves.spring.tensionCurveId == 205);
    REQUIRE(curves.spring.compressionCurveId == 205); // blank IDC is IDT
    REQUIRE(curves.damper.tensionCurveId == 206);
    REQUIRE(curves.damper.compressionCurveId == 207);
    const BushProp1DCard equations = cardAt<BushProp1DTable>(model, 2);
    REQUIRE(equations.shock.type == BushProp1DDefinition::Equation);
    REQUIRE(equations.shock.dampingTension == 1.8);
    REQUIRE(equations.shock.dampingCompression == 1.8); // blank CVC is CVT
    REQUIRE(equations.shock.velocityExponentTension == 1.0);
    REQUIRE(equations.shock.velocityExponentCompression == 1.0); // blank EXPVC is EXPVT
    REQUIRE(equations.shock.scaleTensionEquationId == 7301);
    REQUIRE(equations.shock.scaleCompressionEquationId == 7301); // blank IDECS is IDETS
    REQUIRE(equations.shock.scaleTensionDerivativeEquationId == 7302);
    REQUIRE(equations.shock.scaleCompressionDerivativeEquationId == 7302);
    REQUIRE(equations.shock.scaleTableId == 0);
    REQUIRE(equations.general.present);
    REQUIRE(equations.general.tensionCurveId == 7405);
    REQUIRE(equations.general.compressionCurveId == 7405);
    REQUIRE(equations.general.tensionDisplacementDerivativeId == 7406);
    REQUIRE(equations.general.compressionVelocityDerivativeId == 7407);
    const BushProp1DCard table = cardAt<BushProp1DTable>(model, 3);
    REQUIRE(table.shock.scaleTableId == 8213);
    REQUIRE(table.shock.dampingCompression == 1.4);
    REQUIRE(table.shock.velocityExponentCompression == 1.0);
    requireRoundTrip(model);

    // The IDs of a table are TABLEDi entries, those of an equation DEQATN entries.
    RecordingVisitor visitor;
    model.table<BushProp1DTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"tensionCurveId", "compressionCurveId", "tensionCurveId", "compressionCurveId"});
    for (const ReferenceTarget target : visitor.targets)
        REQUIRE(target == ReferenceTarget::DynamicTable);
    RecordingVisitor equation;
    model.table<BushProp1DTable>().visitReferences(2, equation);
    REQUIRE_FALSE(equation.targets.empty());
    for (const ReferenceTarget target : equation.targets)
        REQUIRE(target == ReferenceTarget::DesignEquation);
}

TEST_CASE("an invalid PBUSH1D is reported by field", "[model][cards]")
{
    expectInvalid("BushProp1D;0;1.", "id");
    expectInvalid("BushProp1D;1;-1.", "stiffness");
    expectInvalid("BushProp1D;1;1.;;;;;Spring;Table;5;;;;Spring;Table;6", "ShockAbsorber");
    expectInvalid("BushProp1D;1;1.;;;;;Spring;CURVE;5", "kind");
    expectInvalid("BushProp1D;1;1.;;;;;Spring;Table", "tensionCurveId");
    expectInvalid("BushProp1D;1;1.;;;;;Spring;Equation;5", "tensionDisplacementDerivativeId");
    expectInvalid("BushProp1D;1;1.;;;;;Spring;Table;5;;7", "tensionDisplacementDerivativeId");
    expectInvalid("BushProp1D;1;1.;;;;;ShockAbsorber;Table;1.", "scaleTableId");
    expectInvalid("BushProp1D;1;1.;;;;;ShockAbsorber;Equation;1.;;;;5;5", "scaleTensionDerivativeEquationId");
    expectInvalid("BushProp1D;1;1.;;;;;General;Table;5;;6;;7", "kind");
    expectInvalid("BushProp1D;1;1.;;;;;General;;5;;6", "tensionVelocityDerivativeId");
}

TEST_CASE("the companions of a property have an ID space of their own and refer to the property they extend",
          "[model][cards]")
{
    Model model = readBulk(
        "SpringProp;10;1.\n"
        "SpringPropFreq;10;201;202;203\n"
        "DamperProp;20;1.\n"
        "DamperPropFreq;20;184\n"
        "BushProp;30;Stiffness;1.\n"
        "BushPropFreq;30;Stiffness;101\n"
        "FastenerProp;40;1.;;;1.;2.;3.\n"
        "FastenerPropFreq;40;Stiffness;51\n"
        "SpringProp;11;2.\n");
    // The same ID in the property space and in the space of each companion: no clash.
    REQUIRE(countOfCards<SpringPropFreqTable>(model) == 1);
    REQUIRE(countOfCards<DamperPropFreqTable>(model) == 1);
    REQUIRE(countOfCards<BushPropFreqTable>(model) == 1);
    REQUIRE(countOfCards<FastenerPropFreqTable>(model) == 1);
    RecordingVisitor visitor;
    model.table<SpringPropFreqTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"id", "stiffnessTableId", "structuralDampingTableId", "nonlinearForceTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    // The properties exist, the tables do not: one table of each companion but the PDAMPT's is missing... all five.
    REQUIRE(unresolvedIn(model) == 3 + 1 + 1 + 1);
    // Renumbering the properties follows into the companions, and their own IDs are still those of the properties.
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<SpringPropFreqTable>(model).id == 1010);
    REQUIRE(cardAt<DamperPropFreqTable>(model).id == 1020);
    REQUIRE(cardAt<BushPropFreqTable>(model).id == 1030);
    REQUIRE(cardAt<FastenerPropFreqTable>(model).id == 1040);
    REQUIRE(model.locateInType(typeIndexOf<SpringPropFreqTable>(), 1010).has_value());
    // Renumbering the tables follows too.
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(5000));
    REQUIRE(cardAt<SpringPropFreqTable>(model).stiffnessTableId == 5201);
    REQUIRE(cardAt<SpringPropFreqTable>(model).nonlinearForceTableId == 5203);
    REQUIRE(cardAt<DamperPropFreqTable>(model).dampingTableId == 5184);
    REQUIRE(cardAt<BushPropFreqTable>(model).stiffnessTableIds[0] == 5101);
    REQUIRE(cardAt<FastenerPropFreqTable>(model).stiffnessTableIds[0] == 5051);
    // Two companions of one property are a duplicate ID.
    REQUIRE_THROWS_AS(readBulk("SpringPropFreq;10;201\nSpringPropFreq;10;202"), base::DeckError);
}

TEST_CASE("PELAST and PDAMPT have blank tables and refuse negative ones", "[model][cards]")
{
    Model model = readBulk("SpringPropFreq;1\nSpringPropFreq;2;;;42\nDamperPropFreq;3\nDamperPropFreq;4;5");
    REQUIRE(cardAt<SpringPropFreqTable>(model, 0).stiffnessTableId == 0);
    REQUIRE(cardAt<SpringPropFreqTable>(model, 1).nonlinearForceTableId == 42);
    REQUIRE(cardAt<DamperPropFreqTable>(model, 1).dampingTableId == 5);
    requireRoundTrip(model);
    expectInvalid("SpringPropFreq;0", "id");
    expectInvalid("SpringPropFreq;1;-1", "stiffnessTableId");
    expectInvalid("DamperPropFreq;1;-5", "dampingTableId");
}

TEST_CASE("PBUSHT lines: tables, the damping table of all directions, and the fuse lines", "[model][cards]")
{
    Model model = readBulk(
        "BushPropFreq;1;Stiffness;101;102;103;104;105;106\n"
        "BushPropFreq;2;Stiffness;72;;;;;;Damping;18;;;;;;StructuralDamping;107\n"
        "BushPropFreq;3;StructuralDamping;107\n"
        "BushPropFreq;4;StructuralDamping;107;0;104;107;105;107\n"
        "BushPropFreq;5;StructuralDamping;0\n"
        "BushPropFreq;6;NonlinearForce;201;;;;;;;TransverseXy;1;12;UltimateLoad;0.5;2.5;0.001;2\n"
        "BushPropFreq;7;NonlinearForce;5;;;;;;;NonlinearResponse;2;631;ActualUltimateLoad\n"
        "BushPropFreq;8;Damping;2;3\n");
    REQUIRE(cardAt<BushPropFreqTable>(model, 0).stiffnessTableIds ==
            std::array<base::EntityId, 6>{101, 102, 103, 104, 105, 106});
    const BushPropFreqCard groups = cardAt<BushPropFreqTable>(model, 1);
    REQUIRE(groups.stiffnessTableIds[0] == 72);
    REQUIRE(groups.dampingTableIds[0] == 18);
    // one TGEID alone is the table of all six, a TGEID given with the others (a 0 too) is not.
    REQUIRE(groups.structuralDampingTableIds == std::array<base::EntityId, 6>{107, 107, 107, 107, 107, 107});
    REQUIRE(cardAt<BushPropFreqTable>(model, 2).structuralDampingTableIds ==
            std::array<base::EntityId, 6>{107, 107, 107, 107, 107, 107});
    REQUIRE(cardAt<BushPropFreqTable>(model, 3).structuralDampingTableIds ==
            std::array<base::EntityId, 6>{107, 0, 104, 107, 105, 107});
    REQUIRE(cardAt<BushPropFreqTable>(model, 4).structuralDampingTableIds == std::array<base::EntityId, 6>{});
    const BushPropFreqCard fuse = cardAt<BushPropFreqTable>(model, 5);
    REQUIRE(fuse.nonlinearForceTableIds[0] == 201);
    REQUIRE(fuse.forceDeflectionRule == ForceDeflectionRule::Trxy);
    REQUIRE(fuse.fuseMode == 1);
    REQUIRE(fuse.direction == 12);
    REQUIRE(fuse.fuseOption == FuseOption::UltimateLoad);
    REQUIRE(fuse.lowerBound == 0.5);
    REQUIRE(fuse.upperBound == 2.5);
    REQUIRE(fuse.fuseStiffnessRetention == 1.0e-3);
    REQUIRE(fuse.largeRotationEndA == 2);
    const BushPropFreqCard actual = cardAt<BushPropFreqTable>(model, 6);
    REQUIRE(actual.forceDeflectionRule == ForceDeflectionRule::None); // NR
    REQUIRE(actual.direction == 136);                                 // 631, in ascending order
    REQUIRE(actual.fuseOption == FuseOption::ActualUltimateLoad);
    REQUIRE(actual.fuseStiffnessRetention == BushPropFreqCard::DEFAULT_FUSE_STIFFNESS_RETENTION);
    REQUIRE(cardAt<BushPropFreqTable>(model, 7).dampingTableIds[1] == 3);
    requireRoundTrip(model);

    // Where the fuse fields are written: field 3 of the line after the last group.
    BushPropFreqCard written;
    written.id = 9;
    written.stiffnessTableIds[0] = 7;
    written.forceDeflectionRule = ForceDeflectionRule::Ts;
    written.fuseMode = 2;
    written.largeRotationEndA = 1;
    Model built;
    built.add<BushPropFreqTable>(written);

    oor::testing::requireModelFileRoundTrip(built);
    const Model again = readModelText(writeModelText(built), true);
    REQUIRE(cardAt<BushPropFreqTable>(again).forceDeflectionRule == ForceDeflectionRule::Ts);
    REQUIRE(cardAt<BushPropFreqTable>(again).largeRotationEndA == 1);
}

TEST_CASE("an invalid PBUSHT or PFASTT is reported by field", "[model][cards]")
{
    expectInvalid("BushPropFreq;0;Stiffness;1", "id");
    expectInvalid("BushPropFreq;1;Stiffness;1;;;;;;Stiffness;2", "id");
    expectInvalid("BushPropFreq;1;Stiffness;-1", "stiffnessTableId1");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;TXY", "forceDeflectionRule");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;;3", "fuseMode");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;;;77", "direction");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;;;;FAST", "fuseOption");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;;;;;;;0.", "fuseStiffnessRetention");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;;;;;;;;-1", "largeRotationEndA");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;TransverseShear;;;;;;;;;;;;5", "largeRotationEndA");
    expectInvalid("BushPropFreq;1;NonlinearForce;5;;;;;;;TransverseShear;;;;;;;;Damping;5", "id");
    expectInvalid("FastenerPropFreq;0;Stiffness;1", "id");
    expectInvalid("FastenerPropFreq;1;StructuralDamping;-5", "structuralDampingTableId");
    expectInvalid("FastenerPropFreq;1;Stiffness;1;;;;;;StructuralDamping;5;StructuralDamping;6", "id");
    expectInvalid("FastenerPropFreq;1;NonlinearForce;5;;;;;;;TransverseShear;3", "fuseMode");
    expectInvalid("FastenerPropFreq;1;NonlinearForce;5;;;;;;;;;;;;;0.", "fuseStiffnessRetention");
}

TEST_CASE("PFASTT has a single damping table and the same fuse lines as PBUSHT", "[model][cards]")
{
    Model model = readBulk(
        "FastenerPropFreq;1;Stiffness;51;61;10237\n"
        "FastenerPropFreq;2;Stiffness;51;;;;;;StructuralDamping;154;NonlinearForce;61;62\n"
        "FastenerPropFreq;3;NonlinearForce;5;;;;;;;TransverseShear;2;123;ActualUltimateLoad;1.;2.;2.E-5;1");
    REQUIRE(cardAt<FastenerPropFreqTable>(model, 0).stiffnessTableIds ==
            std::array<base::EntityId, 6>{51, 61, 10237, 0, 0, 0});
    const FastenerPropFreqCard groups = cardAt<FastenerPropFreqTable>(model, 1);
    REQUIRE(groups.structuralDampingTableId == 154);
    REQUIRE(groups.nonlinearForceTableIds[1] == 62);
    const FastenerPropFreqCard fuse = cardAt<FastenerPropFreqTable>(model, 2);
    REQUIRE(fuse.forceDeflectionRule == ForceDeflectionRule::Ts);
    REQUIRE(fuse.fuseMode == 2);
    REQUIRE(fuse.direction == 123);
    REQUIRE(fuse.fuseOption == FuseOption::ActualUltimateLoad);
    REQUIRE(fuse.fuseStiffnessRetention == 2.0e-5);
    REQUIRE(fuse.largeRotationEndA == 1);
    requireRoundTrip(model);
}

TEST_CASE("PCOMPG has a global ID per ply, a damping flag on the first ply and the Hashin failure criteria",
          "[model][cards]")
{
    Model model = readBulk(
        "CompPropGlobal;1;-0.186;6.35;9.E+3;Hoffman;1.;0.01;MembraneOnly;1101;163;0.048;0.;Yes;-1;101;163;0.06;45.;Yes;"
        ";2102;;0."
        "048;-45.;No;;102;164;;90.\n"
        "CompPropGlobal;2;;;;HashinCriterion;;;;1;1;0.1;0.;Yes;;2;;;30.\n"
        "CompProp;3;;;;HashinTape;;;;1;0.1");
    const CompPropGlobalCard card = cardAt<CompPropGlobalTable>(model, 0);
    REQUIRE(card.bottomOffset == -0.186);
    REQUIRE(card.nonStructMass == 6.35);
    REQUIRE(card.bondShearAllowable == 9000.0);
    REQUIRE(card.failureTheory == FailureTheory::Hoffman);
    REQUIRE(card.option == LaminateOption::Mem);
    REQUIRE(card.structuralDampingFlag == -1);
    REQUIRE(card.plies.size() == 4);
    REQUIRE(card.plies[0].globalId == 1101);
    REQUIRE(card.plies[2].matId == 163);       // a blank MID is the one of the ply before
    REQUIRE(card.plies[3].thickness == 0.048); // a blank T too
    REQUIRE(card.plies[3].matId == 164);
    REQUIRE_FALSE(card.plies[2].stressOutput);
    // A blank Z0 is minus half of the thickness of the laminate, stored resolved.
    const CompPropGlobalCard hashin = cardAt<CompPropGlobalTable>(model, 1);
    REQUIRE(hashin.failureTheory == FailureTheory::HashinFailure);
    REQUIRE(hashin.plies.size() == 2);
    REQUIRE(hashin.plies[1].matId == 1);
    REQUIRE(hashin.plies[1].thickness == 0.1);
    REQUIRE(hashin.bottomOffset == -0.1);
    // PCOMP takes the Hashin criteria too.
    REQUIRE(cardAt<CompPropTable>(model).failureTheory == FailureTheory::HashinTape);
    RecordingVisitor visitor;
    model.table<CompPropGlobalTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"plyMatId", "plyMatId", "plyMatId", "plyMatId"});
    requireRoundTrip(model);
    expectInvalid("CompPropGlobal;1;;;;;;;;0;1;0.1", "globalPlyId1");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1;0.1;;;;1;1;0.1", "globalPlyId1");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;;0.1", "plyMatId1");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1", "plyMatId1");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1;-0.1", "plyThickness1");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1;0.1;0.;MAYBE", "plyStressOutput");
    expectInvalid("CompPropGlobal;1;;;;;;;Symmetry;1;1;0.1", "laminateOption");
    expectInvalid("CompPropGlobal;1;;;;SOMETHING;;;;1;1;0.1", "failureTheory");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1;0.1;0.;Yes;-3", "structuralDampingFlag");
    expectInvalid("CompPropGlobal;1;;;;;;;;1;1;0.1;0.;Yes;-1;2;1;0.1;0.;Yes;-1", "structuralDampingFlag");
    expectInvalid("CompPropGlobal;1", "globalPlyId1");
}

TEST_CASE("PBEAM3 has fixed positions, resolves the values of B and C from end A, and keeps whether warping is given",
          "[model][cards]")
{
    Model model = readBulk(
        "BeamProp3;1;2;3.4;2.6;4.81;;;1.;0.3;2.5;-1.4;2.2;1.8;0.6;;;Yes;1.;19.4;28.3;;;;1.1;3.2;;;;;;;YesSameAsEndA;3."
        "2;2.1;3.2;"
        ";;1.;0.8;;0.5;;;;;;0.9;1.;;1.5;;;;;;1.\n"
        "BeamProp3;2;2;0.1;0.01;0.01;0.;0.02;;0.5;0.;0.;0.5;-0.5;0.;0.;-0.5");
    const BeamProp3Card card = cardAt<BeamProp3Table>(model, 0);
    const BeamProp3Station& endA = card.stations[0];
    const BeamProp3Station& endB = card.stations[1];
    const BeamProp3Station& nodeC = card.stations[2];
    REQUIRE(endA.area == 3.4);
    REQUIRE(endA.productInertiaYZ == 0.0);       // blank: 0.0
    REQUIRE(endA.torsionConstant == 2.6 + 4.81); // blank: IZ + IY
    REQUIRE(endA.nonStructMass == 1.0);
    REQUIRE(endA.stressPoints == std::array<double, 8>{0.3, 2.5, -1.4, 2.2, 1.8, 0.6, 0.0, 0.0});
    REQUIRE(endB.stressOutput);
    REQUIRE(endB.area == 1.0);
    REQUIRE(endB.bendingInertiaZ == 19.4);
    REQUIRE(endB.bendingInertiaY == 28.3);
    REQUIRE(endB.torsionConstant == endA.torsionConstant); // blank: the value of end A
    REQUIRE(endB.nonStructMass == 1.0);
    REQUIRE(endB.stressPoints[0] == 1.1);
    REQUIRE(endB.stressPoints[2] == 0.0); // the stress points are 0.0, not those of end A
    REQUIRE_FALSE(nodeC.stressOutput);
    REQUIRE(nodeC.area == 3.2);
    REQUIRE(nodeC.nonStructMass == 1.0);
    REQUIRE(card.shearFactorY == 0.9);
    REQUIRE(card.shearFactorZ == 1.0);
    REQUIRE(endA.neutralAxis == std::array<double, 2>{0.0, 1.5});
    REQUIRE(endB.neutralAxis == endA.neutralAxis);
    REQUIRE(nodeC.neutralAxis == endA.neutralAxis);
    REQUIRE(endA.nsmCenter == std::array<double, 2>{0.0, 1.0});
    REQUIRE_FALSE(card.warpingGiven);
    REQUIRE_FALSE(card.gaussStress);
    // Written explicitly: editing end A later does not move B and C.
    const Model again = readModelText(writeModelText(model), true);
    REQUIRE(cardAt<BeamProp3Table>(again).stations[2].area == 3.2);
    requireRoundTrip(model);

    Model warped = readBulk(
        "BeamProp3;1;2;0.1;0.01;0.01;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;GaussPoints;1.;"
        "2.;3.;"
        ";;;0.5;0.6");
    const BeamProp3Card gauss = cardAt<BeamProp3Table>(warped);
    REQUIRE(gauss.gaussStress);
    REQUIRE(gauss.warpingGiven);
    REQUIRE(gauss.stations[0].warping[0] == 1.0);
    REQUIRE(gauss.stations[0].warping[7] == 0.6);
    REQUIRE(gauss.stations[2].warping[7] == 0.6); // B and C: the values of end A
    requireRoundTrip(warped);
    // A zero given is a warping field given.
    Model zero =
        readBulk("BeamProp3;1;2;0.1;0.01;0.01;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;0.");
    REQUIRE(cardAt<BeamProp3Table>(zero).warpingGiven);
    requireRoundTrip(zero);

    expectInvalid("BeamProp3;1;2;0.1;0.01", "bendingInertiaY");
    expectInvalid("BeamProp3;1;2;-0.1;0.01;0.01", "area");
    expectInvalid("BeamProp3;1;2;0.1;0.01;0.01;1.", "productInertiaYZ");
    expectInvalid("BeamProp3;1;2;0.1;0.01;0.01;;;;;;;;;;;;MAYBE", "stressOutput");
    expectInvalid("BeamProp3;1;2;0.1;0.01;0.01;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;0.", "shearFactorY");
    expectInvalid(
        "BeamProp3;1;2;0.1;0.01;0.01;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;SOMETIMES",
        "stressLocation");
}

TEST_CASE("PWELD: positions of the fields, the flags and the defaults of the length ratios", "[model][cards]")
{
    Model model = readBulk("WeldProp;1;3;1.\nWeldProp;2;3;1.;On;SpotWeld;0.3;6.");
    const WeldPropCard plain = cardAt<WeldPropTable>(model, 0);
    REQUIRE(plain.diameter == 1.0);
    REQUIRE_FALSE(plain.eliminateConstraints);
    REQUIRE_FALSE(plain.spot);
    REQUIRE(plain.minLengthRatio == 0.2);
    REQUIRE(plain.maxLengthRatio == 5.0);
    const WeldPropCard full = cardAt<WeldPropTable>(model, 1);
    REQUIRE(full.eliminateConstraints);
    REQUIRE(full.spot);
    REQUIRE(full.minLengthRatio == 0.3);
    REQUIRE(full.maxLengthRatio == 6.0);
    RecordingVisitor visitor;
    model.table<WeldPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});
    requireRoundTrip(model);
    expectInvalid("WeldProp;1;3", "diameter");
    expectInvalid("WeldProp;1;3;0.", "diameter");
    expectInvalid("WeldProp;1;0;1.", "matId");
    expectInvalid("WeldProp;1;3;1.;MAYBE", "eliminateConstraints");
    expectInvalid("WeldProp;1;3;1.;;BUTT", "kind");
    expectInvalid("WeldProp;1;3;1.;;;-1.", "minLengthRatio");
}
