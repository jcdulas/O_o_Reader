// Checks of the aeroelastic analysis cards: AERO and AEROS (one of each per model, coordinate systems, symmetry
// keys), FLUTTER (a field that is an integer or a real by its method, the words, the FLFACT cards it names), FLFACT (a
// list, or the THRU form that keeps its form and expands by the formula of the format reference), MKAERO1 and MKAERO2
// (the tables of Mach numbers and reduced frequencies), GUST and DIVERG (the sets the Case Control selects) and CLOAD
// (a combination of superelement load vectors, filed with the loads).

#include "oor/model/cards/aero/aeroDivergence/AeroDivergenceTable.hpp"
#include "oor/model/cards/aero/aeroFlutter/AeroFlutterTable.hpp"
#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsTable.hpp"
#include "oor/model/cards/aero/aeroGust/AeroGustTable.hpp"
#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Table.hpp"
#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Table.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"
#include "oor/model/cards/loads/superelementLoadCombination/SuperelementLoadCombinationTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
using Catch::Approx;

namespace
{

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

// Three coordinate systems for the cards that name one.
const char* const COORDINATE_SYSTEMS =
    "RectFrame2;4;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
    "RectFrame2;12;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
    "RectFrame2;24;;0.;0.;0.;0.;0.;1.;1.;0.;0.";

} // namespace

TEST_CASE("an AERO has its fields in the places of the format reference and a deck has one", "[model][cards][aero]")
{
    Model model = readBulk(std::string("AeroUnsteadyParams;4;2.4E+4;80.;2.E-5;1;-1\n") + COORDINATE_SYSTEMS);
    const AeroUnsteadyParamsCard card = cardAt<AeroUnsteadyParamsTable>(model);
    REQUIRE((card.aeroFrameId == 4 && card.velocity == 24000.0 && card.referenceChord == 80.0));
    REQUIRE((card.referenceDensity == Approx(2.0E-5) && card.symmetricXz == 1 && card.symmetricXy == -1));
    requireRoundTrip(model);

    // ACSID is 0 (the basic system) when blank and VELOCITY stays blank.
    const AeroUnsteadyParamsCard minimal = cardAt<AeroUnsteadyParamsTable>(readBulk("AeroUnsteadyParams;;;1.;1."));
    REQUIRE((minimal.aeroFrameId == 0 && !minimal.velocity.has_value() && minimal.symmetricXz == 0 &&
             minimal.symmetricXy == 0));
    const AeroUnsteadyParamsCard sample =
        cardAt<AeroUnsteadyParamsTable>(readBulk("AeroUnsteadyParams;0;;3.1752;2.418E-7;1"));
    REQUIRE((sample.aeroFrameId == 0 && !sample.velocity.has_value() && sample.referenceChord == 3.1752));
    REQUIRE((sample.referenceDensity == Approx(2.418E-7) && sample.symmetricXz == 1 && sample.symmetricXy == 0));

    // The fields are in the places of the format reference: a blank ACSID and SYMXZ are blank in the text.
    const Model plain = readBulk("AeroUnsteadyParams;;25.;4.;1.225;;1");

    // Its coordinate system is the only reference, and the basic system is not one.
    RecordingVisitor visitor;
    model.table<AeroUnsteadyParamsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroFrameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate});
    Model blank = readBulk("AeroUnsteadyParams;;;1.;1.");
    RecordingVisitor none;
    blank.table<AeroUnsteadyParamsTable>().visitReferences(0, none);
    REQUIRE(none.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("AeroUnsteadyParams;7;;1.;1.");
    REQUIRE(unresolvedIn(lacking) == 1);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE(cardAt<AeroUnsteadyParamsTable>(model).aeroFrameId == 104);

    // Only one AERO is allowed: the second is reported when the deck is read.
    const Model twice = readModelText("[model]\nAeroUnsteadyParams;;;1.;1.\nAeroUnsteadyParams;;;2.;2.\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-aero-unsteady-params");

    expectInvalid("AeroUnsteadyParams;-1;;1.;1.", "aeroFrameId");
    expectInvalid("AeroUnsteadyParams;;-5.;1.;1.", "velocity");
    expectInvalid("AeroUnsteadyParams;;0.;1.;1.", "velocity");   // positive when given
    expectInvalid("AeroUnsteadyParams;;;;1.", "referenceChord"); // required
    expectInvalid("AeroUnsteadyParams;;;0.;1.", "referenceChord");
    expectInvalid("AeroUnsteadyParams;;;1.", "referenceDensity"); // required
    expectInvalid("AeroUnsteadyParams;;;1.;-1.", "referenceDensity");
    expectInvalid("AeroUnsteadyParams;;;1.;1.;2", "symmetricXz");
    expectInvalid("AeroUnsteadyParams;;;1.;1.;;-2", "symmetricXy");
    requireModelFileRoundTrip(plain);
}

