// Checks of the monitor and output cards. MONCARL, MONGRP, MONSUM, MONSUM1 and
// MONSUMT (the Monte-Carlo parameters and the monitor groups and sums), STOCHAS, ITER, NHRMPRM, CYSYM, ECHOON, ECHOOFF
// and METADATA (the solution control entries) and ABSNMVB, ABSNMVI, ABSNMVR and ABSNMVS (the name and value pairs of a
// meta data group, which the ABSTRCT cards define). Per card: the fields in their format reference positions, the
// defaults, the references the card lists and what each one points to, the renumbering of the spaces it owns or names,
// the refused values and the round trip through the three formats. The generic tests of CardRoundTripTest.cpp read,
// write and copy every sample; this file covers what each card means.

#include "oor/model/cards/loads/metadataBooleanPairs/MetadataBooleanPairsTable.hpp"
#include "oor/model/cards/loads/metadataGroup/MetadataGroupTable.hpp"
#include "oor/model/cards/loads/metadataIntegerPairs/MetadataIntegerPairsTable.hpp"
#include "oor/model/cards/loads/metadataRealPairs/MetadataRealPairsTable.hpp"
#include "oor/model/cards/loads/metadataTextPairs/MetadataTextPairsTable.hpp"
#include "oor/model/cards/matrices/monitorGroup/MonitorGroupTable.hpp"
#include "oor/model/cards/matrices/monitorSum/MonitorSumTable.hpp"
#include "oor/model/cards/matrices/monitorSumLocated/MonitorSumLocatedTable.hpp"
#include "oor/model/cards/matrices/monitorSumTotal/MonitorSumTotalTable.hpp"
#include "oor/model/cards/matrices/monteCarloParams/MonteCarloParamsTable.hpp"
#include "oor/model/cards/solution_control/cyclicSymmetry/CyclicSymmetryTable.hpp"
#include "oor/model/cards/solution_control/echoOffMarker/EchoOffMarkerTable.hpp"
#include "oor/model/cards/solution_control/echoOnMarker/EchoOnMarkerTable.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsCatalogue.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsTable.hpp"
#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsCatalogue.hpp"
#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsTable.hpp"
#include "oor/model/cards/solution_control/resultMetadata/ResultMetadataTable.hpp"
#include "oor/model/cards/solution_control/stochasticParams/StochasticParamsTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

// A term of a MONSUM or a MONSUM1.
MonitorSumTerm term(std::int32_t newAxes, MonitorResultType type, const char* name, std::int32_t axes,
                    double coefficient = 1.0)
{
    MonitorSumTerm result;
    result.newAxes = newAxes;
    result.type = type;
    result.name = name;
    result.axes = axes;
    result.coefficient = coefficient;
    return result;
}

IterativeSolverOptionsOption iterOption(const char* name, IterativeSolverOptionsValue value)
{
    return IterativeSolverOptionsOption{name, std::move(value)};
}

NonlinearHarmonicSolverParamsParameter harmonicSolverParameter(const char* name,
                                                               NonlinearHarmonicSolverParamsValue value)
{
    return NonlinearHarmonicSolverParamsParameter{name, std::move(value)};
}

// The references a card lists: the field names and what each points to.
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

const std::string FOOT_LABEL = "SPAR ROOT LOADS FROM THE BODY AND THE AERO MODEL";

constexpr MonitorResultType AERO_POINT = MonitorResultType::AeroPoint;
constexpr MonitorResultType AERO_DISPLACEMENT = MonitorResultType::AeroDisplacement;
constexpr MonitorResultType STRUCTURAL_POINT = MonitorResultType::StructuralPoint;
constexpr MonitorResultType STRUCTURAL_DISPLACEMENT = MonitorResultType::StructuralDisplacement;
constexpr MonitorResultType LOAD_POINT = MonitorResultType::LoadPoint;

} // namespace