TEST_CASE("an AEROS has its fields in the places of the format reference and a deck has one", "[model][cards][aero]")
{
    Model model = readBulk(std::string("AeroStaticParams;12;24;8.;60.;480.;1\n") + COORDINATE_SYSTEMS);
    const AeroStaticParamsCard card = cardAt<AeroStaticParamsTable>(model);
    REQUIRE((card.aeroFrameId == 12 && card.referenceFrameId == 24 && card.referenceChord == 8.0 &&
             card.referenceSpan == 60.0 && card.referenceArea == 480.0));
    REQUIRE((card.symmetricXz == 1 && card.symmetricXy == 0));
    requireRoundTrip(model);

    const AeroStaticParamsCard sample =
        cardAt<AeroStaticParamsTable>(readBulk("AeroStaticParams;0;100;42.;300.;12600."));
    REQUIRE((sample.aeroFrameId == 0 && sample.referenceFrameId == 100 && sample.referenceArea == 12600.0));
    const AeroStaticParamsCard full = cardAt<AeroStaticParamsTable>(readBulk("AeroStaticParams;2;3;1.5;12.;18.;-1;1"));
    REQUIRE((full.aeroFrameId == 2 && full.referenceFrameId == 3 && full.symmetricXz == -1 && full.symmetricXy == 1));
    const Model plain = readBulk("AeroStaticParams;;3;1.5;12.;18.;;1");

    RecordingVisitor visitor;
    model.table<AeroStaticParamsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"aeroFrameId", "referenceFrameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::Coordinate});
    Model basic = readBulk("AeroStaticParams;;;1.;1.;1.");
    RecordingVisitor none;
    basic.table<AeroStaticParamsTable>().visitReferences(0, none);
    REQUIRE(none.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("AeroStaticParams;1;100;42.;300.;12600.");
    REQUIRE(unresolvedIn(lacking) == 2);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE((cardAt<AeroStaticParamsTable>(model).aeroFrameId == 112 &&
             cardAt<AeroStaticParamsTable>(model).referenceFrameId == 124));

    const Model twice = readModelText("[model]\nAeroStaticParams;;;1.;1.;1.\nAeroStaticParams;;;2.;2.;2.\n", false);
    REQUIRE(twice.diagnostics().firstError()->code == "deck.duplicate-aero-static-params");

    expectInvalid("AeroStaticParams;-1;;1.;1.;1.", "aeroFrameId");
    expectInvalid("AeroStaticParams;;-2;1.;1.;1.", "referenceFrameId");
    expectInvalid("AeroStaticParams;;;0.;1.;1.", "referenceChord");
    expectInvalid("AeroStaticParams;;;1.;0.;1.", "referenceSpan");
    expectInvalid("AeroStaticParams;;;1.;1.", "referenceArea"); // required
    expectInvalid("AeroStaticParams;;;1.;1.;-1.", "referenceArea");
    expectInvalid("AeroStaticParams;;;1.;1.;1.;2", "symmetricXz");
    expectInvalid("AeroStaticParams;;;1.;1.;1.;;3", "symmetricXy");
    requireModelFileRoundTrip(plain);
}

TEST_CASE("a FLUTTER has an integer or a real in field 7 by its method and names three FLFACT cards",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroFlutter;23;KMethod;123;223;323;Surface;4;;2.E-4\n"
        "AeroFlutter;30;PkMethod;1;2;3\n"
        "AeroFlutter;60;PkMethodSweep;61;62;63;TermwiseCubic;;150.5;0.005\n"
        "AeroFlutter;70;PkMethodNoLoopSweep;71;72;73;;;12.\n"
        "AeroFlutterFactors;123;0.1\n"
        "AeroFlutterFactors;223;0.2\n"
        "AeroFlutterFactors;323;0.3\n"
        "AeroFlutterFactors;1;1.\n"
        "AeroFlutterFactors;2;0.5\n"
        "AeroFlutterFactors;3;100.\n"
        "AeroFlutterFactors;61;1.\n"
        "AeroFlutterFactors;62;0.5\n"
        "AeroFlutterFactors;63;100.\n"
        "AeroFlutterFactors;71;1.\n"
        "AeroFlutterFactors;72;0.5\n"
        "AeroFlutterFactors;73;100.");
    const AeroFlutterCard first = cardAt<AeroFlutterTable>(model, 0);
    REQUIRE((first.id == 23 && first.method == AeroFlutterMethod::K && first.densityFactorsId == 123 &&
             first.machFactorsId == 223));
    REQUIRE((first.reducedFreqFactorsId == 323 && first.interpolationMethod == AeroFlutterInterpolation::Surface &&
             first.eigenvalueCount == 4));
    REQUIRE((!first.maxFrequency.has_value() && first.epsilon == Approx(2.0E-4)));
    // The defaults: IMETH is L, NVALUE and OMAX are blank, EPS is 1.0E-3.
    const AeroFlutterCard pk = cardAt<AeroFlutterTable>(model, 1);
    REQUIRE((pk.method == AeroFlutterMethod::Pk && pk.interpolationMethod == AeroFlutterInterpolation::Linear &&
             pk.eigenvalueCount == 0));
    REQUIRE((!pk.maxFrequency.has_value() && pk.epsilon == 1.0E-3));
    // PKS and PKNLS read field 7 as OMAX, a real.
    const AeroFlutterCard sweep = cardAt<AeroFlutterTable>(model, 2);
    REQUIRE((sweep.method == AeroFlutterMethod::Pks &&
             sweep.interpolationMethod == AeroFlutterInterpolation::TermwiseCubic));
    REQUIRE((sweep.eigenvalueCount == 0 && sweep.maxFrequency == 150.5 && sweep.epsilon == Approx(5.0E-3)));
    REQUIRE((cardAt<AeroFlutterTable>(model, 3).maxFrequency == 12.0));
    requireRoundTrip(model);

    // Words are read in any case; an integer is a real where OMAX is read.
    const AeroFlutterCard lower =
        cardAt<AeroFlutterTable>(readBulk("AeroFlutter;5;PkMethodNoLoop;1;2;3;TermwiseCubic;3"));
    REQUIRE((lower.method == AeroFlutterMethod::Pknl &&
             lower.interpolationMethod == AeroFlutterInterpolation::TermwiseCubic));
    REQUIRE(lower.eigenvalueCount == 3);
    REQUIRE((cardAt<AeroFlutterTable>(readBulk("AeroFlutter;5;PkMethodNoLoopSweep;1;2;3;;7")).maxFrequency == 7.0));

    // Field 7 holds NVALUE or OMAX, whichever the method has, and the words of the format reference are written.

    const Model counted = readBulk("AeroFlutter;31;KMethodEfficient;1;2;3;;3");

    // The three FLFACT cards of every FLUTTER, and the numbers they are renumbered with.
    RecordingVisitor visitor;
    model.table<AeroFlutterTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"densityFactorsId", "machFactorsId", "reducedFreqFactorsId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroFlutterFactor,
                                                            ReferenceTarget::AeroFlutterFactor,
                                                            ReferenceTarget::AeroFlutterFactor});
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("AeroFlutter;23;KMethod;123;223;323");
    REQUIRE(unresolvedIn(lacking) == 3);
    Renumbering::apply(model, RenumberSpace::AeroFlutterFactor, Renumbering::offset(1000));
    REQUIRE((cardAt<AeroFlutterTable>(model, 0).densityFactorsId == 1123 &&
             cardAt<AeroFlutterTable>(model, 0).reducedFreqFactorsId == 1323));
    REQUIRE(cardAt<AeroFlutterFactorsTable>(model, 0).id == 1123);
    Renumbering::apply(model, RenumberSpace::AeroFlutterSet, Renumbering::offset(10));
    REQUIRE((cardAt<AeroFlutterTable>(model, 0).id == 33 && cardAt<AeroFlutterTable>(model, 3).id == 80));
    REQUIRE_THROWS_AS(readBulk("AeroFlutter;5;PkMethod;1;2;3\nAeroFlutter;5;KMethod;1;2;3"), base::DeckError);

    expectInvalid("AeroFlutter;0;PkMethod;1;2;3", "id");
    expectInvalid("AeroFlutter;5;;1;2;3", "method"); // required
    expectInvalid("AeroFlutter;5;XX;1;2;3", "method");
    expectInvalid("AeroFlutter;5;PkMethod;0;2;3", "densityFactorsId");
    expectInvalid("AeroFlutter;5;PkMethod;1;;3", "machFactorsId");
    expectInvalid("AeroFlutter;5;PkMethod;1;2", "reducedFreqFactorsId");
    expectInvalid("AeroFlutter;5;PkMethod;1;2;3;Q", "interpolationMethod");
    expectInvalid("AeroFlutter;5;KMethod;1;2;3;TermwiseCubic",
                  "interpolationMethod"); // the cubic interpolation is for the PK methods
    expectInvalid("AeroFlutter;5;KMethodEfficient;1;2;3;TermwiseCubic", "interpolationMethod");
    expectInvalid("AeroFlutter;5;KMethod;1;2;3;;;2.5", "eigenvalueCount"); // an integer for this method
    expectInvalid("AeroFlutter;5;PkMethod;1;2;3;;0", "eigenvalueCount");
    expectInvalid("AeroFlutter;5;PkMethod;1;2;3;;-3", "eigenvalueCount");
    expectInvalid("AeroFlutter;5;PkMethodSweep;1;2;3;;;-3.", "maxFrequency");
    expectInvalid("AeroFlutter;5;PkMethodSweep;1;2;3;;x", "maxFrequency");
    expectInvalid("AeroFlutter;5;PkMethod;1;2;3;;;;0.", "epsilon");
    requireModelFileRoundTrip(counted);
}

TEST_CASE("the Case Control FMETHOD selects a FLUTTER by its SID", "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\nFlutterMethodSelection;23\n[model]\nAeroFlutter;23;PkMethod;1;2;3\nAeroFlutterFactors;1;1."
        "\nAeroFlutterFactors;2;0.5\nAeroFlutterFactors;3;100.\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::AeroFlutterSet, Renumbering::offset(10));
    REQUIRE(cardAt<AeroFlutterTable>(model).id == 33);
    REQUIRE(globalCommand(model, "FlutterMethodSelection") == 33);
    Model dangling = readModelText(
        "[case]\nFlutterMethodSelection;9\n[model]\nAeroFlutter;23;PkMethod;1;2;3\nAeroFlutterFactors;1;1."
        "\nAeroFlutterFactors;2;0.5\nAeroFlutterFactors;3;100.\n",
        true);
    REQUIRE(unresolvedIn(dangling) == 1);
}

TEST_CASE("an FLFACT lists its factors or gives the THRU form, and keeps the form it was read in",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroFlutterFactors;94;0.4;0.8;2.5\n"
        "AeroFlutterFactors;98;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.\n"
        "AeroFlutterFactors;205;0.3;Through;0.1;9;0.166667\n"
        "AeroFlutterFactors;6;30.;Through;450.;30\n"
        "AeroFlutterFactors;7;-450.");
    const AeroFlutterFactorsCard list = cardAt<AeroFlutterFactorsTable>(model, 0);
    REQUIRE((list.id == 94 && list.factors == std::vector<double>{0.4, 0.8, 2.5} && !list.isRange));
    const AeroFlutterFactorsCard continued = cardAt<AeroFlutterFactorsTable>(model, 1);
    REQUIRE((continued.factors.size() == 10 && continued.factors[7] == 8.0 && continued.factors[9] == 10.0));
    const AeroFlutterFactorsCard range = cardAt<AeroFlutterFactorsTable>(model, 2);
    REQUIRE((range.isRange && range.factors.empty() && range.factor1 == 0.3 && range.factorLast == 0.1 &&
             range.factorCount == 9));
    REQUIRE((range.factorMid == 0.166667));
    const AeroFlutterFactorsCard blankMiddle = cardAt<AeroFlutterFactorsTable>(model, 3);
    REQUIRE((blankMiddle.isRange && blankMiddle.factor1 == 30.0 && blankMiddle.factorLast == 450.0 &&
             blankMiddle.factorCount == 30));
    REQUIRE(!blankMiddle.factorMid.has_value());
    REQUIRE((cardAt<AeroFlutterFactorsTable>(model, 4).factors == std::vector<double>{-450.0}));
    REQUIRE(model.findTable<AeroFlutterFactorsTable>()->factorsAt(1).size() == 10);
    requireRoundTrip(model);

    // A card read as THRU is written as THRU, with a blank FMID left blank.

    // An FLFACT names no other card; its numbers are those of the FLFACT space.
    RecordingVisitor visitor;
    model.table<AeroFlutterFactorsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    Renumbering::apply(model, RenumberSpace::AeroFlutterFactor, Renumbering::offset(100));
    REQUIRE(
        (cardAt<AeroFlutterFactorsTable>(model, 0).id == 194 && cardAt<AeroFlutterFactorsTable>(model, 2).id == 305));
    // An AEFACT and an FLFACT may share a number, two FLFACT cards may not.
    REQUIRE_NOTHROW(readBulk("AeroFactorList;5;0.3\nAeroFlutterFactors;5;0.3"));
    REQUIRE_THROWS_AS(readBulk("AeroFlutterFactors;5;0.3\nAeroFlutterFactors;5;0.4"), base::DeckError);

    expectInvalid("AeroFlutterFactors;0;0.3", "id");
    expectInvalid("AeroFlutterFactors;5", "factor1");          // at least one factor
    expectInvalid("AeroFlutterFactors;5;0.3;;0.7", "factors"); // embedded blank fields are not allowed
    expectInvalid("AeroFlutterFactors;5;0.3;LEFT", "factors");
    expectInvalid("AeroFlutterFactors;5;0.3;Through", "factorLast");
    expectInvalid("AeroFlutterFactors;5;0.3;Through;0.1", "factorCount");
    expectInvalid("AeroFlutterFactors;5;0.3;Through;0.1;0", "factorCount");
    expectInvalid("AeroFlutterFactors;5;0.3;Through;0.1;5.", "factorCount"); // an integer
    expectInvalid("AeroFlutterFactors;5;0.3;Through;0.1;2000000", "factorCount");
    expectInvalid("AeroFlutterFactors;5;0.3;Through;xx;5", "factorLast");
    expectInvalid("AeroFlutterFactors;5;0.3;Through;0.1;5;0.2;9", ""); // the THRU form has six fields
}