// --------------------------------------------------------------------------------------------------------------------
// MONCARL
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MONCARL has a type and a structural and a fluid line of three variabilities",
          "[model][cards][monitoroutput]")
{
    REQUIRE((MonteCarloParamsTable::INFO.category == CardCategory::MonteCarloSet &&
             MonteCarloParamsTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "MonteCarloParams;10\nMonteCarloParams;20;DeltaVariation;StructuralVariation;0.003;0.04;0.006;FluidVariation;;"
        "0.002\nMonteCarloParams;30;;"
        "FluidVariation;0.5;;;StructuralVariation");
    const MonteCarloParamsCard minimal = cardAt<MonteCarloParamsTable>(model, 0);
    REQUIRE((minimal.id == 10 && minimal.type == MonteCarloParamsVariabilityType::Alpha));
    REQUIRE_FALSE((minimal.structural.has_value() || minimal.fluid.has_value()));

    const MonteCarloParamsCard both = cardAt<MonteCarloParamsTable>(model, 1);
    REQUIRE((both.id == 20 && both.type == MonteCarloParamsVariabilityType::Delta));
    REQUIRE(both.structural.has_value());
    REQUIRE(
        (both.structural->mass == 0.003 && both.structural->stiffness == 0.04 && both.structural->damping == 0.006));
    REQUIRE(both.fluid.has_value());
    REQUIRE((both.fluid->mass == 0.0 && both.fluid->stiffness == 0.002 && both.fluid->damping == 0.0)); // blank is 0.0

    // The lines come in either order, and a keyword alone is a line of zeros.
    const MonteCarloParamsCard reversed = cardAt<MonteCarloParamsTable>(model, 2);
    REQUIRE((reversed.fluid.has_value() && reversed.fluid->mass == 0.5));
    REQUIRE(
        (reversed.structural.has_value() && reversed.structural->mass == 0.0 && reversed.structural->damping == 0.0));
    requireRoundTrip(model);

    // The structural line is written first, and ALPHAVAR is written as a blank.

    // The card names no other card.
    REQUIRE(visitedBy<MonteCarloParamsTable>(model, 1).fields.empty());

    expectInvalid("MonteCarloParams", "id");
    expectInvalid("MonteCarloParams;0", "id");
    expectInvalid("MonteCarloParams;1;BETAVAR", "variabilityType");
    expectInvalid("MonteCarloParams;1;;StructuralVariation;-0.1", "structuralMassVariability");
    expectInvalid("MonteCarloParams;1;;StructuralVariation;0.;-0.1", "structuralStiffnessVariability");
    expectInvalid("MonteCarloParams;1;;FluidVariation;0.;0.;-1.", "fluidDampingVariability");
    expectInvalid("MonteCarloParams;1;;StructuralVariation;0.1;;;StructuralVariation;0.2",
                  "structuralVariability");                                                   // a keyword once
    REQUIRE_THROWS_AS(readBulk("MonteCarloParams;10\nMonteCarloParams;10"), base::DeckError); // one set per SID
}

TEST_CASE("a MONCARL can be edited and is selected by the Case Control MONCARL, which follows its renumbering",
          "[model][cards][monitoroutput]")
{
    Model model =
        readBulk("MonteCarloParams;20;DeltaVariation;StructuralVariation;0.003;0.04;0.006;FluidVariation;;0.002");
    MonteCarloParamsCard edited = cardAt<MonteCarloParamsTable>(model);
    edited.structural->mass = 0.5;
    edited.fluid.reset();
    edited.type = MonteCarloParamsVariabilityType::Alpha;
    REQUIRE_FALSE(MonteCarloParamsFormat::check(edited).has_value());
    model.table<MonteCarloParamsTable>().store(0, edited);
    REQUIRE(cardAt<MonteCarloParamsTable>(model) == edited);
    edited.structural->damping = -1.0;
    REQUIRE(MonteCarloParamsFormat::check(edited).has_value());
    edited.structural->damping = 0.0;
    edited.id = 0;
    REQUIRE(MonteCarloParamsFormat::check(edited).has_value());

    Model selected = readModelText(
        "[case]\nMonteCarloSelection;100\n[model]\nMonteCarloParams;100\nMonteCarloParams;200;DeltaVariation;"
        "StructuralVariation;0.01\n",
        true);
    REQUIRE(unresolvedIn(selected) == 0);
    REQUIRE(globalCommand(selected, "MonteCarloSelection") == 100);
    REQUIRE(Renumbering::apply(selected, RenumberSpace::MonteCarloSet, Renumbering::offset(1000)) == 2);
    REQUIRE((cardAt<MonteCarloParamsTable>(selected, 0).id == 1100 &&
             cardAt<MonteCarloParamsTable>(selected, 1).id == 1200));
    REQUIRE(globalCommand(selected, "MonteCarloSelection") == 1100);
    REQUIRE(unresolvedIn(selected) == 0);

    Model dangling = readModelText("[case]\nMonteCarloSelection;7\n[model]\nMonteCarloParams;100\n", true);
    REQUIRE(unresolvedIn(dangling) == 1);
}

// --------------------------------------------------------------------------------------------------------------------
// MONGRP
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// MONSUM
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MONSUM has an alternate layout with the type before the new component", "[model][cards][monitoroutput]")
{
    // The alternate layout: one type on the first line, then NEWAXISA.
    Model model = readBulk(
        "MonitorSum;SPARSUM;Loads summed at the spar "
        "root;AeroMonitorIntegratedLoad;123456;SPARB;123;3.25;SPARB;456;1.75");
    const MonitorSumCard card = cardAt<MonitorSumTable>(model);
    REQUIRE(card.body.alternate);
    REQUIRE(card.body.terms == std::vector<MonitorSumTerm>{term(123456, AERO_POINT, "SPARB", 123, 3.25),
                                                           term(123456, AERO_POINT, "SPARB", 456, 1.75)});
    requireRoundTrip(model);

    // The layout is kept: the type is written once for the lines that go on with it, and a new block needs only its
    // NEWAXISj.
    const Model blocks =
        readBulk("MonitorSum;TWO;;StructMonitorIntegratedLoad;12;P1;12;1.;P2;12;2.;;;P3;12;3.;;;;;3;P1;3");
    const MonitorSumCard two = cardAt<MonitorSumTable>(blocks);
    REQUIRE(two.body.alternate);
    REQUIRE(two.body.terms ==
            std::vector<MonitorSumTerm>{term(12, STRUCTURAL_POINT, "P1", 12), term(12, STRUCTURAL_POINT, "P2", 12, 2.0),
                                        term(12, STRUCTURAL_POINT, "P3", 12, 3.0), term(3, STRUCTURAL_POINT, "P1", 3)});

    requireRoundTrip(blocks);

    // A type that changes inside a block is written where it changes.
    const Model changed =
        readBulk("MonitorSum;CHG;;AeroMonitorIntegratedLoad;12;P1;12;;;;;StructMonitorIntegratedLoad;;P2;12");
    REQUIRE(cardAt<MonitorSumTable>(changed).body.terms ==
            std::vector<MonitorSumTerm>{term(12, AERO_POINT, "P1", 12), term(12, STRUCTURAL_POINT, "P2", 12)});
    requireRoundTrip(changed);
}

TEST_CASE("a MONSUM refuses what its format forbids, and an edited MONSUM is checked", "[model][cards][monitoroutput]")
{
    expectInvalid("MonitorSum", "entryName");
    expectInvalid("MonitorSum;X", "termNames"); // at least one term
    oor::testing::requireModelFileRoundTrip(
        readModelText("MonitorSum;ABCDEFGHI;;1;AeroMonitorIntegratedLoad;A;1", true)); // long names are preserved
    expectInvalid("MonitorSum;X;;1;BMONPNT1;A;1", "termType");
    expectInvalid("MonitorSum;X;;1;;A;1", "termType"); // a block starts with its type
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;A;1;;;;;2;AeroMonitorDisplacement;B;2",
                  "termType");                                                 // forces and displacements are not added
    expectInvalid("MonitorSum;X;;7;AeroMonitorIntegratedLoad;A;1", "newAxes"); // the digits 1 to 6
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;A;7", "termAxes");
    expectInvalid("MonitorSum;X;;12;AeroMonitorIntegratedLoad;A;12;;;;;23;AeroMonitorIntegratedLoad;B;23",
                  "newAxes"); // a component is filled once
    expectInvalid("MonitorSum;X;;12;AeroMonitorIntegratedLoad;A;1",
                  "termAxes"); // the NEWAXISj is the union of the AXES
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;A;12",
                  "termAxes"); // one component out: each input is one component
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;;1", "termNames");
    expectInvalid("MonitorSum;X;;;AeroMonitorIntegratedLoad;A;1", "termNames"); // a term needs its NEWAXISj above
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;;;;;;;2;AeroMonitorIntegratedLoad;A;2",
                  "termNames"); // a NEWAXISj with no term
    expectInvalid("MonitorSum;X;;1;AeroMonitorIntegratedLoad;A;1;x", "termCoefficient");

    Model model = readBulk("MonitorSum;OK;;12;AeroMonitorIntegratedLoad;A;1;2.;B;2");
    MonitorSumCard edited = cardAt<MonitorSumTable>(model);
    REQUIRE_FALSE(MonitorSumFormat::check(edited).has_value());
    edited.body.terms.push_back(term(3, LOAD_POINT, "C", 4));
    REQUIRE_FALSE(MonitorSumFormat::check(edited).has_value()); // a block of its own that fills component 3 from 4
    edited.body.terms.back().axes = 45;
    REQUIRE(MonitorSumFormat::check(edited).has_value());
    edited.body.terms.back().axes = 4;
    edited.body.terms.back().newAxes = 2; // component 2 is filled by the first block too
    REQUIRE(MonitorSumFormat::check(edited).has_value());
    edited.body.terms.back().newAxes = 3;
    edited.body.terms.back().coefficient = std::numeric_limits<double>::infinity();
    REQUIRE(MonitorSumFormat::check(edited).has_value());
    edited.body.terms.back().coefficient = -2.5;
    edited.name = "TOOLONGNAME";
    REQUIRE_FALSE(MonitorSumFormat::check(edited).has_value());
    edited.name = "NEWNAME";
    model.table<MonitorSumTable>().store(0, edited);
    REQUIRE(cardAt<MonitorSumTable>(model) == edited);
    requireRoundTrip(model);
}