TEST_CASE("an MKAERO1 has its Mach numbers in the first line and its frequencies on the continuation line",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroMachFreqTable1;0.2;0.8;;;;;;;0.4;0.9;1.5\n"
        "AeroMachFreqTable1;0.1;0.2;0.3;0.4;0.5;0.6;0.7;0.8;0.01;0.02;0.05;0.1;0.2;0.5;1.;2.\n"
        "AeroMachFreqTable1;0.;0.2;;;;;;;0.001;0.05");
    const AeroMachFreqTable1Card first = cardAt<AeroMachFreqTable1Table>(model, 0);
    REQUIRE((first.machs == std::vector<double>{0.2, 0.8} && first.frequencies == std::vector<double>{0.4, 0.9, 1.5}));
    const AeroMachFreqTable1Card full = cardAt<AeroMachFreqTable1Table>(model, 1);
    REQUIRE((full.machs.size() == 8 && full.machs[7] == 0.8 && full.frequencies.size() == 8));
    REQUIRE((full.frequencies[0] == 0.01 && full.frequencies[7] == 2.0));
    // A Mach number of 0.0 is a number: the blank fields end the list.
    const AeroMachFreqTable1Card zero = cardAt<AeroMachFreqTable1Table>(model, 2);
    REQUIRE((zero.machs == std::vector<double>{0.0, 0.2} && zero.frequencies == std::vector<double>{0.001, 0.05}));
    REQUIRE(model.findTable<AeroMachFreqTable1Table>()->machsAt(1).size() == 8);
    requireRoundTrip(model);

    // The Mach numbers are in fields 1 to 8 and the frequencies on the line below.

    // A table names no other card, and a model may have any number of them.
    RecordingVisitor visitor;
    model.table<AeroMachFreqTable1Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(rowCount<AeroMachFreqTable1Table>(model) == 3);

    expectInvalid("AeroMachFreqTable1", "machNumber1");
    expectInvalid("AeroMachFreqTable1;0.1", "reducedFreq1");              // the continuation line is required
    expectInvalid("AeroMachFreqTable1;0.1;;0.5;;;;;;0.3", "machNumbers"); // a blank field ends the list
    expectInvalid("AeroMachFreqTable1;0.1;x;;;;;;;0.3", "machNumbers");
    expectInvalid("AeroMachFreqTable1;-0.1;;;;;;;;0.3", "machNumbers");
    expectInvalid("AeroMachFreqTable1;0.1;;;;;;;;0.", "reducedFreqs");
    expectInvalid("AeroMachFreqTable1;0.1;;;;;;;;0.3;;0.6", "reducedFreqs");
}

TEST_CASE("an MKAERO2 has its pairs of a Mach number and a frequency in one line", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroMachFreqPairs2;0.2;0.4;0.2;0.8;0.6;0.4;0.6;1.5\nAeroMachFreqPairs2;0.5;0.02\nAeroMachFreqPairs2;0.5;0.02;;"
        ";0.9;0.4");
    const AeroMachFreqPairs2Card full = cardAt<AeroMachFreqPairs2Table>(model, 0);
    REQUIRE(full.pairs.size() == 4);
    REQUIRE((full.pairs[0] == AeroMachFreqPairs2Pair{0.2, 0.4} && full.pairs[1] == AeroMachFreqPairs2Pair{0.2, 0.8}));
    REQUIRE((full.pairs[2] == AeroMachFreqPairs2Pair{0.6, 0.4} && full.pairs[3] == AeroMachFreqPairs2Pair{0.6, 1.5}));
    REQUIRE((cardAt<AeroMachFreqPairs2Table>(model, 1).pairs == std::vector<AeroMachFreqPairs2Pair>{{0.5, 0.02}}));
    // A pair with both fields blank is skipped: the card is written with the pairs packed.
    REQUIRE((cardAt<AeroMachFreqPairs2Table>(model, 2).pairs ==
             std::vector<AeroMachFreqPairs2Pair>{{0.5, 0.02}, {0.9, 0.4}}));
    REQUIRE(model.findTable<AeroMachFreqPairs2Table>()->pairsAt(0).size() == 4);
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AeroMachFreqPairs2Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("AeroMachFreqPairs2", "machNumber1");
    expectInvalid("AeroMachFreqPairs2;0.5", "reducedFreq"); // half a pair
    expectInvalid("AeroMachFreqPairs2;;0.5", "machNumber");
    expectInvalid("AeroMachFreqPairs2;-0.5;0.3", "machNumber");
    expectInvalid("AeroMachFreqPairs2;0.5;0.", "reducedFreq");
}