// --------------------------------------------------------------------------------------------------------------------
// MONSUM1
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MONSUM1 has a location line before the terms and names its coordinate systems",
          "[model][cards][monitoroutput]")
{
    REQUIRE((MonitorSumLocatedTable::INFO.category == CardCategory::MonitorPoint &&
             MonitorSumLocatedTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "RectFrame2;5\nRectFrame2;6\nRectFrame2;20\n"
        "MonitorSumLocated;ROOT;" +
        FOOT_LABEL +
        ";;310.;0.;27.5;20;123456;MonitorNodeForceSum3;BODY_310;123;-3.25;BODY_310;356;-1.75;;"
        "AeroMonitorIntegratedLoad;SPARB;123;3.25;SPARB2;456;"
        "1.75\n"
        "MonitorSumLocated;ALT;" +
        FOOT_LABEL +
        ";;310.;0.;27.5;20;MonitorNodeForceSum3;123456;BODY_310;123;-3.25;BODY_310;456;-1.75;AeroMonitorIntegratedLoad;"
        "123456;SPARB;123;3.25;"
        "SPARB2;456;1.75\n"
        "MonitorSumLocated;LOC;Every "
        "field;5;1.;2.;-3.;6;123;AeroMonitorIntegratedLoad;A1;123;2.;A2;12;0.5;;;A3;3;1.5;;;;456;"
        "StructMonitorIntegratedLoad;"
        "S1;456\n"
        "MonitorSumLocated;DFLT;;5;;;;;1;StructMonitorIntegratedLoad;P");
    const MonitorSumLocatedCard root = cardAt<MonitorSumLocatedTable>(model, 0);
    REQUIRE((root.name == "ROOT" && root.label == FOOT_LABEL));
    REQUIRE((root.positionFrameId == 0 && root.xCoord == 310.0 && root.yCoord == 0.0 && root.zCoord == 27.5 &&
             root.displacementFrameId == 20));
    REQUIRE_FALSE(root.body.alternate);
    REQUIRE(root.body.terms == std::vector<MonitorSumTerm>{term(123456, LOAD_POINT, "BODY_310", 123, -3.25),
                                                           term(123456, LOAD_POINT, "BODY_310", 356, -1.75),
                                                           term(123456, AERO_POINT, "SPARB", 123, 3.25),
                                                           term(123456, AERO_POINT, "SPARB2", 456, 1.75)});
    // The alternate layout with the two types on two lines that both start with NEWAXISA: one block, two sections.
    const MonitorSumLocatedCard alternate = cardAt<MonitorSumLocatedTable>(model, 1);
    REQUIRE(alternate.body.alternate);
    REQUIRE(alternate.body.terms == std::vector<MonitorSumTerm>{term(123456, LOAD_POINT, "BODY_310", 123, -3.25),
                                                                term(123456, LOAD_POINT, "BODY_310", 456, -1.75),
                                                                term(123456, AERO_POINT, "SPARB", 123, 3.25),
                                                                term(123456, AERO_POINT, "SPARB2", 456, 1.75)});
    const MonitorSumLocatedCard every = cardAt<MonitorSumLocatedTable>(model, 2);
    REQUIRE((every.positionFrameId == 5 && every.xCoord == 1.0 && every.yCoord == 2.0 && every.zCoord == -3.0 &&
             every.displacementFrameId == 6));
    REQUIRE(every.body.terms ==
            std::vector<MonitorSumTerm>{term(123, AERO_POINT, "A1", 123, 2.0), term(123, AERO_POINT, "A2", 12, 0.5),
                                        term(123, AERO_POINT, "A3", 3, 1.5), term(456, STRUCTURAL_POINT, "S1", 456)});
    // A blank CD is the system of CP, kept resolved.
    REQUIRE((cardAt<MonitorSumLocatedTable>(model, 3).positionFrameId == 5 &&
             cardAt<MonitorSumLocatedTable>(model, 3).displacementFrameId == 5));
    requireRoundTrip(model);

    // CP and CD are coordinate systems; the basic system is not visited. The names of the terms are not IDs.
    const Visited rootVisited = visitedBy<MonitorSumLocatedTable>(model, 0);
    REQUIRE(rootVisited.fields == std::vector<std::string>{"displacementFrameId"});
    REQUIRE(rootVisited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    const Visited everyVisited = visitedBy<MonitorSumLocatedTable>(model, 2);
    REQUIRE(everyVisited.fields == std::vector<std::string>{"positionFrameId", "displacementFrameId"});
    REQUIRE(everyVisited.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::Coordinate));
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE((cardAt<MonitorSumLocatedTable>(model, 0).displacementFrameId == 120 &&
             cardAt<MonitorSumLocatedTable>(model, 2).positionFrameId == 105));
    REQUIRE((cardAt<MonitorSumLocatedTable>(model, 2).displacementFrameId == 106 &&
             cardAt<MonitorSumLocatedTable>(model, 3).displacementFrameId == 105));
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readBulk("MonitorSumLocated;DFLT;;9;;;;;1;StructMonitorIntegratedLoad;P");
    REQUIRE(unresolvedIn(dangling) == 2); // the CP and the CD that is the CP

    const Model minimal = readBulk("MonitorSumLocated;MIN;;;;;;;1;StructMonitorIntegratedLoad;P");
    REQUIRE((cardAt<MonitorSumLocatedTable>(minimal).positionFrameId == 0 &&
             cardAt<MonitorSumLocatedTable>(minimal).displacementFrameId == 0));

    expectInvalid("MonitorSumLocated;X;;-1;;;;;1;AeroMonitorIntegratedLoad;A;1", "positionFrameId");
    expectInvalid("MonitorSumLocated;X;;;;;;-2;1;AeroMonitorIntegratedLoad;A;1", "displacementFrameId");
    expectInvalid("MonitorSumLocated;X;;;x;;;;1;AeroMonitorIntegratedLoad;A;1", "xCoord");
    expectInvalid("MonitorSumLocated;X;;;;;;;1;AeroMonitorDisplacement;A;1", "termType"); // only forces and moments
    expectInvalid("MonitorSumLocated;X;;;;;;;1;StructMonitorDisplacement;A;1", "termType");
    expectInvalid("MonitorSumLocated;X;;;;;;;12;AeroMonitorIntegratedLoad;A;1", "termAxes");
    expectInvalid("MonitorSumLocated;X\n", "termNames");
    // A sixth number on the location line has no field to hold it.
}

// --------------------------------------------------------------------------------------------------------------------
// MONSUMT
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// STOCHAS
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a STOCHAS has a group of statistics for each of its flags, in any order", "[model][cards][monitoroutput]")
{
    REQUIRE((StochasticParamsTable::INFO.category == CardCategory::Other &&
             StochasticParamsTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "StochasticParams;100;PropertyEntries;;;;MaterialEntries\n"
        "StochasticParams;200;LoadEntries;Gauss;0.3;;MaterialEntries;Gauss;0.1\n"
        "StochasticParams;300;EnforcedMotionConstr;;0.2;2.5\n"
        "StochasticParams;400;ConnectivityEntries;;;;EnforcedMotionConstr;;;;PropertyEntries");
    const StochasticParamsCard defaults = cardAt<StochasticParamsTable>(model, 0);
    REQUIRE(defaults.id == 100);
    REQUIRE(defaults.groups.size() == 2);
    REQUIRE((defaults.groups[0].flag == StochasticParamsFlag::Property && defaults.groups[0].distribution.empty()));
    REQUIRE((defaults.groups[0].variationCoefficient == STOCHASTIC_PARAMS_DEFAULT_VARIATION_COEFFICIENT &&
             defaults.groups[0].standardDeviations == STOCHASTIC_PARAMS_DEFAULT_STANDARD_DEVIATIONS));
    REQUIRE(defaults.groups[1].flag == StochasticParamsFlag::Material);

    const StochasticParamsCard gauss = cardAt<StochasticParamsTable>(model, 1);
    REQUIRE(gauss.groups.size() == 2);
    REQUIRE(
        (gauss.groups[0].flag == StochasticParamsFlag::Loads && gauss.groups[0].distribution == "Gauss")); // upper case
    REQUIRE((gauss.groups[0].variationCoefficient == 0.3 && gauss.groups[0].standardDeviations == 3.0));
    REQUIRE((gauss.groups[1].flag == StochasticParamsFlag::Material && gauss.groups[1].variationCoefficient == 0.1));

    const StochasticParamsCard single = cardAt<StochasticParamsTable>(model, 2);
    REQUIRE((single.groups.size() == 1 && single.groups[0].flag == StochasticParamsFlag::Spcd));
    REQUIRE((single.groups[0].variationCoefficient == 0.2 && single.groups[0].standardDeviations == 2.5));

    // The order of the flags is the order of the card.
    const StochasticParamsCard order = cardAt<StochasticParamsTable>(model, 3);
    REQUIRE((order.groups.size() == 3 && order.groups[0].flag == StochasticParamsFlag::Connectivity));
    REQUIRE(
        (order.groups[1].flag == StochasticParamsFlag::Spcd && order.groups[2].flag == StochasticParamsFlag::Property));
    requireRoundTrip(model);

    // The defaults of CoV and m are static: they are written as blanks.

    REQUIRE(visitedBy<StochasticParamsTable>(model, 0).fields.empty());

    expectInvalid("StochasticParams", "id");
    expectInvalid("StochasticParams;0;PropertyEntries", "id");
    expectInvalid("StochasticParams;1", "flag"); // at least one flag
    expectInvalid("StochasticParams;1;BOGUS", "flag");
    expectInvalid("StochasticParams;1;PropertyEntries;;;;PropertyEntries", "flag"); // a flag once
    expectInvalid("StochasticParams;1;;;0.1", "flag");                              // values need their flag
    expectInvalid("StochasticParams;1;PropertyEntries;;0.", "variationCoefficient");
    expectInvalid("StochasticParams;1;PropertyEntries;;-0.1", "variationCoefficient");
    expectInvalid("StochasticParams;1;PropertyEntries;;;-1.", "standardDeviations");
    expectInvalid("StochasticParams;1;PropertyEntries;;0.5;3.",
                  "standardDeviations"); // m times CoV must be less than 1.0
    oor::testing::requireModelFileRoundTrip(
        readModelText("StochasticParams;1;PropertyEntries;TOOLONGDISTRIBUTION", true));
    REQUIRE_THROWS_AS(readBulk("StochasticParams;100;PropertyEntries\nStochasticParams;100;MaterialEntries"),
                      base::DeckError);

    StochasticParamsCard edited = defaults;
    edited.groups[0].variationCoefficient = 0.33;
    edited.groups[0].standardDeviations = 3.1;
    REQUIRE(StochasticParamsFormat::check(edited).has_value()); // m times CoV is 1.023: more than 1 is refused
    edited.groups[0].standardDeviations = 3.0;
    REQUIRE_FALSE(StochasticParamsFormat::check(edited).has_value()); // 0.99
}

// --------------------------------------------------------------------------------------------------------------------
// ITER
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ITER lists OPTION=VALUE items that the table of its format types", "[model][cards][monitoroutput]")
{
    REQUIRE((IterativeSolverOptionsTable::INFO.category == CardCategory::IterativeSolver &&
             IterativeSolverOptionsTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "IterativeSolverOptions;10\n"
        "IterativeSolverOptions;20;IterationTolerance=3.5E-6;IterationMessages=No;PreconditionerKind="
        "BlockIncompleteCholesky;"
        "PreconditionerPadding=2\n"
        "IterativeSolverOptions;30;PreconditionerKind=BlockIncompleteCholeskyComplex;ConvergenceCriteria="
        "EnergyNormCriterionExtended;MaxIterationSteps=500;"
        "PreconditionerExtractionLevel=3;EstimateResourcesOnly=-1;FUTURE=7;TXT=abc;SCALE=2.5\n"
        "IterativeSolverOptions;40;IterationTolerance= 1;PreconditionerKind= JacobiScaled");
    REQUIRE(cardAt<IterativeSolverOptionsTable>(model, 0).options.empty());
    REQUIRE(cardAt<IterativeSolverOptionsTable>(model, 1).options ==
            std::vector<IterativeSolverOptionsOption>{
                iterOption("IterationTolerance", IterativeSolverOptionsValue(3.5e-6)),
                iterOption("IterationMessages", IterativeSolverOptionsValue(std::string("No"))),
                iterOption("PreconditionerKind", IterativeSolverOptionsValue(std::string("BlockIncompleteCholesky"))),
                iterOption("PreconditionerPadding", IterativeSolverOptionsValue(std::int32_t{2}))});
    // Words are the neutral names, the names the format does not list with the value as written.
    REQUIRE(
        cardAt<IterativeSolverOptionsTable>(model, 2).options ==
        std::vector<IterativeSolverOptionsOption>{
            iterOption("PreconditionerKind",
                       IterativeSolverOptionsValue(std::string("BlockIncompleteCholeskyComplex"))),
            iterOption("ConvergenceCriteria", IterativeSolverOptionsValue(std::string("EnergyNormCriterionExtended"))),
            iterOption("MaxIterationSteps", IterativeSolverOptionsValue(std::int32_t{500})),
            iterOption("PreconditionerExtractionLevel", IterativeSolverOptionsValue(std::int32_t{3})),
            iterOption("EstimateResourcesOnly", IterativeSolverOptionsValue(std::int32_t{-1})),
            iterOption("FUTURE", IterativeSolverOptionsValue(std::int32_t{7})),
            iterOption("TXT", IterativeSolverOptionsValue(std::string("abc"))),
            iterOption("SCALE", IterativeSolverOptionsValue(2.5))});
    // A real option written as an integer holds the real that is whole; blanks around the equals sign are allowed.
    REQUIRE(cardAt<IterativeSolverOptionsTable>(model, 3).options ==
            std::vector<IterativeSolverOptionsOption>{
                iterOption("IterationTolerance", IterativeSolverOptionsValue(1.0)),
                iterOption("PreconditionerKind", IterativeSolverOptionsValue(std::string("JacobiScaled")))});
    requireRoundTrip(model);

    REQUIRE(visitedBy<IterativeSolverOptionsTable>(model, 1).fields.empty());

    // A real with two exponents is no number, and a word the table does not list is no preconditioner.
    expectInvalid(
        "IterativeSolverOptions;100;IterationTolerance=2.5E0-6;IterationMessages=No;PreconditionerKind=BICFAST;"
        "PreconditionerPadding=4",
        "optionValues");
    expectInvalid("IterativeSolverOptions;100;PreconditionerKind=BICFAST", "optionValues");
    expectInvalid("IterativeSolverOptions;100;ConvergenceCriteria=XX", "optionValues");
    expectInvalid("IterativeSolverOptions;100;IterationMessages=MAYBE", "optionValues");
    expectInvalid("IterativeSolverOptions;100;IterationTolerance=0.", "optionValues");
    expectInvalid("IterativeSolverOptions;100;IterationTolerance=-1.E-6", "optionValues");
    expectInvalid("IterativeSolverOptions;100;MaxIterationSteps=0", "optionValues");
    expectInvalid("IterativeSolverOptions;100;MaxIterationSteps=2.5", "optionValues");
    expectInvalid("IterativeSolverOptions;100;PreconditionerPadding=-1", "optionValues");
    expectInvalid("IterativeSolverOptions;100;PreconditionerExtractionLevel=8", "optionValues");
    expectInvalid("IterativeSolverOptions;100;EstimateResourcesOnly=1", "optionValues");
    expectInvalid("IterativeSolverOptions;100;PRECOND", "options"); // an item is OPTION=VALUE
    expectInvalid("IterativeSolverOptions;100;=3", "options");
    expectInvalid("IterativeSolverOptions;100;PreconditionerPadding=", "options");
    expectInvalid("IterativeSolverOptions;100;PreconditionerPadding=1;PreconditionerPadding=2",
                  "options"); // an option once
    expectInvalid("IterativeSolverOptions", "id");
    expectInvalid("IterativeSolverOptions;0", "id");
    REQUIRE_THROWS_AS(readBulk("IterativeSolverOptions;10\nIterativeSolverOptions;10"), base::DeckError);

    // The catalogue: eight options, each with its kind and what the value may be.
    REQUIRE(ITERATIVE_SOLVER_OPTIONS_CATALOGUE.size() == 8);
    REQUIRE(findIterativeSolverOption("PreconditionerKind") != nullptr);
    REQUIRE(findIterativeSolverOption("FUTURE") == nullptr);
    REQUIRE(iterativeSolverOptionsRuleText(*findIterativeSolverOption("PreconditionerExtractionLevel")) ==
            "an integer from 0 to 7");
    REQUIRE(iterativeSolverOptionsRuleText(*findIterativeSolverOption("IterationTolerance")) ==
            "a real greater than 0");
    REQUIRE(iterativeSolverOptionsRuleText(*findIterativeSolverOption("PreconditionerKind")) ==
            "one of Jacobi, JacobiScaled, IncompleteCholesky, IncompleteCholeskyScaled, RowIncompleteCholesky, "
            "RowIncompleteCholeskySymmetric, BlockIncompleteCholesky, BlockIncompleteCholeskyComplex, "
            "CacheAwareIncomplete or User");
    REQUIRE(iterativeSolverOptionsKindName(findIterativeSolverOption("IterationMessages")->kind) == "word");
}

TEST_CASE("an ITER is selected by the Case Control SMETHOD, which follows its renumbering",
          "[model][cards][monitoroutput]")
{
    Model model = readModelText(
        "[case]\nIterativeSolverSelection;100\n[model]\nIterativeSolverOptions;100;PreconditionerPadding="
        "3\nIterativeSolverOptions;200\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(globalCommand(model, "IterativeSolverSelection") == 100);
    REQUIRE(Renumbering::apply(model, RenumberSpace::IterativeSolver, Renumbering::offset(1000)) == 2);
    REQUIRE((cardAt<IterativeSolverOptionsTable>(model, 0).id == 1100 &&
             cardAt<IterativeSolverOptionsTable>(model, 1).id == 1200));
    REQUIRE(globalCommand(model, "IterativeSolverSelection") == 1100);
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readModelText("[case]\nIterativeSolverSelection;7\n[model]\nIterativeSolverOptions;100\n", true);
    REQUIRE(unresolvedIn(dangling) == 1);

    IterativeSolverOptionsCard edited = cardAt<IterativeSolverOptionsTable>(model, 0);
    edited.options.push_back(iterOption("PreconditionerExtractionLevel", IterativeSolverOptionsValue(std::int32_t{9})));
    REQUIRE(IterativeSolverOptionsFormat::check(edited).has_value());
    // emplace, not the assignment of a temporary variant: with -fsanitize=address GCC 12 takes the moved string of that
    // temporary for uninitialised (-Wmaybe-uninitialized)
    edited.options.back().value.emplace<std::int32_t>(7);
    REQUIRE_FALSE(IterativeSolverOptionsFormat::check(edited).has_value());
    edited.options.push_back(
        iterOption("IterationTolerance", IterativeSolverOptionsValue(std::int32_t{1}))); // a real option holds a real
    REQUIRE(IterativeSolverOptionsFormat::check(edited).has_value());
}

// --------------------------------------------------------------------------------------------------------------------
// NHRMPRM
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("an NHRMPRM lists the parameters of its table as pairs of name and value", "[model][cards][monitoroutput]")
{
    REQUIRE((NonlinearHarmonicSolverParamsTable::INFO.category == CardCategory::Other &&
             NonlinearHarmonicSolverParamsTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "NonlinearHarmonicSolverParams;MaxNonConvergedFrequencies;3;NonlinearHarmonicDiagnostic;1\n"
        "NonlinearHarmonicSolverParams;NonlinearHarmonicTolerance;1.E-5;MaxFrequencyInterval;10;DecompositionMethod;"
        "QrDecomposition;"
        "ContinuationMethod;2;MYPARAM;2.5;OTHER;Yes;StepHalvingIterations;8");
    REQUIRE(
        cardAt<NonlinearHarmonicSolverParamsTable>(model, 0).parameters ==
        std::vector<NonlinearHarmonicSolverParamsParameter>{
            harmonicSolverParameter("MaxNonConvergedFrequencies", NonlinearHarmonicSolverParamsValue(std::int32_t{3})),
            harmonicSolverParameter("NonlinearHarmonicDiagnostic",
                                    NonlinearHarmonicSolverParamsValue(std::int32_t{1}))});
    // A real parameter written as an integer holds the real that is whole; a word is in upper case; a name the table
    // does not list is kept with its value as written.
    REQUIRE(cardAt<NonlinearHarmonicSolverParamsTable>(model, 1).parameters ==
            std::vector<NonlinearHarmonicSolverParamsParameter>{
                harmonicSolverParameter("NonlinearHarmonicTolerance", NonlinearHarmonicSolverParamsValue(1.0e-5)),
                harmonicSolverParameter("MaxFrequencyInterval", NonlinearHarmonicSolverParamsValue(10.0)),
                harmonicSolverParameter("DecompositionMethod",
                                        NonlinearHarmonicSolverParamsValue(std::string("QrDecomposition"))),
                harmonicSolverParameter("ContinuationMethod", NonlinearHarmonicSolverParamsValue(std::int32_t{2})),
                harmonicSolverParameter("MYPARAM", NonlinearHarmonicSolverParamsValue(2.5)),
                harmonicSolverParameter("OTHER", NonlinearHarmonicSolverParamsValue(std::string("Yes"))),
                harmonicSolverParameter("StepHalvingIterations", NonlinearHarmonicSolverParamsValue(std::int32_t{8}))});
    requireRoundTrip(model);

    REQUIRE(visitedBy<NonlinearHarmonicSolverParamsTable>(model, 0).fields.empty());

    expectInvalid("NonlinearHarmonicSolverParams;ContinuationMethod;3", "value");                 // 0, 1 or 2
    expectInvalid("NonlinearHarmonicSolverParams;ContinuationMethod;1.", "value");                // an integer
    expectInvalid("NonlinearHarmonicSolverParams;DecompositionMethod;LdlDecomposition", "value"); // LU or QR
    expectInvalid("NonlinearHarmonicSolverParams;ExtraSamplingPoints;2.5", "value");
    expectInvalid("NonlinearHarmonicSolverParams;NonlinearHarmonicTolerance;abc", "value");
    expectInvalid("NonlinearHarmonicSolverParams;ExtraSamplingPoints",
                  "value");                                              // a name needs its value in the next field
    expectInvalid("NonlinearHarmonicSolverParams;;5", "parameterName");  // a value needs its name
    expectInvalid("NonlinearHarmonicSolverParams;1;5", "parameterName"); // a name starts with a letter
    oor::testing::requireModelFileRoundTrip(readModelText("NonlinearHarmonicSolverParams;ABCDEFGHI;5", true));

    // The catalogue: the twelve parameters of the table.
    REQUIRE(NONLINEAR_HARMONIC_SOLVER_PARAMS_CATALOGUE.size() == 12);
    REQUIRE(findNonlinearHarmonicSolverParam("ContinuationMethod") != nullptr);
    REQUIRE(findNonlinearHarmonicSolverParam("MYPARAM") == nullptr);
    REQUIRE(nonlinearHarmonicSolverParamsValuesText(*findNonlinearHarmonicSolverParam("ContinuationMethod")) ==
            "0, 1 or 2");
    REQUIRE(nonlinearHarmonicSolverParamsValuesText(*findNonlinearHarmonicSolverParam("DecompositionMethod")) ==
            "LowerUpperDecomposition or QrDecomposition");
    REQUIRE(nonlinearHarmonicSolverParamsValuesText(*findNonlinearHarmonicSolverParam("NonlinearHarmonicTolerance")) ==
            "a real");
    REQUIRE(nonlinearHarmonicSolverParamsValuesText(*findNonlinearHarmonicSolverParam("ExtraSamplingPoints")) ==
            "an integer");
    REQUIRE(nonlinearHarmonicSolverParamsKindName(findNonlinearHarmonicSolverParam("DecompositionMethod")->kind) ==
            "word");

    NonlinearHarmonicSolverParamsCard edited = cardAt<NonlinearHarmonicSolverParamsTable>(model, 0);
    edited.parameters.push_back(
        harmonicSolverParameter("ContinuationMethod", NonlinearHarmonicSolverParamsValue(std::int32_t{2})));
    REQUIRE_FALSE(NonlinearHarmonicSolverParamsFormat::check(edited).has_value());
    edited.parameters.push_back(
        harmonicSolverParameter("NonlinearHarmonicTolerance",
                                NonlinearHarmonicSolverParamsValue(std::int32_t{1}))); // a real parameter holds a real
    REQUIRE(NonlinearHarmonicSolverParamsFormat::check(edited).has_value());
}

// --------------------------------------------------------------------------------------------------------------------
// CYSYM
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CYSYM has a number of segments and a symmetry type", "[model][cards][monitoroutput]")
{
    REQUIRE((CyclicSymmetryTable::INFO.category == CardCategory::Other &&
             CyclicSymmetryTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk("CyclicSymmetry;5;Rotational\nCyclicSymmetry;4;Dihedral\nCyclicSymmetry;12;Axisymmetric");
    REQUIRE((cardAt<CyclicSymmetryTable>(model, 0).segments == 5 &&
             cardAt<CyclicSymmetryTable>(model, 0).type == CyclicSymmetryType::Rotational));
    REQUIRE((cardAt<CyclicSymmetryTable>(model, 1).segments == 4 &&
             cardAt<CyclicSymmetryTable>(model, 1).type == CyclicSymmetryType::Dihedral));
    REQUIRE(cardAt<CyclicSymmetryTable>(model, 2).type == CyclicSymmetryType::Axisymmetric);
    requireRoundTrip(model);

    REQUIRE(visitedBy<CyclicSymmetryTable>(model, 0).fields.empty());

    expectInvalid("CyclicSymmetry", "segmentCount");
    expectInvalid("CyclicSymmetry;0;Rotational", "segmentCount");
    expectInvalid("CyclicSymmetry;-3;Rotational", "segmentCount");
    expectInvalid("CyclicSymmetry;6", "symmetryKind"); // no default on the format
    expectInvalid("CyclicSymmetry;6;XXX", "symmetryKind");

    CyclicSymmetryCard edited = cardAt<CyclicSymmetryTable>(model, 0);
    edited.segments = 8;
    REQUIRE_FALSE(CyclicSymmetryFormat::check(edited).has_value());
    model.table<CyclicSymmetryTable>().store(0, edited);
    REQUIRE(cardAt<CyclicSymmetryTable>(model, 0).segments == 8);
    edited.segments = 0;
    REQUIRE(CyclicSymmetryFormat::check(edited).has_value());
}

// --------------------------------------------------------------------------------------------------------------------
// ECHOON and ECHOOFF
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("ECHOON and ECHOOFF are Bulk Data cards with no field, kept in place, also inside a part",
          "[model][cards][monitoroutput]")
{
    REQUIRE((EchoOnMarkerTable::INFO.category == CardCategory::Other &&
             EchoOnMarkerTable::INFO.idSpace == base::IdSpace::None));
    REQUIRE((EchoOffMarkerTable::INFO.category == CardCategory::Other &&
             EchoOffMarkerTable::INFO.idSpace == base::IdSpace::None));
    // Strict mode: they are cards of the library, not unknown cards, and a part keeps its own.
    Model model = readBulk(
        "[model]\n"
        "EchoOnMarker\n"
        "Node;1;;0.;0.;0.\n"
        "EchoOffMarker\n"
        "Node;2;;1.;0.;0.\n"
        "EchoOnMarker\n"
        "[model;SuperelementPart;1]\n"
        "EchoOnMarker\n"
        "Node;3;;2.;0.;0.\n"
        "EchoOffMarker");
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    REQUIRE(rowCount<EchoOnMarkerTable>(model) == 2);
    REQUIRE(rowCount<EchoOffMarkerTable>(model) == 1);
    REQUIRE(rowCount<UnknownCardTable>(model) == 0);
    const Model& part = model.superelementModel(1);
    REQUIRE(rowCount<EchoOnMarkerTable>(part) == 1);
    REQUIRE(rowCount<EchoOffMarkerTable>(part) == 1);
    REQUIRE(rowCount<UnknownCardTable>(part) == 0);

    // Each card is written where it was.

    requireRoundTrip(model);

    // The cards have no field to read, to name or to renumber.
    REQUIRE(visitedBy<EchoOnMarkerTable>(model, 0).fields.empty());
    REQUIRE(visitedBy<EchoOffMarkerTable>(model, 0).fields.empty());
    REQUIRE((cardAt<EchoOnMarkerTable>(model) == EchoOnMarkerCard{} &&
             cardAt<EchoOffMarkerTable>(model) == EchoOffMarkerCard{}));
    REQUIRE_FALSE(EchoOnMarkerFormat::check(EchoOnMarkerCard{}).has_value());
    REQUIRE_FALSE(EchoOffMarkerFormat::check(EchoOffMarkerCard{}).has_value());
    // The executive and case sections keep their own handling of the words.
    const Model control = readModelText(
        "[executive]\nExecutiveEchoOff\nSolutionSequence;101\n[case]\nBulkEcho;None\n[model]\nEchoOnMarker\n", true);
    REQUIRE(rowCount<EchoOnMarkerTable>(control) == 1);
    REQUIRE(rowCount<EchoOffMarkerTable>(control) == 0);
}

// --------------------------------------------------------------------------------------------------------------------
// METADATA
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("a METADATA has a handle and lines of text with blanks inside", "[model][cards][monitoroutput]")
{
    REQUIRE((ResultMetadataTable::INFO.category == CardCategory::Other &&
             ResultMetadataTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "ResultMetadata;Door_Panel;Panel note line 1;Panel note line 2;Panel note line 3...\n"
        "ResultMetadata;Handle with blanks;first line;;third line\n"
        "ResultMetadata;ABCDEFGHIJKLMNOPQRSTUVWX;0123456789012345678901234567890123456789012345678901234567890123\n"
        "ResultMetadata;ALONE");
    const ResultMetadataCard fender = cardAt<ResultMetadataTable>(model, 0);
    REQUIRE(fender.handle == "Door_Panel");
    REQUIRE(fender.lines == std::vector<std::string>{"Panel note line 1", "Panel note line 2", "Panel note line 3..."});
    // A blank line in the middle is an empty text, and the handle may hold blanks.
    const ResultMetadataCard blanks = cardAt<ResultMetadataTable>(model, 1);
    REQUIRE(blanks.handle == "Handle with blanks");
    REQUIRE(blanks.lines == std::vector<std::string>{"first line", "", "third line"});
    // The longest handle (24 characters) and line (64 characters).
    const ResultMetadataCard longest = cardAt<ResultMetadataTable>(model, 2);
    REQUIRE(longest.handle.size() == 24);
    REQUIRE(longest.lines.size() == 1);
    REQUIRE(longest.lines[0].size() == 64);
    REQUIRE(cardAt<ResultMetadataTable>(model, 3).lines.empty());
    requireRoundTrip(model);

    // The small format cuts a text in pieces of eight characters and reads it back whole.

    REQUIRE(visitedBy<ResultMetadataTable>(model, 0).fields.empty());

    expectInvalid("ResultMetadata", "metadataName");
    oor::testing::requireModelFileRoundTrip(
        readModelText("ResultMetadata;ABCDEFGHIJKLMNOPQRSTUVWXY", true)); // a handle beyond the former limit
    oor::testing::requireModelFileRoundTrip(
        readModelText("ResultMetadata;H;01234567890123456789012345678901234567890123456789012345678901234",
                      true)); // a text beyond the former limit

    // Quoted records preserve punctuation and surrounding blanks; control characters are refused.
    ResultMetadataCard edited = fender;
    REQUIRE_FALSE(ResultMetadataFormat::check(edited).has_value());
    for (const char* bad : {"cost $5", "one, two", "tab\there", " leading", "trailing "})
    {
        INFO(bad);
        edited.lines = {bad};
        REQUIRE(ResultMetadataFormat::check(edited).has_value() ==
                (std::string_view(bad).find('\t') != std::string_view::npos));
    }
    edited.lines = {"text", ""};
    REQUIRE(ResultMetadataFormat::check(edited).has_value());
    edited.lines = {"", "text"};
    REQUIRE_FALSE(ResultMetadataFormat::check(edited).has_value());
    edited.lines = {};
    edited.handle = "with, comma";
    REQUIRE_FALSE(ResultMetadataFormat::check(edited).has_value());
    edited.handle = "";
    REQUIRE(ResultMetadataFormat::check(edited).has_value());
    edited.handle = "NEW";
    model.table<ResultMetadataTable>().store(0, edited);
    REQUIRE(cardAt<ResultMetadataTable>(model, 0) == edited);
}

// --------------------------------------------------------------------------------------------------------------------
// ABSNMVB, ABSNMVI, ABSNMVR, ABSNMVS and ABSTRCT
// --------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ABSNMVB has Boolean pairs from its fourth field, an ABSNMVI integer pairs and an ABSNMVR real pairs",
          "[model][cards][monitoroutput]")
{
    for (const CardTypeInfo* info : {&MetadataBooleanPairsTable::INFO, &MetadataIntegerPairsTable::INFO,
                                     &MetadataRealPairsTable::INFO, &MetadataTextPairsTable::INFO})
    {
        INFO(info->name);
        // The ID is the ID of a meta data group, shared by the cards of the group: a set ID, in no category of its own.
        REQUIRE((info->category == CardCategory::Other && info->idSpace == base::IdSpace::Group));
    }
    REQUIRE((MetadataGroupTable::INFO.category == CardCategory::MetadataGroup &&
             MetadataGroupTable::INFO.idSpace == base::IdSpace::Group));

    Model model = readBulk(
        "MetadataBooleanPairs;50;RIBBED;True;TAPERED;False\n"
        "MetadataBooleanPairs;51;P1;True;P2;False;P3;True;P4;False;P5;True\n"
        "MetadataIntegerPairs;50;BAYS;9\n"
        "MetadataIntegerPairs;52;A;-5;B;0;C;2147483647;D;7\n"
        "MetadataRealPairs;50;SWEEPA;22.;SWEEPB;35\n"
        "MetadataRealPairs;53;SMALL;1.5E-5;BIG;-2.5E+10");
    const MetadataBooleanPairsCard stiffener = cardAt<MetadataBooleanPairsTable>(model, 0);
    REQUIRE(stiffener.metadataGroupId == 50);
    REQUIRE(stiffener.pairs == std::vector<MetadataBooleanPairsPair>{{"RIBBED", true}, {"TAPERED", false}});
    // The pairs go on over the continuation lines; the case of the words is not kept, the case of the names is.
    REQUIRE(
        cardAt<MetadataBooleanPairsTable>(model, 1).pairs ==
        std::vector<MetadataBooleanPairsPair>{{"P1", true}, {"P2", false}, {"P3", true}, {"P4", false}, {"P5", true}});
    REQUIRE(cardAt<MetadataIntegerPairsTable>(model, 0).pairs == std::vector<MetadataIntegerPairsPair>{{"BAYS", 9}});
    REQUIRE(cardAt<MetadataIntegerPairsTable>(model, 1).pairs ==
            std::vector<MetadataIntegerPairsPair>{{"A", -5}, {"B", 0}, {"C", 2147483647}, {"D", 7}});
    REQUIRE(cardAt<MetadataRealPairsTable>(model, 0).pairs ==
            std::vector<MetadataRealPairsPair>{{"SWEEPA", 22.0}, {"SWEEPB", 35.0}});
    REQUIRE(cardAt<MetadataRealPairsTable>(model, 1).pairs ==
            std::vector<MetadataRealPairsPair>{{"SMALL", 1.5e-5}, {"BIG", -2.5e10}});
    requireRoundTrip(model);

    // The group is a reference: the ABSTID names the ABSTRCT cards that share it.
    for (std::uint32_t row = 0; row < 2; ++row)
    {
        const Visited visited = visitedBy<MetadataBooleanPairsTable>(model, row);
        REQUIRE(visited.fields == std::vector<std::string>{"metadataGroupId"});
        REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::MetadataGroup});
    }
    REQUIRE(visitedBy<MetadataIntegerPairsTable>(model, 0).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::MetadataGroup});
    REQUIRE(visitedBy<MetadataRealPairsTable>(model, 1).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::MetadataGroup});

    expectInvalid("MetadataBooleanPairs", "metadataGroupId");
    expectInvalid("MetadataBooleanPairs;0;A;True", "metadataGroupId");
    expectInvalid("MetadataBooleanPairs;1", "entryName");          // at least one pair
    expectInvalid("MetadataBooleanPairs;1;A;MAYBE", "entryValue"); // TRUE or FALSE
    expectInvalid("MetadataBooleanPairs;1;A;1", "entryValue");
    expectInvalid("MetadataBooleanPairs;1;A", "entryValue");    // a name needs its value
    expectInvalid("MetadataBooleanPairs;1;;True", "entryName"); // a value needs its name
    oor::testing::requireModelFileRoundTrip(
        readModelText("MetadataBooleanPairs;1;ABCDEFGHI;True", true)); // long names are preserved
    expectInvalid("MetadataIntegerPairs;1;A;1.5", "entryValue");
    expectInvalid("MetadataIntegerPairs;1;A", "entryValue");
    expectInvalid("MetadataIntegerPairs;1;A;99999999999", "entryValue");
    expectInvalid("MetadataRealPairs;1;A;TRUE", "entryValue");
    expectInvalid("MetadataRealPairs;1;A;1.5;B", "entryValue");
    expectInvalid("MetadataRealPairs;1;;1.5", "entryName");

    MetadataBooleanPairsCard edited = stiffener;
    edited.pairs.push_back(MetadataBooleanPairsPair{"NEW", true});
    REQUIRE_FALSE(MetadataBooleanPairsFormat::check(edited).has_value());
    edited.pairs.push_back(MetadataBooleanPairsPair{"WITH BLANK", true});
    REQUIRE_FALSE(MetadataBooleanPairsFormat::check(edited).has_value());
    edited.pairs.pop_back();
    edited.metadataGroupId = 0;
    REQUIRE(MetadataBooleanPairsFormat::check(edited).has_value());
}

TEST_CASE("the pairs of a meta data group name the ABSTRCT cards and follow their renumbering",
          "[model][cards][monitoroutput]")
{
    Model model = readBulk(
        "MetadataGroup;BCEVENT;1;SinglePointConstr;1\n"
        "MetadataGroup;CONSTRNT;2;SinglePointConstr;1\n"
        "MetadataGroup;CONSTRNT;2;SinglePointConstr;2\n"
        "MetadataTextPairs;1;ABSNAME;Constraint 1\n"
        "MetadataBooleanPairs;2;FLAG;True\n"
        "MetadataIntegerPairs;2;N;5\n"
        "MetadataRealPairs;2;V;1.5\n"
        "MetadataTextPairs;2;ABSNAME;x");
    REQUIRE(unresolvedIn(model) == 0);
    // Two ABSTRCT cards share the ID 2: the group of a pair card is all of them.
    Renumbering::apply(model, RenumberSpace::MetadataGroup, Renumbering::offset(10));
    REQUIRE((cardAt<MetadataGroupTable>(model, 0).id == 11 && cardAt<MetadataGroupTable>(model, 1).id == 12));
    REQUIRE(cardAt<MetadataGroupTable>(model, 2).id == 12);
    REQUIRE(cardAt<MetadataTextPairsTable>(model, 0).metadataGroupId == 11);
    REQUIRE(cardAt<MetadataBooleanPairsTable>(model).metadataGroupId == 12);
    REQUIRE(cardAt<MetadataIntegerPairsTable>(model).metadataGroupId == 12);
    REQUIRE(cardAt<MetadataRealPairsTable>(model).metadataGroupId == 12);
    REQUIRE(cardAt<MetadataTextPairsTable>(model, 1).metadataGroupId == 12);
    REQUIRE(unresolvedIn(model) == 0);

    // The ID of a pair card that no ABSTRCT has is an unresolved reference.
    Model dangling = readBulk(
        "MetadataBooleanPairs;3;A;True\nMetadataIntegerPairs;3;A;1\nMetadataRealPairs;3;A;1.\nMetadataTextPairs;3;A;b");
    REQUIRE(unresolvedIn(dangling) == 4);

    // Changing the ID of a card changes the group it belongs to.
    MetadataBooleanPairsCard edited = cardAt<MetadataBooleanPairsTable>(model);
    edited.metadataGroupId = 11;
    REQUIRE_FALSE(MetadataBooleanPairsFormat::check(edited).has_value());
    model.table<MetadataBooleanPairsTable>().store(0, edited);
    REQUIRE(cardAt<MetadataBooleanPairsTable>(model).metadataGroupId == 11);
}