TEST_CASE("a GUST has its fields in the places of the format reference and is selected by the Case Control GUST",
          "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\nGustSelection;137\n[model]\nAeroGust;137;64;1.;0.;2.E+4\nAeroGust;3004;3004;2.4817E-4;-2.5;200.\n",
        true);
    const AeroGustCard first = cardAt<AeroGustTable>(model, 0);
    REQUIRE((first.id == 137 && first.dynamicLoadId == 64 && first.gustScale == 1.0 && first.gustOriginX == 0.0 &&
             first.velocity == 20000.0));
    const AeroGustCard second = cardAt<AeroGustTable>(model, 1);
    REQUIRE((second.id == 3004 && second.dynamicLoadId == 3004 && second.gustScale == Approx(2.4817E-4)));
    REQUIRE((second.gustOriginX == -2.5 && second.velocity == 200.0));
    requireRoundTrip(model);
    const Model plain = readBulk("AeroGust;5;6;1.5;2.5;100.");

    // DLOAD names a TLOADi or an RLOADi: a set of the dynamic loads. The GUST SID is what the command names.
    RecordingVisitor visitor;
    model.table<AeroGustTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"dynamicLoadId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicLoadSet});
    Model matching = readBulk("AeroGust;137;64;1.;0.;2.E+4\nTransientTableLoad1;64;630;;;;700");
    Model other = readBulk("AeroGust;137;64;1.;0.;2.E+4\nTransientTableLoad1;99;630;;;;700");
    REQUIRE(unresolvedIn(other) == unresolvedIn(matching) + 1);
    Renumbering::apply(matching, RenumberSpace::DynamicLoadSet, Renumbering::offset(5));
    REQUIRE(cardAt<AeroGustTable>(matching).dynamicLoadId == 69);
    REQUIRE(unresolvedIn(model) == 2); // the two DLOAD sets that no card of this deck has
    Renumbering::apply(model, RenumberSpace::AeroGustSet, Renumbering::offset(10));
    REQUIRE((cardAt<AeroGustTable>(model, 0).id == 147 && cardAt<AeroGustTable>(model, 1).id == 3014));
    REQUIRE(globalCommand(model, "GustSelection") == 147);
    Model dangling = readModelText("[case]\nGustSelection;9\n[model]\nAeroGust;137;64;1.;0.;2.E+4\n", true);
    REQUIRE(unresolvedIn(dangling) == 2); // the command and the DLOAD
    REQUIRE_THROWS_AS(readBulk("AeroGust;5;61;1.;0.;1.\nAeroGust;5;62;1.;0.;1."), base::DeckError);

    expectInvalid("AeroGust;0;61;1.;0.;1.", "id");
    expectInvalid("AeroGust;5;0;1.;0.;1.", "dynamicLoadId");
    expectInvalid("AeroGust;5;61;0.;0.;1.", "gustScale"); // a gust of no size
    expectInvalid("AeroGust;5;61;1.;;1.", "gustOriginX"); // required
    expectInvalid("AeroGust;5;61;1.;0.", "velocity");     // required
    expectInvalid("AeroGust;5;61;1.;0.;0.", "velocity");
    expectInvalid("AeroGust;5;61;1.;0.;-5.", "velocity");
    requireModelFileRoundTrip(plain);
}

TEST_CASE("a DIVERG lists its distinct Mach numbers and is selected by the Case Control DIVERG", "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\n"
        "DivergenceSelection;74\n"
        "[model]\n"
        "AeroDivergence;74;2;0.4;0.7;0.9\n"
        "AeroDivergence;71;;0.7\n"
        "AeroDivergence;72;3;0.1;0.2;0.3;0.4;0.5;0.6;0.7;0.8\n"
        "AeroDivergence;73;1;0.;0.5\n",
        true);
    const AeroDivergenceCard first = cardAt<AeroDivergenceTable>(model, 0);
    REQUIRE((first.id == 74 && first.rootCount == 2 && first.machs == std::vector<double>{0.4, 0.7, 0.9}));
    // NROOT is 1 when blank; the Mach numbers go on over the continuation lines; 0.0 is a Mach number.
    REQUIRE((cardAt<AeroDivergenceTable>(model, 1).rootCount == 1 &&
             cardAt<AeroDivergenceTable>(model, 1).machs == std::vector<double>{0.7}));
    REQUIRE(cardAt<AeroDivergenceTable>(model, 2).machs == std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
    REQUIRE((cardAt<AeroDivergenceTable>(model, 3).machs == std::vector<double>{0.0, 0.5}));
    requireRoundTrip(model);

    // The Mach numbers start at field 3, after NROOT, which is left blank when it is 1.

    RecordingVisitor visitor;
    model.table<AeroDivergenceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::DivergenceSet, Renumbering::offset(10));
    REQUIRE((cardAt<AeroDivergenceTable>(model, 0).id == 84 && cardAt<AeroDivergenceTable>(model, 3).id == 83));
    REQUIRE(globalCommand(model, "DivergenceSelection") == 84);
    Model dangling = readModelText("[case]\nDivergenceSelection;9\n[model]\nAeroDivergence;74;1;0.5\n", true);
    REQUIRE(unresolvedIn(dangling) == 1);
    REQUIRE_THROWS_AS(readBulk("AeroDivergence;5;1;0.5\nAeroDivergence;5;1;0.6"), base::DeckError);

    expectInvalid("AeroDivergence;0;1;0.5", "id");
    expectInvalid("AeroDivergence;5;0;0.5", "rootCount");
    expectInvalid("AeroDivergence;5;1.5;0.5", "rootCount");     // an integer
    expectInvalid("AeroDivergence;5;1", "machNumber1");         // at least one Mach number
    expectInvalid("AeroDivergence;5;1;0.5;0.5", "machNumber");  // the Mach numbers must be distinct
    expectInvalid("AeroDivergence;5;1;0.5;;0.7", "machNumber"); // a blank field ends the list
    expectInvalid("AeroDivergence;5;1;-0.5", "machNumber");
    expectInvalid("AeroDivergence;5;1;0.5;x", "machNumber");
}

TEST_CASE("a CLOAD combines the load vectors of a superelement and is filed with the loads", "[model][cards][aero]")
{
    Model model = readBulk(
        "SuperelementLoadCombination;35;1.;15.;10;-1.;103;0.33;605;-48.;48\n"
        "SuperelementLoadCombination;600;1.;1.;666\n"
        "PointForceLoad;10;1;;1.;1.;0.;0.\n"
        "Node;1\n"
        "StaticLoadSequence;7;103;10\n"
        "StaticLoadSequence;7;605;10\n"
        "StaticLoadSequence;7;48;10\n"
        "StaticLoadSequence;7;666;10");
    const SuperelementLoadCombinationCard first = cardAt<SuperelementLoadCombinationTable>(model, 0);
    REQUIRE((first.id == 35 && first.scaleFactor == 1.0 && first.terms.size() == 4));
    REQUIRE((first.terms[0] == SuperelementLoadCombinationTerm{15.0, 10} &&
             first.terms[1] == SuperelementLoadCombinationTerm{-1.0, 103}));
    REQUIRE((first.terms[2].scale == Approx(0.33) && first.terms[2].loadId == 605));
    REQUIRE((first.terms[3] == SuperelementLoadCombinationTerm{-48.0, 48}));
    REQUIRE((cardAt<SuperelementLoadCombinationTable>(model, 1).id == 600 &&
             cardAt<SuperelementLoadCombinationTable>(model, 1).terms ==
                 std::vector<SuperelementLoadCombinationTerm>{{1.0, 666}}));
    requireRoundTrip(model);

    // The pairs are in fields 3 and 4, 5 and 6, then from field 9 on the continuation line.

    // Each IDV is an excitation set: a load set or the EXCITEID of an LSEQ card.
    RecordingVisitor visitor;
    model.table<SuperelementLoadCombinationTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"loadIds", "loadIds", "loadIds", "loadIds"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::ExcitationSet));
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("SuperelementLoadCombination;35;1.;1.;10;2.;11");
    REQUIRE(unresolvedIn(lacking) == 2);

    // The CID is a load set: a CLOAD and a FORCE may share a number, and the renumbering of the loads follows the
    // vectors.
    REQUIRE_NOTHROW(readBulk("SuperelementLoadCombination;10;1.;1.;5\nPointForceLoad;10;1;;1.;1.;0.;0.\nNode;1"));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(100));
    REQUIRE((cardAt<SuperelementLoadCombinationTable>(model, 0).id == 135 &&
             cardAt<SuperelementLoadCombinationTable>(model, 1).id == 700));
    REQUIRE((cardAt<SuperelementLoadCombinationTable>(model, 0).terms[0].loadId == 110 &&
             cardAt<SuperelementLoadCombinationTable>(model, 0).terms[1].loadId == 103));

    expectInvalid("SuperelementLoadCombination;0;1.;1.;5", "id");
    expectInvalid("SuperelementLoadCombination;25;;1.;5", "scaleFactor");
    expectInvalid("SuperelementLoadCombination;25;1.", "scaleFactor1"); // at least one load vector
    expectInvalid("SuperelementLoadCombination;25;1.;x;5", "scaleFactor1");
    expectInvalid("SuperelementLoadCombination;25;1.;1.", "loadIds");
    expectInvalid("SuperelementLoadCombination;25;1.;1.;0", "loadIds");
    expectInvalid("SuperelementLoadCombination;25;1.;1.;-3", "loadIds");
    expectInvalid("SuperelementLoadCombination;25;1.;1.;5;2.", "loadIds"); // S2 with no IDV2
}

TEST_CASE("the Case Control CLOAD selects a CLOAD by its CID", "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\nSuperelementLoadCombinationSelection;35\n[model]\nSuperelementLoadCombination;35;1.;1.;10\n", true);
    const std::size_t missing = unresolvedIn(model); // the load vector 10 of the superelement is not in the deck
    REQUIRE(missing == 1);
    Model dangling = readModelText(
        "[case]\nSuperelementLoadCombinationSelection;9\n[model]\nSuperelementLoadCombination;35;1.;1.;10\n", true);
    REQUIRE(unresolvedIn(dangling) == missing + 1);
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(100));
    REQUIRE((cardAt<SuperelementLoadCombinationTable>(model).id == 135 &&
             globalCommand(model, "SuperelementLoadCombinationSelection") == 135));
}
