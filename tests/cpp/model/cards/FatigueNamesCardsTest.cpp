// Checks of the fatigue and file name cards: the fatigue entries (FTGDEF, FTGEVNT, FTGLOAD, FTGPARM, FTGSEQ, PFTG,
// TIM2PSD), the file names and the HDF5 output entries (UDNAME, HDF5DA, H5OUT, HDF5OUT), the TABLRPC table, and the
// MATDIGI that now names its UDNAME. Per card: the fields in their format reference positions, the defaults, the
// formats of the format that one card reads (told apart by their fields), the references and what each points to, the
// renumbering of the spaces the cards own, the refused values and the round trip through a model file. The generic
// tests of CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/materials/micromechanicsCompositeMat/MicromechanicsCompositeMatTable.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"
#include "oor/model/cards/solution_control/fatigueDefinition/FatigueDefinitionTable.hpp"
#include "oor/model/cards/solution_control/fatigueEvent/FatigueEventTable.hpp"
#include "oor/model/cards/solution_control/fatigueLoad/FatigueLoadTable.hpp"
#include "oor/model/cards/solution_control/fatigueParams/FatigueParamsTable.hpp"
#include "oor/model/cards/solution_control/fatigueProp/FatiguePropTable.hpp"
#include "oor/model/cards/solution_control/fatigueSequence/FatigueSequenceTable.hpp"
#include "oor/model/cards/solution_control/moduleResultFileParams/ModuleResultFileParamsTable.hpp"
#include "oor/model/cards/solution_control/resultFileModules/ResultFileModulesTable.hpp"
#include "oor/model/cards/solution_control/resultFileParams/ResultFileParamsTable.hpp"
#include "oor/model/cards/solution_control/timeToSpectrum/TimeToSpectrumTable.hpp"
#include "oor/model/cards/solution_control/userFileName/UserFileNameTable.hpp"
#include "oor/model/cards/tables/tableTimeHistoryFile/TableTimeHistoryFileTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The references that name nothing in a model that was just read (the support function takes a model that stays).
std::size_t unresolvedIn(Model&& model)
{
    return testing::unresolvedIn(model);
}

// The references a card lists: the field names and what each one points to, as a visitor sees them.
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

// A name of a file of about `length` characters, made of folders so that it has no blank and no special character.
std::string longPath(std::size_t length)
{
    std::string path = "/data";
    while (path.size() < length)
        path += "/dir" + std::to_string(path.size());
    return path + "/file.h5";
}

// A card with an ID in the table of its type, given a new ID, a clash and a deletion.
template <typename TableT>
void requireIdEdits(Model& model, base::EntityId first, base::EntityId second, base::EntityId unused)
{
    const std::uint16_t type = typeIndexOf<TableT>();
    const auto where = model.locateInType(type, first);
    REQUIRE(where.has_value());
    model.setId(*where, unused);
    REQUIRE(TableT::cardId(cardAt<TableT>(model, where->row)) == unused);
    REQUIRE_FALSE(model.locateInType(type, first).has_value());
    REQUIRE_THROWS_AS(model.setId(*model.locateInType(type, second), unused), base::DuplicateIdError);
    model.remove(*model.locateInType(type, unused));
    REQUIRE_FALSE(model.locateInType(type, unused).has_value());
    REQUIRE(model.locateInType(type, second).has_value());
}

using Ids = std::vector<base::EntityId>;

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// The lines of a card (FatigueLines.hpp)
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE(
    "the fields of a continuation line of the fatigue cards are numbered from the format reference field 2 of their "
    "line",
    "[model][cards][fatigue]")
{
    // The format reference counts the card name as field 1, the reader does not: the ID is reader field 1, the first
    // field of the first continuation line (format reference field 2, where a section has its keyword) is reader
    // field 9.
    REQUIRE(fatigueField(0, 2) == 1);
    REQUIRE(fatigueField(0, 9) == 8);
    REQUIRE(fatigueField(1, 2) == 9);
    REQUIRE(fatigueField(1, 3) == 10);
    REQUIRE(fatigueField(2, 2) == 17);

    // Names preserve long text and record punctuation; control characters remain invalid.
    REQUIRE_FALSE(checkSpannedText("a/b.h5", "entryName").has_value());
    REQUIRE(checkSpannedText("", "entryName").has_value());
    REQUIRE_FALSE(checkSpannedText(std::string(221, 'a'), "entryName").has_value());
    for (const std::string_view bad : {"a b", "a,b", "a$b", "a%b", "a|b", "a'b", "a\"b", "a\tb"})
        REQUIRE(checkSpannedText(bad, "entryName").has_value() == (bad.find('\t') != std::string_view::npos));
}

// ---------------------------------------------------------------------------------------------------------------------
// UDNAME
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a UDNAME keeps the name of a file over the continuation lines whatever the format",
          "[model][cards][fatigue]")
{
    const std::string longName = longPath(130);
    Model model = readBulk(
        "UserFileName;92001;D:/wing_runs/gust_case12.h5\nUserFileName;92002;D:\\runs\\road_input.rsp\n"
        "UserFileName;92003;model.h5\nUserFileName;92004;" +
        longName);
    REQUIRE((UserFileNameTable::INFO.category == CardCategory::UserDefinedName &&
             UserFileNameTable::INFO.idSpace == base::IdSpace::PerCardType));
    REQUIRE((cardAt<UserFileNameTable>(model, 0).id == 92001 &&
             cardAt<UserFileNameTable>(model, 0).entryName == "D:/wing_runs/gust_case12.h5"));
    REQUIRE(cardAt<UserFileNameTable>(model, 1).entryName == "D:\\runs\\road_input.rsp");
    REQUIRE(cardAt<UserFileNameTable>(model, 2).entryName == "model.h5");
    REQUIRE(cardAt<UserFileNameTable>(model, 3).entryName == longName);
    REQUIRE(longName.size() > 128);
    requireRoundTrip(model);

    // The name is cut in pieces of eight characters in the small format, whole in the free format, on the second line.

    // The name is written, read and edited as the format says: a blank, a bar or a quote is refused.
    UserFileNameCard edited = cardAt<UserFileNameTable>(model, 0);
    edited.entryName = "a b.h5";
    REQUIRE_FALSE(UserFileNameFormat::check(edited).has_value());
    edited.entryName = std::string(512, 'x');
    REQUIRE_FALSE(UserFileNameFormat::check(edited).has_value());
    edited.entryName += "x";
    REQUIRE_FALSE(UserFileNameFormat::check(edited).has_value());
    edited.entryName = "other.h5";
    model.table<UserFileNameTable>().store(0, edited);

    expectInvalid("UserFileName;0;file.h5", "id");
    expectInvalid("UserFileName;5", "entryName"); // the name is required
    oor::testing::requireModelFileRoundTrip(readModelText("UserFileName;5;my file.h5", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserFileName;5;a%b.h5", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserFileName;5;a|b.h5", true));
    oor::testing::requireModelFileRoundTrip(readModelText("UserFileName;5;\"a\"\"b.h5\"", true));
    expectInvalid("UserFileName;x;file.h5", "id");
}

TEST_CASE("a UDNAME is found by the cards that name a file and follows its renumbering", "[model][cards][fatigue]")
{
    Model model = readBulk("UserFileName;10;a.h5\nUserFileName;11;b.h5\nUserFileName;12;c.h5");
    REQUIRE(visitedBy<UserFileNameTable>(model, 0).fields.empty()); // a UDNAME names no other card
    requireIdEdits<UserFileNameTable>(model, 10, 11, 20);
    REQUIRE(unresolvedIn(readBulk("UserFileName;10;a.h5\nMicromechanicsCompositeMat;6;10")) == 0);
    REQUIRE(unresolvedIn(readBulk("MicromechanicsCompositeMat;6;10")) == 1);
}

// ---------------------------------------------------------------------------------------------------------------------
// PFTG
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a PFTG has the format reference defaults and needs a roughness for the finishes KROUGH and KSURFC",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueProp;92001;GroundSurface;;;1.35\n"
        "FatigueProp;92002\n"
        "FatigueProp;92003;RoughnessFactor;0.5;1.2;1.1;5.;0.9;6.;1.5;1.2;1\n"
        "FatigueProp;92004;SurfaceFinishFactor;12.5\n"
        "FatigueProp;92005;MachinedSurface");
    REQUIRE((FatiguePropTable::INFO.category == CardCategory::FatigueProperty &&
             FatiguePropTable::INFO.idSpace == base::IdSpace::PerCardType));
    const FatiguePropCard sampleCard = cardAt<FatiguePropTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.surfaceFinish == FatiguePropFinish::Ground &&
             sampleCard.scaleFactor == 1.35));
    REQUIRE((sampleCard.fatigueNotchFactor == 1.0 && sampleCard.offset == 0.0 && sampleCard.treatmentFactor == 1.0 &&
             sampleCard.spotWeldFlag == 0));
    REQUIRE_FALSE((sampleCard.finishFactor || sampleCard.diameter || sampleCard.thickness1 || sampleCard.thickness2));
    const FatiguePropCard bare = cardAt<FatiguePropTable>(model, 1);
    REQUIRE(
        (bare.surfaceFinish == FatiguePropFinish::None && bare.scaleFactor == 1.0 && bare.fatigueNotchFactor == 1.0));
    const FatiguePropCard full = cardAt<FatiguePropTable>(model, 2);
    REQUIRE((full.surfaceFinish == FatiguePropFinish::Krough && full.finishFactor == 0.5 &&
             full.fatigueNotchFactor == 1.2 && full.scaleFactor == 1.1));
    REQUIRE((full.offset == 5.0 && full.treatmentFactor == 0.9 && full.diameter == 6.0 && full.thickness1 == 1.5 &&
             full.thickness2 == 1.2));
    REQUIRE(full.spotWeldFlag == 1);
    REQUIRE((cardAt<FatiguePropTable>(model, 3).surfaceFinish == FatiguePropFinish::Ksurfc &&
             cardAt<FatiguePropTable>(model, 3).finishFactor == 12.5));
    REQUIRE(cardAt<FatiguePropTable>(model, 4).surfaceFinish ==
            FatiguePropFinish::Machine); // the words are read in any case
    requireRoundTrip(model);

    // The defaults are blanks, the fields of the second line are in their places.

    REQUIRE(visitedBy<FatiguePropTable>(model, 0).fields.empty()); // a PFTG names no other card
    requireIdEdits<FatiguePropTable>(model, 92001, 92002, 92100);

    FatiguePropCard edited = full;
    edited.thickness2.reset();
    REQUIRE(std::string_view(FatiguePropFormat::check(edited)->field) == "thickness2"); // both thicknesses or none
    edited.thickness1.reset();
    REQUIRE_FALSE(FatiguePropFormat::check(edited).has_value());
    edited.surfaceFinish = FatiguePropFinish::Ksurfc;
    edited.finishFactor.reset();
    REQUIRE(std::string_view(FatiguePropFormat::check(edited)->field) == "finishFactor");

    expectInvalid("FatigueProp;0", "id");
    expectInvalid("FatigueProp", "id");
    expectInvalid("FatigueProp;1;SHINY", "surfaceFinish");
    expectInvalid("FatigueProp;1;RoughnessFactor", "finishFactor"); // required for these two finishes
    expectInvalid("FatigueProp;1;SurfaceFinishFactor", "finishFactor");
    expectInvalid("FatigueProp;1;RoughnessFactor;1.5", "finishFactor");
    expectInvalid("FatigueProp;1;RoughnessFactor;0.", "finishFactor");
    expectInvalid("FatigueProp;1;SurfaceFinishFactor;-1.", "finishFactor");
    expectInvalid("FatigueProp;1;;;-1.", "fatigueNotchFactor");
    expectInvalid("FatigueProp;1;;;;;;-1.", "treatmentFactor");
    expectInvalid("FatigueProp;1;;;;;;;0.", "diameter");
    expectInvalid("FatigueProp;1;;;;;;;;1.5", "thickness2");
    expectInvalid("FatigueProp;1;;;;;;;;;1.5", "thickness1");
    expectInvalid("FatigueProp;1;;;;;;;;-1.;1.", "thickness1");
    expectInvalid("FatigueProp;1;;;;;;;;;;2", "spotWeldFlag");
}

// ---------------------------------------------------------------------------------------------------------------------
// FTGEVNT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FTGEVNT lists FTGLOAD cards over the lines and may end with an event name", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueEvent;92001;2;7;13;Name;pothole_series\n"
        "FatigueEvent;92002;1;4\n"
        "FatigueEvent;92003;9;1;2;3;4;5;6;7;8;9\n"
        "FatigueEvent;92004;8;1;2;3;4;5;6;7;8;Name;long_event_name\n"
        "FatigueEvent;92005;15;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15");
    REQUIRE((FatigueEventTable::INFO.category == CardCategory::FatigueEvent &&
             FatigueEventTable::INFO.idSpace == base::IdSpace::PerCardType));
    const FatigueEventCard sampleCard = cardAt<FatigueEventTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.fatigueLoadIds == Ids{7, 13} &&
             sampleCard.entryName == "pothole_series"));
    REQUIRE((cardAt<FatigueEventTable>(model, 1).fatigueLoadIds == Ids{4} &&
             cardAt<FatigueEventTable>(model, 1).entryName.empty()));
    REQUIRE(cardAt<FatigueEventTable>(model, 2).fatigueLoadIds ==
            Ids{1, 2, 3, 4, 5, 6, 7, 8, 9}); // seven on a line, then seven more
    const FatigueEventCard named = cardAt<FatigueEventTable>(model, 3);
    REQUIRE((named.fatigueLoadIds == Ids{1, 2, 3, 4, 5, 6, 7, 8} && named.entryName == "long_event_name"));
    REQUIRE(cardAt<FatigueEventTable>(model, 4).fatigueLoadIds.size() == 15);
    requireRoundTrip(model);

    // The name has the line after the last line of loads, in fields 3 to 9 of the format after the keyword NAME.

    const Visited visited = visitedBy<FatigueEventTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"fatigueLoadIds", "fatigueLoadIds"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::FatigueLoad));
    requireIdEdits<FatigueEventTable>(model, 92001, 92002, 92100);

    expectInvalid("FatigueEvent;0;1;1", "id");
    expectInvalid("FatigueEvent;5", "fatigueLoadIds"); // at least one load
    expectInvalid("FatigueEvent;5;1;0", "fatigueLoadIds");
    expectInvalid("FatigueEvent;5;1;x", "fatigueLoadIds");
    expectInvalid("FatigueEvent;5;1;1;Name", "eventName");
    requireRoundTrip(readBulk("FatigueEvent;5;1;1;Name;a b")); // a blank ends the name
    requireRoundTrip(readBulk("FatigueEvent;5;1;1;Name;" + std::string(157, 'a')));
}

// ---------------------------------------------------------------------------------------------------------------------
// FTGLOAD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FTGLOAD of SOL 101 and 103 has a table or a file, its defaults, and the UNITS line",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueLoad;92001;6;3;1.;1.;0.\n"
        "FatigueLoad;92002;4;5;1.;1.;0.;Decibel;;Units;3.5;Orbits\n"
        "FatigueLoad;92003;4;1\n"
        "FatigueLoad;92004;;1;;3.;-2.;ConstantLoad\n"
        "FatigueLoad;92005;;1;;2.;;ConstantLoad\n"
        "FatigueLoad;92006;92011;1;1.;1.;0.;LoadHistoryFile;3\n"
        "FatigueLoad;92007;92011;1;0.5;;;Static\n"
        "FatigueLoad;92008;4;1;;;;;;Units");
    REQUIRE((FatigueLoadTable::INFO.category == CardCategory::FatigueLoad &&
             FatigueLoadTable::INFO.idSpace == base::IdSpace::PerCardType));
    const FatigueLoadCard sampleCard = cardAt<FatigueLoadTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.tableId == 6 && sampleCard.loadCaseId == 3 &&
             sampleCard.referenceLoadMagnitude == 1.0 && sampleCard.scaleFactor == 1.0 && sampleCard.offset == 0.0));
    REQUIRE_FALSE((sampleCard.type || sampleCard.units || sampleCard.channel != 0 || !sampleCard.tones.empty() ||
                   sampleCard.sweep));
    const FatigueLoadCard units = cardAt<FatigueLoadTable>(model, 1);
    REQUIRE((units.type == FatigueLoadType::Db && units.units.has_value()));
    REQUIRE((units.units->equivalentUnits == 3.5 && units.units->name == "Orbits"));
    // The defaults: LDM and SCALE 1.0, OFFSET 0.0, and -1.0 for the second peak of a constant amplitude cycle.
    const FatigueLoadCard bare = cardAt<FatigueLoadTable>(model, 2);
    REQUIRE((bare.referenceLoadMagnitude == 1.0 && bare.scaleFactor == 1.0 && bare.offset == 0.0 && !bare.type));
    const FatigueLoadCard peaks = cardAt<FatigueLoadTable>(model, 3);
    REQUIRE((peaks.type == FatigueLoadType::Const && peaks.tableId == 0 && peaks.scaleFactor == 3.0 &&
             peaks.offset == -2.0));
    REQUIRE(cardAt<FatigueLoadTable>(model, 4).offset == -1.0); // a blank MIN, resolved from the type
    const FatigueLoadCard fromFile = cardAt<FatigueLoadTable>(model, 5);
    REQUIRE((fromFile.type == FatigueLoadType::LoadHistoryFile && fromFile.channel == 3 && fromFile.tableId == 92011));
    REQUIRE((cardAt<FatigueLoadTable>(model, 6).type == FatigueLoadType::Static &&
             cardAt<FatigueLoadTable>(model, 6).referenceLoadMagnitude == 0.5));
    const FatigueLoadCard defaultUnits = cardAt<FatigueLoadTable>(model, 7);
    REQUIRE((defaultUnits.units.has_value() && defaultUnits.units->equivalentUnits == 1.0 &&
             defaultUnits.units->name.empty()));
    requireRoundTrip(model);

    // The offset depends on the type, so it is always written; the other defaults are blanks.

    // The cards of the two formats are told apart by their lines: a UNITS line, or the DETLOAD and SWEEP lines.
    FatigueLoadCard mixed = units;
    mixed.tones.push_back(FatigueLoadTone{10.0, 1.0, std::nullopt});
    REQUIRE(std::string_view(FatigueLoadFormat::check(mixed)->field) == "Units");

    requireIdEdits<FatigueLoadTable>(model, 92001, 92002, 92100);

    expectInvalid("FatigueLoad;0;1;1", "id");
    expectInvalid("FatigueLoad;5;1", "loadCaseId"); // no default
    expectInvalid("FatigueLoad;5;;1", "tableId");   // a table or a file is needed for a blank TYPE
    expectInvalid("FatigueLoad;5;;1;;;;Decibel", "tableId");
    expectInvalid("FatigueLoad;5;;1;;;;LoadHistoryFile", "tableId");
    expectInvalid("FatigueLoad;5;1;1;;;;WAVE", "historyKind");
    expectInvalid("FatigueLoad;5;-1;1", "tableId");
    expectInvalid("FatigueLoad;5;1;1;0.", "referenceLoadMagnitude");
    expectInvalid("FatigueLoad;5;1;1;;;;;-1", "channel");
    expectInvalid("FatigueLoad;5;1;1;;;;;;Units;0.", "equivalentUnits");
    oor::testing::requireModelFileRoundTrip(readModelText("FatigueLoad;5;1;1;;;;;;Units;1.;a b", true));
    expectInvalid("FatigueLoad;5;1;1;;;;;;Units;;;Units", "Units");
    expectInvalid("FatigueLoad;5;1;1;;;;;;Units;;;DeterministicLoad;1;1.;1.", "Units");
}

TEST_CASE("an FTGLOAD of SOL 108 and 111 has the DETLOAD and SWEEP lines of its type", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueLoad;92009;;3;;1.;;SineLoad;;DeterministicLoad;3;15.;4.;;25.;5.;;40.;8.\n"
        "FatigueLoad;92010;;4;;1.;;NarrowBand;;DeterministicLoad;2;45.;3.;4.;90.;2.5;5.\n"
        "FatigueLoad;92011;;5;;1.;;SineLoad;;SweepLoad;15.;3.;250.;5.;Decibel;30;0.4;3;Force;Velocity\n"
        "FatigueLoad;92012;;6;;1.;;SineLoad;;SweepLoad;15.;3.;250.;5.;;;1.;2\n"
        "FatigueLoad;92013;;7;;1.;;SineLoad;;DeterministicLoad;1;12.;2.5;;SweepLoad;15.;3.;250.;5.;FreqInHertz;10;1.;"
        "2\n"
        "FatigueLoad;92014;92011;-1;;;;SpectralDensityLoad\n"
        "FatigueLoad;92015;;92021;;3.;;SpectralDensityLoad");
    const FatigueLoadCard sines = cardAt<FatigueLoadTable>(model, 0);
    REQUIRE((sines.type == FatigueLoadType::Sine && sines.tableId == 0 && sines.loadCaseId == 3));
    REQUIRE(sines.tones.size() == 3); // two bands on the first line, the third on the next
    REQUIRE((sines.tones[0] == FatigueLoadTone{15.0, 4.0, std::nullopt} &&
             sines.tones[1] == FatigueLoadTone{25.0, 5.0, std::nullopt}));
    REQUIRE(sines.tones[2] == FatigueLoadTone{40.0, 8.0, std::nullopt});
    const FatigueLoadCard bands = cardAt<FatigueLoadTable>(model, 1);
    REQUIRE((bands.type == FatigueLoadType::Narrow && bands.tones.size() == 2));
    REQUIRE((bands.tones[0] == FatigueLoadTone{45.0, 3.0, 4.0} && bands.tones[1] == FatigueLoadTone{90.0, 2.5, 5.0}));
    const FatigueLoadCard sweep = cardAt<FatigueLoadTable>(model, 2);
    REQUIRE(sweep.sweep.has_value());
    REQUIRE((sweep.sweep->firstFrequency == 15.0 && sweep.sweep->firstHeight == 3.0 &&
             sweep.sweep->lastFrequency == 250.0 && sweep.sweep->lastHeight == 5.0));
    REQUIRE((sweep.sweep->type == FatigueLoadSweepType::Db && sweep.sweep->count == 30 && sweep.sweep->rate == 0.4 &&
             sweep.sweep->passes == 3));
    REQUIRE(
        (sweep.sweep->frfUnit == FatigueLoadQuantity::Force && sweep.sweep->loadUnit == FatigueLoadQuantity::Velocity));
    // The defaults of the second line of a SWEEP: HZ, 50 sine waves, and A for the two units.
    const FatigueLoadCard defaults = cardAt<FatigueLoadTable>(model, 3);
    REQUIRE((defaults.sweep->type == FatigueLoadSweepType::Hz && defaults.sweep->count == 50 &&
             defaults.sweep->rate == 1.0 && defaults.sweep->passes == 2));
    REQUIRE((defaults.sweep->frfUnit == FatigueLoadQuantity::Acceleration &&
             defaults.sweep->loadUnit == FatigueLoadQuantity::Acceleration));
    const FatigueLoadCard both = cardAt<FatigueLoadTable>(model, 4);
    REQUIRE((both.tones.size() == 1 && both.sweep.has_value() && both.sweep->count == 10));
    requireRoundTrip(model);

    // The lines are written in the order of the format: DETLOAD, then SWEEP and its second line.

    // (the defaults SCALE 1.0, SWTYPE HZ and SWNUM 50 are blanks; the OFFSET is always written)

    // What TID and LCID name depends on the type. A PSD load names the RANDPS set of its LCID, and the file of a time
    // history of a TIM2PSD by its TID with a negative LCID, which is not a reference.
    REQUIRE(visitedBy<FatigueLoadTable>(model, 0).fields.empty());
    const Visited psd = visitedBy<FatigueLoadTable>(model, 5);
    REQUIRE(psd.fields == std::vector<std::string>{"tableId"});
    REQUIRE(psd.targets == std::vector<ReferenceTarget>{ReferenceTarget::UserDefinedName});
    const Visited random = visitedBy<FatigueLoadTable>(model, 6);
    REQUIRE(random.fields == std::vector<std::string>{"loadCaseId"});
    REQUIRE(random.targets == std::vector<ReferenceTarget>{ReferenceTarget::RandomSet});

    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad", "bandFrequency");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad;1;0.;1.", "bandFrequency");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad;1;1.;-1.", "bandAmplitude");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad;1;1.;1.;-1.", "bandWidth");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad;1;1.", "bandAmplitude");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;DeterministicLoad;1;1.;1.;;DeterministicLoad;1;3.;3.",
                  "DeterministicLoad");
    const std::string sweepLine = "FatigueLoad;5;;1;;;;SineLoad;;SweepLoad;15.;3.;250.;5.";
    expectInvalid(sweepLine, "sweepRate"); // the second line is needed
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;SweepLoad;0.;3.;250.;5.;;;1.;2", "sweepStartFrequency");
    expectInvalid("FatigueLoad;5;;1;;;;SineLoad;;SweepLoad;15.;3.;;5.;;;1.;2", "sweepEndFrequency");
    expectInvalid(sweepLine + ";XX;10;1.;2", "sweepKind");
    expectInvalid(sweepLine + ";;0;1.;2", "sweepNumber");
    expectInvalid(sweepLine + ";;10;0.;2", "sweepRate");
    expectInvalid(sweepLine + ";;10;1.", "sweepCount");
    expectInvalid(sweepLine + ";;10;1.;0", "sweepCount");
    expectInvalid(sweepLine + ";;10;1.;2;Q", "transferFunctionQuantity");
    expectInvalid(sweepLine + ";;10;1.;2;;Q", "loadQuantity");
}

TEST_CASE("an FTGLOAD names a table, a file, or a random set, and the references follow their renumbering",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "UserFileName;10;a.h5\n"
        "TableFatigueHistory;1;0.;1.;EndTable\n"
        "RandomSpectralDensity;7;1;3\n"
        "FatigueLoad;20;1;1\n"
        "FatigueLoad;21;10;1;;;;LoadHistoryFile\n"
        "FatigueLoad;22;;7;;;;SpectralDensityLoad\n"
        "FatigueLoad;23;;1;;1.;;ConstantLoad");
    REQUIRE(unresolvedIn(model) == 0);
    const Visited table = visitedBy<FatigueLoadTable>(model, 0);
    REQUIRE((table.fields == std::vector<std::string>{"tableId"} &&
             table.targets == std::vector<ReferenceTarget>{ReferenceTarget::FatigueTable}));
    REQUIRE(visitedBy<FatigueLoadTable>(model, 1).targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::UserDefinedName});
    REQUIRE(visitedBy<FatigueLoadTable>(model, 3).fields.empty()); // CONST ignores its table and names no file

    // A table that no card has, and a file that no UDNAME names, are reported; so is a RANDPS set that does not exist.
    REQUIRE(unresolvedIn(readBulk("FatigueLoad;20;1;1")) == 1);
    REQUIRE(unresolvedIn(readBulk("FatigueLoad;21;10;1;;;;LoadHistoryFile")) == 1);
    REQUIRE(unresolvedIn(readBulk("FatigueLoad;22;;7;;;;SpectralDensityLoad")) == 1);
    REQUIRE(unresolvedIn(readBulk("FatigueLoad;23;;1;;1.;;ConstantLoad")) == 0);

    // Each reference follows the renumbering of its own space, and only that one.
    Renumbering::apply(model, RenumberSpace::FatigueTable, Renumbering::offset(100));
    REQUIRE((cardAt<FatigueLoadTable>(model, 0).tableId == 101 && cardAt<FatigueLoadTable>(model, 1).tableId == 10));
    Renumbering::apply(model, RenumberSpace::UserDefinedName, Renumbering::offset(1000));
    REQUIRE((cardAt<FatigueLoadTable>(model, 1).tableId == 1010 && cardAt<UserFileNameTable>(model, 0).id == 1010));
    REQUIRE(cardAt<FatigueLoadTable>(model, 0).tableId == 101);
    Renumbering::apply(model, RenumberSpace::Random, Renumbering::offset(50));
    REQUIRE(
        (cardAt<FatigueLoadTable>(model, 2).loadCaseId == 57 && cardAt<FatigueLoadTable>(model, 0).loadCaseId == 1));
    Renumbering::apply(model, RenumberSpace::FatigueLoad, Renumbering::offset(500));
    REQUIRE((cardAt<FatigueLoadTable>(model, 0).id == 520 && cardAt<FatigueLoadTable>(model, 3).id == 523));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// FTGSEQ
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FTGSEQ lists events and repeats, in the format of SOL 101, 103, 112 or of SOL 108, 111",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueSequence;92001;;;;;1;8;2.5\n"
        "FatigueSequence;92002;1;0;;;5;92011;4.;92012;7.;92013;1.5;92014;9.;92015;;Units;3.5;Orbits\n"
        "FatigueSequence;92003;1;;TimeInHours;2.5;2;92011;12.;92012;18.\n"
        "FatigueSequence;92004;;;;;2;92011;;92012\n"
        "FatigueSequence;92005;;2;;;1;7;3.\n"
        "FatigueSequence;92006;;;;;1;7;;Units");
    REQUIRE((FatigueSequenceTable::INFO.category == CardCategory::FatigueEvent &&
             FatigueSequenceTable::INFO.idSpace == base::IdSpace::PerCardType));
    const FatigueSequenceCard sampleCard = cardAt<FatigueSequenceTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.eventOutput == 0 && sampleCard.combinationMethod == 0 &&
             sampleCard.timeUnit == FatigueSequenceTimeUnit::Secs && sampleCard.referenceLoadMagnitude == 1.0));
    REQUIRE((sampleCard.items == std::vector<FatigueSequenceItem>{{8, 2.5}} && !sampleCard.units));
    const FatigueSequenceCard full = cardAt<FatigueSequenceTable>(model, 1);
    REQUIRE(full.eventOutput == 1);
    REQUIRE(full.items.size() == 5); // four pairs on a line, then one more
    REQUIRE((full.items[0] == FatigueSequenceItem{92011, 4.0} && full.items[3] == FatigueSequenceItem{92014, 9.0}));
    REQUIRE(full.items[4] == FatigueSequenceItem{92015, 1.0}); // a blank Ni is 1.0
    REQUIRE((full.units.has_value() && full.units->equivalentUnits == 3.5 && full.units->name == "Orbits"));
    // The format of SOL 108 and 111: TUNIT and LDM in the places after the blank METHOD.
    const FatigueSequenceCard random = cardAt<FatigueSequenceTable>(model, 2);
    REQUIRE((random.timeUnit == FatigueSequenceTimeUnit::Hrs && random.referenceLoadMagnitude == 2.5 &&
             random.combinationMethod == 0 && random.eventOutput == 1));
    REQUIRE(
        (cardAt<FatigueSequenceTable>(model, 3).items == std::vector<FatigueSequenceItem>{{92011, 1.0}, {92012, 1.0}}));
    REQUIRE((cardAt<FatigueSequenceTable>(model, 4).combinationMethod == 2 &&
             cardAt<FatigueSequenceTable>(model, 4).eventOutput == 0));
    REQUIRE((cardAt<FatigueSequenceTable>(model, 5).units.has_value() &&
             cardAt<FatigueSequenceTable>(model, 5).units->name.empty()));
    requireRoundTrip(model);

    // Each FIDi is an event or a sequence; a number that is neither is reported.
    const Visited visited = visitedBy<FatigueSequenceTable>(model, 1);
    REQUIRE(visited.fields == std::vector<std::string>(5, "functionId1"));
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(5, ReferenceTarget::FatigueEvent));
    requireIdEdits<FatigueSequenceTable>(model, 92001, 92002, 92100);

    FatigueSequenceCard mixed = random;
    mixed.combinationMethod = 1;
    REQUIRE(std::string_view(FatigueSequenceFormat::check(mixed)->field) ==
            "combinationMethod"); // one format or the other

    expectInvalid("FatigueSequence;0;;;;;1;1", "id");
    expectInvalid("FatigueSequence;5", "functionId1");
    expectInvalid("FatigueSequence;5;2;;;;1;1", "eventOutput");
    expectInvalid("FatigueSequence;5;;3;;;1;1", "combinationMethod");
    expectInvalid("FatigueSequence;5;;1;TimeInHours;;1;1", "combinationMethod");
    expectInvalid("FatigueSequence;5;;1;;2.;1;1", "combinationMethod");
    expectInvalid("FatigueSequence;5;;;YEARS;;1;1", "timeUnit");
    expectInvalid("FatigueSequence;5;;;;0.;1;1", "referenceLoadMagnitude");
    expectInvalid("FatigueSequence;5;;;;;1;;3.", "functionId1");
    expectInvalid("FatigueSequence;5;;;;;1;0", "functionId1");
    expectInvalid("FatigueSequence;5;;;;;1;1;-2.", "repeats");
    expectInvalid("FatigueSequence;5;;;;;2;1;2.;UNITS", "functionId1");
    expectInvalid("FatigueSequence;5;;;;;1;1;;Units;0.", "equivalentUnits");
    oor::testing::requireModelFileRoundTrip(readModelText("FatigueSequence;5;;;;;1;1;;Units;1.;a b", true));
}

TEST_CASE("an FTGEVNT is named by an FTGSEQ and the numbers of both are one space", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "TableFatigueHistory;1;0.;1.;-1.;EndTable\n"
        "FatigueLoad;7;1;2\n"
        "FatigueLoad;13;1;3\n"
        "FatigueEvent;22;2;7;13\n"
        "FatigueSequence;30;;;;;1;22;2.\n"
        "FatigueSequence;31;;;;;2;30;1.;22;3.");
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(visitedBy<FatigueEventTable>(model, 0).targets ==
            std::vector<ReferenceTarget>(2, ReferenceTarget::FatigueLoad));
    // A sequence names events and other sequences with the same target.
    REQUIRE(visitedBy<FatigueSequenceTable>(model, 1).targets ==
            std::vector<ReferenceTarget>(2, ReferenceTarget::FatigueEvent));
    REQUIRE(unresolvedIn(readBulk("FatigueSequence;30;;;;;1;22;2.")) == 1);
    REQUIRE(unresolvedIn(readBulk("FatigueSequence;30;;;;;1;31;2.\nFatigueSequence;31;;;;;1;30;1.")) == 0);

    // The events and the sequences are renumbered together, and every FIDi and FLOADi follows.
    Renumbering::apply(model, RenumberSpace::FatigueLoad, Renumbering::offset(100));
    REQUIRE(cardAt<FatigueEventTable>(model, 0).fatigueLoadIds == Ids{107, 113});
    Renumbering::apply(model, RenumberSpace::FatigueEvent, Renumbering::offset(1000));
    REQUIRE((cardAt<FatigueEventTable>(model, 0).id == 1022 && cardAt<FatigueSequenceTable>(model, 0).id == 1030));
    REQUIRE(cardAt<FatigueSequenceTable>(model, 0).items == std::vector<FatigueSequenceItem>{{1022, 2.0}});
    REQUIRE(
        (cardAt<FatigueSequenceTable>(model, 1).items == std::vector<FatigueSequenceItem>{{1030, 1.0}, {1022, 3.0}}));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // Two cards of one type with a number are the reader's business; an FTGEVNT and an FTGSEQ with one number are the
    // business of the check `compat.fatigue-event-id-repeated`, and are read.
    REQUIRE_THROWS_AS(readBulk("FatigueEvent;5;1;1\nFatigueEvent;5;1;2"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("FatigueEvent;5;1;1\nFatigueSequence;5;;;;;1;1"));
}

// ---------------------------------------------------------------------------------------------------------------------
// FTGDEF
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FTGDEF reads the sections of both formats, the ELSET with or without properties",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "FatigueDefinition;92001;75.;8\n"
        "FatigueDefinition;92002;;;;;;;;ElemSet;2;14;3;15;4\n"
        "FatigueDefinition;92003\n"
        "FatigueDefinition;92004;50.;3;25.;10;200;2;1;ElemSet;4;14;3;15;4;16;;17;5;SpotWeld;2;18;6;19;;SeamWeld;2;20;7;"
        "21;"
        "FilletWeld;ToeWeld;22;;23;LaserWeld;;ExcludedElemSet;8;30;31;32;33;34;35;36;37\n"
        "FatigueDefinition;92005;80.;;;;;;;ElemSet;4;14;;15;;16;;17;;ExcludedElemSet;2;30;31\n"
        "FatigueDefinition;92006;;;;;;;;ExcludedElemSet;1;41");
    REQUIRE((FatigueDefinitionTable::INFO.category == CardCategory::Other &&
             FatigueDefinitionTable::INFO.idSpace == base::IdSpace::PerCardType));
    const FatigueDefinitionCard sampleCard = cardAt<FatigueDefinitionTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.topStressCount == 75.0 && sampleCard.fatiguePropIds == 8 &&
             sampleCard.maxOutputEntities == 100 && sampleCard.hotSpotGate == 0));
    REQUIRE_FALSE((sampleCard.topDamageCount || sampleCard.maxEntities || sampleCard.hotSpotCount));
    const FatigueDefinitionCard elset = cardAt<FatigueDefinitionTable>(model, 1);
    REQUIRE(elset.elemSets == std::vector<FatigueDefinitionPair>{{14, 3}, {15, 4}});
    REQUIRE((cardAt<FatigueDefinitionTable>(model, 2).elemSets.empty() &&
             !cardAt<FatigueDefinitionTable>(model, 2).topStressCount));
    const FatigueDefinitionCard full = cardAt<FatigueDefinitionTable>(model, 3);
    REQUIRE((full.topStressCount == 50.0 && full.fatiguePropIds == 3 && full.topDamageCount == 25.0 &&
             full.maxEntities == 10 && full.maxOutputEntities == 200));
    REQUIRE((full.hotSpotCount == 2 && full.hotSpotGate == 1));
    // Three pairs to a line; the fourth is in the first pair of the next line, and a blank PFTGIDi stays blank.
    REQUIRE(full.elemSets == std::vector<FatigueDefinitionPair>{{14, 3}, {15, 4}, {16, 0}, {17, 5}});
    REQUIRE(full.spotWelds == std::vector<FatigueDefinitionPair>{{18, 6}, {19, 0}});
    REQUIRE(full.seamWelds.size() == 2);
    REQUIRE((full.seamWelds[0] ==
             FatigueDefinitionSeam{20, 7, 21, FatigueDefinitionWeld::Fillet, FatigueDefinitionWeldLocation::Toe}));
    // A blank TYPEi is TOE, and ROOT when the WELDi is LASER; it is stored resolved.
    REQUIRE((full.seamWelds[1] ==
             FatigueDefinitionSeam{22, 0, 23, FatigueDefinitionWeld::Laser, FatigueDefinitionWeldLocation::Root}));
    REQUIRE(full.excludedElemSetIds == Ids{30, 31, 32, 33, 34, 35, 36, 37}); // seven to a line
    // The format of SOL 108 and 111: the ELSIDi are in the places of the pairs, with the properties blank.
    const FatigueDefinitionCard random = cardAt<FatigueDefinitionTable>(model, 4);
    REQUIRE((random.topStressCount == 80.0 &&
             random.elemSets == std::vector<FatigueDefinitionPair>{{14, 0}, {15, 0}, {16, 0}, {17, 0}}));
    REQUIRE(random.excludedElemSetIds == Ids{30, 31});
    REQUIRE(cardAt<FatigueDefinitionTable>(model, 5).excludedElemSetIds == Ids{41});
    requireRoundTrip(model);

    requireIdEdits<FatigueDefinitionTable>(model, 92001, 92002, 92100);

    expectInvalid("FatigueDefinition;0", "id");
    expectInvalid("FatigueDefinition;5;101.", "topStressCount");
    expectInvalid("FatigueDefinition;5;-101.", "topStressCount");
    expectInvalid("FatigueDefinition;5;;-1", "fatiguePropIds");
    expectInvalid("FatigueDefinition;5;;;0.", "topDamageCount");
    expectInvalid("FatigueDefinition;5;;;101.", "topDamageCount");
    expectInvalid("FatigueDefinition;5;;;;;0", "maxOutputEntities");
    expectInvalid("FatigueDefinition;5;;;;;;0", "hotSpotCount");
    expectInvalid("FatigueDefinition;5;;;;;;;-1", "hotSpotGate");
    expectInvalid("FatigueDefinition;5;;;;;;;;ElemSet", "elemSetIds"); // a section has at least one entry
    expectInvalid("FatigueDefinition;5;;;;;;;;ElemSet;1;;3", "elemSetIds");
    expectInvalid("FatigueDefinition;5;;;;;;;;ElemSet;1;0", "elemSetIds");
    expectInvalid("FatigueDefinition;5;;;;;;;;ElemSet;1;10;;ElemSet;1;11", "ElemSet");
    expectInvalid("FatigueDefinition;5;;;;;;;;SpotWeld", "elemSetIds");
    expectInvalid("FatigueDefinition;5;;;;;;;;SeamWeld", "elemSetIds");
    expectInvalid("FatigueDefinition;5;;;;;;;;SeamWeld;1;10;;;BAD", "weldType");
    expectInvalid("FatigueDefinition;5;;;;;;;;SeamWeld;1;10;;;FilletWeld;SIDE", "weldLocation");
    expectInvalid("FatigueDefinition;5;;;;;;;;ExcludedElemSet", "excludedElemSetIds");
    expectInvalid("FatigueDefinition;5;;;;;;;;ExcludedElemSet;1;-3", "excludedElemSetIds");
}

TEST_CASE("an FTGDEF names its properties and its sets, and the references follow their renumbering",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "IdListSet1;14;1;2\n"
        "IdListSet1;15;3\n"
        "IdListSet1;21;4\n"
        "IdListSet1;30;5\n"
        "FatigueProp;3\n"
        "FatigueProp;4\n"
        "FatigueProp;5\n"
        "FatigueProp;7\n"
        "FatigueDefinition;22;;5;;;;;;ElemSet;2;14;3;15;4;SeamWeld;1;15;5;21;;;ExcludedElemSet;1;30");
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<FatigueDefinitionTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"fatiguePropIds", "elemSetIds", "fatiguePropIds", "elemSetIds",
                                                       "fatiguePropIds", "elemSetIds", "fatiguePropIds", "nodeSetIds",
                                                       "excludedElemSetIds"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::FatigueProperty, ReferenceTarget::Set,
                                                            ReferenceTarget::FatigueProperty, ReferenceTarget::Set,
                                                            ReferenceTarget::FatigueProperty, ReferenceTarget::Set,
                                                            ReferenceTarget::FatigueProperty, ReferenceTarget::Set,
                                                            ReferenceTarget::Set});
    REQUIRE(unresolvedIn(readBulk("FatigueDefinition;22;;;;;;;;ElemSet;1;14;3")) ==
            2); // neither the set nor the properties exist

    Renumbering::apply(model, RenumberSpace::FatigueProperty, Renumbering::offset(100));
    const FatigueDefinitionCard card = cardAt<FatigueDefinitionTable>(model, 0);
    REQUIRE((card.fatiguePropIds == 105 && card.elemSets == std::vector<FatigueDefinitionPair>{{14, 103}, {15, 104}}));
    REQUIRE(card.seamWelds[0].fatiguePropId == 105);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    const FatigueDefinitionCard moved = cardAt<FatigueDefinitionTable>(model, 0);
    REQUIRE(moved.elemSets == std::vector<FatigueDefinitionPair>{{1014, 103}, {1015, 104}});
    REQUIRE((moved.seamWelds[0].elemSetId == 1015 && moved.seamWelds[0].nodeSetId == 1021 &&
             moved.excludedElemSetIds == Ids{1030}));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// FTGPARM
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an FTGPARM refuses what its format does not allow", "[model][cards][fatigue]")
{
    expectInvalid("FatigueParams;0", "id");
    expectInvalid("FatigueParams;5;XX", "lifeModel");
    expectInvalid("FatigueParams;5;StressLifeMethod;0.", "factor");
    expectInvalid("FatigueParams;5;StressLifeMethod;;-1", "threadCount");
    expectInvalid("FatigueParams;5;StressLifeMethod;;;6", "logLevel");
    expectInvalid("FatigueParams;5;StressLifeMethod;;;;3", "layer");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;;;;Strain", "strainResults"); // one of the two flags
    expectInvalid("FatigueParams;5;;;;;;Strain", "strainResults");                // the strain is not valid for TYPE SN
    expectInvalid("FatigueParams;5;;;;;;Stress;StandardCorrection", "combination"); // a spot weld word
    expectInvalid("FatigueParams;5;;;;;;Stress;;Simple", "meanStressCorrection");
    expectInvalid("FatigueParams;5;;;;;;Stress;BAD", "combination");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;BAD", "plasticityCorrection");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;BAD", "resultLocation");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;2", "curveInterpolation");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;BAD", "stressRecovery");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;;MAYBE", "surfaceResolved");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;;;361", "planeAngleCount");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;;;0", "planeAngleCount");
    expectInvalid("FatigueParams;5;;;;;;RainflowCounting", "responseKind");
    expectInvalid("FatigueParams;5;;;;;;RainflowCounting;TIME", "responseKind");
    expectInvalid("FatigueParams;5;;;;;;CertaintyOfSurvival;100.", "survivalCertainty");
    expectInvalid("FatigueParams;5;;;;;;CertaintyOfSurvival;0.", "survivalCertainty");
    expectInvalid("FatigueParams;5;;;;;;FactorOfSafety;MAYBE", "safetyFactorOption");
    expectInvalid("FatigueParams;5;;;;;;FactorOfSafety;;-1.", "targetLife");
    expectInvalid("FatigueParams;5;;;;;;FactorOfSafety;;;0.001", "backCalculationAccuracy");
    expectInvalid("FatigueParams;5;;;;;;FactorOfSafety;;;;1.", "maxSafetyFactor");
    expectInvalid("FatigueParams;5;;;;;;FactorOfSafety;;;;;0.6", "minSafetyFactor");
    expectInvalid("FatigueParams;5;;;;;;SpotWeld;;SmithWatsonTopper", "meanStressCorrection");
    expectInvalid("FatigueParams;5;;;;;;SpotWeld;;;5", "planeAngleCount");
    expectInvalid("FatigueParams;5;;;;;;SpotWeld;;;;4", "spotWeldLocation");
    expectInvalid("FatigueParams;5;;;;;;SpotWeld;;;;;2", "middleSheets");
    expectInvalid("FatigueParams;5;;;;;;SpotWeld;;;;;;2", "torsion");
    expectInvalid("FatigueParams;5;;;;;;SeamWeld;SignedVonMises", "combination");
    expectInvalid("FatigueParams;5;;;;;;SeamWeld;;GoodmanCorrection", "meanStressCorrection");
    expectInvalid("FatigueParams;5;;;;;;SeamWeld;;;2", "thicknessCorrection");
    expectInvalid("FatigueParams;5;;;;;;SeamWeld;;;;BAD", "seamWeldLocation");
    expectInvalid("FatigueParams;5;;;;;;SeamWeld;;;;;Strain", "seamWeldResult");
    expectInvalid("FatigueParams;5;;;;;;Multiaxial;BAD", "multiaxialMethod");
    expectInvalid("FatigueParams;5;;;;;;Multiaxial;;x", "nonProportionalityLower");
    expectInvalid("FatigueParams;5;;;;;;NodeAverage;BAD", "averagingMethod");
    expectInvalid("FatigueParams;5;;;;;;NodeAverage;;-2", "output");
    expectInvalid("FatigueParams;5;;;;;;NodeAverage;;;BAD", "surfaceNormal");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;BAD", "spectrumMethod");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;0.", "maxStressRms");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;-1.", "clipLevel");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;;0.", "maxPeak");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;;;0", "stressBins");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;;;;101.", "maxFrequencyFraction");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;;;;;MAYBE", "binLifeMethod");
    expectInvalid("FatigueParams;5;;;;;;Stress;;;;;;;;;Stress", "Stress");
    expectInvalid("FatigueParams;5;;;;;;VibrationFatigue;;;;;;;;VibrationFatigue", "VibrationFatigue");

    // The fields of a card that has been edited are held to the same rules.
    FatigueParamsCard card;
    card.id = 5;
    card.results = FatigueParamsResults{};
    card.results->kind = FatigueParamsResultKind::Strain;
    REQUIRE(std::string_view(FatigueParamsFormat::check(card)->field) == "strainResults");
    card.type = FatigueParamsType::En;
    REQUIRE_FALSE(FatigueParamsFormat::check(card).has_value());
}

TEST_CASE("an FTGPARM names the coordinate system of its NAVG line when OUTPUT is positive", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "RectFrame2;12\nFatigueParams;5;;;;;;NodeAverage;GeometryOption;12\nFatigueParams;6;;;;;;NodeAverage;;-"
        "1\nFatigueParams;7;;;;;;"
        "NodeAverage\nFatigueParams;8");
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<FatigueParamsTable>(model, 0);
    REQUIRE((visited.fields == std::vector<std::string>{"output"} &&
             visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate}));
    for (const std::uint32_t row : {1U, 2U, 3U}) // the element system, the basic system and no line name no system
        REQUIRE(visitedBy<FatigueParamsTable>(model, row).fields.empty());
    REQUIRE(unresolvedIn(readBulk("FatigueParams;5;;;;;;NodeAverage;GeometryOption;12")) == 1);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(100));
    REQUIRE(cardAt<FatigueParamsTable>(model, 0).average->output == 112);
    REQUIRE(cardAt<FatigueParamsTable>(model, 1).average->output == -1);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

// ---------------------------------------------------------------------------------------------------------------------
// TIM2PSD
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TIM2PSD has events, deletions and a channel map in sections", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "TimeToSpectrum;92001;400.;HanningWindow;CommaSeparatedFile;Yes;2;Event;3;92011;3.;1.5;92012;1.5;0.75;92013;3.;"
        "1.5;Delete;1;92011;0."
        "4;1.6;;;;;Map;3;1;4;2;6;3;8\n"
        "TimeToSpectrum;92002;256.;;;;;Event;1;92011;1.\n"
        "TimeToSpectrum;92003;1024.;None;LoadHistoryFile;No;0;Event;1;92011;1.;0.25;Delete;1;92011;0.5;0.8;1.2;1.5;2.;"
        "2.5\n"
        "TimeToSpectrum;92004;128.;;;;;Event;2;92011;1.;;92012;2.;;Map;4;1;1;2;2;3;3;4;4");
    REQUIRE((TimeToSpectrumTable::INFO.category == CardCategory::Other &&
             TimeToSpectrumTable::INFO.idSpace == base::IdSpace::PerCardType));
    const TimeToSpectrumCard sampleCard = cardAt<TimeToSpectrumTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.samplingRate == 400.0 &&
             sampleCard.window == TimeToSpectrumWindow::Hanning && sampleCard.format == TimeToSpectrumFileFormat::Csv));
    REQUIRE((sampleCard.meanStress && sampleCard.skipCount == 2));
    REQUIRE(sampleCard.events ==
            std::vector<TimeToSpectrumEvent>{{92011, 3.0, 1.5}, {92012, 1.5, 0.75}, {92013, 3.0, 1.5}});
    REQUIRE(sampleCard.deletions == std::vector<TimeToSpectrumDeletion>{{92011, {{0.4, 1.6}}}});
    REQUIRE(sampleCard.channels == std::vector<TimeToSpectrumChannel>{{1, 4}, {2, 6}, {3, 8}});
    // The defaults: HANNING, CSV, NO, 0, no overlap.
    const TimeToSpectrumCard bare = cardAt<TimeToSpectrumTable>(model, 1);
    REQUIRE((bare.window == TimeToSpectrumWindow::Hanning && bare.format == TimeToSpectrumFileFormat::Csv &&
             !bare.meanStress && bare.skipCount == 0));
    REQUIRE((bare.events == std::vector<TimeToSpectrumEvent>{{92011, 1.0, 0.0}} && bare.deletions.empty() &&
             bare.channels.empty()));
    const TimeToSpectrumCard fromFile = cardAt<TimeToSpectrumTable>(model, 2);
    REQUIRE((fromFile.window == TimeToSpectrumWindow::None &&
             fromFile.format == TimeToSpectrumFileFormat::LoadHistoryFile && !fromFile.meanStress));
    REQUIRE(fromFile.deletions[0].intervals ==
            std::vector<std::pair<double, double>>{{0.5, 0.8}, {1.2, 1.5}, {2.0, 2.5}});
    REQUIRE(cardAt<TimeToSpectrumTable>(model, 3).channels.size() == 4); // three pairs on a line, then one more
    REQUIRE(cardAt<TimeToSpectrumTable>(model, 3).events.size() == 2);
    requireRoundTrip(model);

    REQUIRE(visitedBy<TimeToSpectrumTable>(model, 0).fields.empty()); // the events and the subcases are plain numbers
    requireIdEdits<TimeToSpectrumTable>(model, 92001, 92002, 92100);

    const std::string events = "TimeToSpectrum;5;100.;;;;;Event;1;1;1.";
    REQUIRE_NOTHROW(readBulk(events));
    expectInvalid("TimeToSpectrum;0;100.;;;;;Event;1;1;1.", "id");
    expectInvalid("TimeToSpectrum;5", "samplingRate");
    expectInvalid("TimeToSpectrum;5;0.;;;;;Event;1;1;1.", "samplingRate");
    expectInvalid("TimeToSpectrum;5;100.;HAMMING;;;;Event;1;1;1.", "window");
    expectInvalid("TimeToSpectrum;5;100.;;TXT;;;Event;1;1;1.", "fileFormat");
    expectInvalid("TimeToSpectrum;5;100.;;;MAYBE;;Event;1;1;1.", "meanStress");
    expectInvalid("TimeToSpectrum;5;100.;;;;-1;Event;1;1;1.", "skipCount");
    expectInvalid("TimeToSpectrum;5;100.", "eventIds");
    expectInvalid("TimeToSpectrum;5;100.;;;;;Event", "eventIds");
    expectInvalid("TimeToSpectrum;5;100.;;;;;Event;1;1;0.", "sliceTimes");
    expectInvalid("TimeToSpectrum;5;100.;;;;;Event;1;1", "sliceTimes");
    expectInvalid("TimeToSpectrum;5;100.;;;;;Event;2;1;1.;;1;2.", "eventIds");
    expectInvalid(events + ";;Event;1;2;1.", "Event");
    expectInvalid(events + ";;Delete;1;2;1.;2.", "eventIds");
    expectInvalid(events + ";;Delete;1;1", "sliceStart");
    expectInvalid(events + ";;Delete;1;1;2.", "sliceEnd");
    expectInvalid(events + ";;Delete;1;1;3.;2.", "sliceStart");
    expectInvalid(events + ";;Delete;1;1;0.;2.", "sliceStart");
    expectInvalid(events + ";;Delete;1;1;1.;2.;;;3.;4.", "sliceTimes");
    expectInvalid(events + ";;Delete;1;1;1.;2.;1.5;3.", "sliceStart");
    expectInvalid(events + ";;Map;1;1", "channels");
    expectInvalid(events + ";;Map;1;0;1", "loadCaseIds");
    expectInvalid(events + ";;Map;2;2;1;1;2", "loadCaseIds");
    expectInvalid(events + ";;Map;2;1;2;2;2", "channels");
}

// ---------------------------------------------------------------------------------------------------------------------
// HDF5DA
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an HDF5DA pairs the modules with the UDNAME of their files", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "ResultFileModules;92001;92016;0;92017;2;92018\n"
        "ResultFileModules;92002;;0;92011\n"
        "ResultFileModules;92003;92012;0;92011;1;92013;2;92013;3;92014;4;92015");
    REQUIRE((ResultFileModulesTable::INFO.category == CardCategory::Other &&
             ResultFileModulesTable::INFO.idSpace == base::IdSpace::PerCardType));
    const ResultFileModulesCard sampleCard = cardAt<ResultFileModulesTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.configuration == 92016));
    REQUIRE(sampleCard.modules == std::vector<ResultFileModulesModule>{{0, 92017}, {2, 92018}});
    REQUIRE((cardAt<ResultFileModulesTable>(model, 1).configuration == 0 &&
             cardAt<ResultFileModulesTable>(model, 1).modules.size() == 1));
    const ResultFileModulesCard many = cardAt<ResultFileModulesTable>(model, 2);
    REQUIRE(many.modules.size() == 5); // four pairs on a line, then one more
    REQUIRE((many.modules[1] == ResultFileModulesModule{1, 92013} &&
             many.modules[2] == ResultFileModulesModule{2, 92013})); // a file may be shared
    requireRoundTrip(model);

    // The configuration and each file are UDNAME cards; the modules are plain numbers.
    const Visited visited = visitedBy<ResultFileModulesTable>(model, 0);
    REQUIRE(visited.fields == std::vector<std::string>{"configuration", "userNameIds", "userNameIds"});
    REQUIRE(visited.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::UserDefinedName));
    Model checked = readBulk(
        "UserFileName;10;a.h5\nUserFileName;11;b.h5\nUserFileName;12;config\nResultFileModules;5;12;0;10;1;11");
    REQUIRE(unresolvedIn(checked) == 0);
    REQUIRE(unresolvedIn(readBulk("ResultFileModules;5;12;0;10;1;11")) == 3);
    Renumbering::apply(checked, RenumberSpace::UserDefinedName, Renumbering::offset(1000));
    const ResultFileModulesCard moved = cardAt<ResultFileModulesTable>(checked, 0);
    REQUIRE(
        (moved.configuration == 1012 && moved.modules == std::vector<ResultFileModulesModule>{{0, 1010}, {1, 1011}}));
    REQUIRE(unresolvedIn(checked) == 0);
    requireIdEdits<ResultFileModulesTable>(model, 92001, 92002, 92100);

    expectInvalid("ResultFileModules;0;;0;1", "id");
    expectInvalid("ResultFileModules;5;-1;0;1", "configuration");
    expectInvalid("ResultFileModules;5", "moduleIds");
    expectInvalid("ResultFileModules;5;;0", "userNameIds");
    expectInvalid("ResultFileModules;5;;;1", "moduleIds");
    expectInvalid("ResultFileModules;5;;-1;1", "moduleIds");
    expectInvalid("ResultFileModules;5;;0;0", "userNameIds");
    expectInvalid("ResultFileModules;5;;0;1;0;2", "moduleIds"); // a module has one file
}

// ---------------------------------------------------------------------------------------------------------------------
// H5OUT and HDF5OUT
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an H5OUT gives the parameters of one module and an HDF5OUT those of every module", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "ModuleResultFileParams;7;InputData;No\n"
        "ModuleResultFileParams;120;RotationalDofs;No\n"
        "ModuleResultFileParams;0;RotationalDofs;No;SeparateModelFile;2;LoadConstraintTables;1;RunInformation;No;"
        "ModalMatrixOutput;Yes;MatrixData;Yes\n"
        "ModuleResultFileParams;3;NEWPARAM;5;other;1.5;word;fast");
    REQUIRE((ModuleResultFileParamsTable::INFO.category == CardCategory::Other &&
             ModuleResultFileParamsTable::INFO.idSpace == base::IdSpace::None));
    const ModuleResultFileParamsCard sampleCard = cardAt<ModuleResultFileParamsTable>(model, 0);
    REQUIRE((sampleCard.moduleId == 7 && sampleCard.parameters.size() == 1 &&
             sampleCard.parameters[0].name == "InputData"));
    REQUIRE((sampleCard.parameters[0].value.isWord() &&
             std::get<std::string>(sampleCard.parameters[0].value.data) == "No"));
    REQUIRE(cardAt<ModuleResultFileParamsTable>(model, 1).moduleId == 120);
    const ModuleResultFileParamsCard many = cardAt<ModuleResultFileParamsTable>(model, 2);
    REQUIRE((many.moduleId == 0 && many.parameters.size() == 6)); // four pairs on a line, then two more
    REQUIRE((many.parameters[1].name == "SeparateModelFile" && many.parameters[1].value.isInteger() &&
             std::get<std::int32_t>(many.parameters[1].value.data) == 2));
    // A name the table does not list is kept with its value as written: an integer, a real or a word.
    const ModuleResultFileParamsCard unknown = cardAt<ModuleResultFileParamsTable>(model, 3);
    REQUIRE((unknown.parameters[0].name == "NEWPARAM" && unknown.parameters[0].value.isInteger()));
    REQUIRE((unknown.parameters[1].name == "other" && unknown.parameters[1].value.isReal() &&
             std::get<double>(unknown.parameters[1].value.data) == 1.5));
    REQUIRE(
        (unknown.parameters[2].value.isWord() && std::get<std::string>(unknown.parameters[2].value.data) == "fast"));
    requireRoundTrip(model);

    REQUIRE(visitedBy<ModuleResultFileParamsTable>(model, 0).fields.empty());

    expectInvalid("ModuleResultFileParams", "moduleId");
    expectInvalid("ModuleResultFileParams;-1;InputData;No", "moduleId");
    expectInvalid("ModuleResultFileParams;1", "parameterNames");
    expectInvalid("ModuleResultFileParams;1;InputData", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;;No", "parameterName");
    expectInvalid("ModuleResultFileParams;1;Precision;16", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;InputData;MAYBE", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;InputData;1", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;CompressionLevel;10", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;CompressionLevel;Yes", "parameterValue");
    expectInvalid("ModuleResultFileParams;1;1ABC;5", "parameterName");
    oor::testing::requireModelFileRoundTrip(readModelText("ModuleResultFileParams;1;TOOLONGNAME;1", true));
}

TEST_CASE("an HDF5OUT lists its parameters and keeps an integer value", "[model][cards][fatigue]")
{
    Model model = readBulk(
        "ResultFileParams;Precision;32;CompressionMethod;Lz4Compression;CompressionLevel;7\n"
        "ResultFileParams;3;Precision;64\n"
        "ResultFileParams;InputData;No;RotationalDofs;No;MatrixData;Yes;SeparateModelFile;-1;NodeElementTables;0;"
        "LoadConstraintTables;0;RunInformation;No;ModalMatrixOutput;Yes");
    REQUIRE((ResultFileParamsTable::INFO.category == CardCategory::Other &&
             ResultFileParamsTable::INFO.idSpace == base::IdSpace::None));
    const ResultFileParamsCard first = cardAt<ResultFileParamsTable>(model, 0);
    REQUIRE((!first.number && first.parameters.size() == 3));
    REQUIRE((first.parameters[0].name == "Precision" && std::get<std::int32_t>(first.parameters[0].value.data) == 32));
    REQUIRE((first.parameters[1].name == "CompressionMethod" &&
             std::get<std::string>(first.parameters[1].value.data) == "Lz4Compression"));
    REQUIRE((first.parameters[2].name == "CompressionLevel" &&
             std::get<std::int32_t>(first.parameters[2].value.data) == 7));
    const ResultFileParamsCard second = cardAt<ResultFileParamsTable>(model, 1);
    REQUIRE((second.number == 3 && second.parameters.size() == 1 && second.parameters[0].name == "Precision"));
    const ResultFileParamsCard many = cardAt<ResultFileParamsTable>(model, 2);
    REQUIRE(many.parameters.size() == 8); // four pairs on a line, then four more
    REQUIRE((many.parameters[3].name == "SeparateModelFile" &&
             std::get<std::int32_t>(many.parameters[3].value.data) == -1));
    REQUIRE(std::get<std::string>(many.parameters[7].value.data) == "Yes");
    requireRoundTrip(model);

    // The catalogue: eleven parameters with their kind, values and default.
    REQUIRE(HDF5_PARAMETER_CATALOGUE.size() == 11);
    const Hdf5ParameterInfo* precision = findHdf5Parameter("Precision");
    REQUIRE((precision != nullptr && precision->kind == Hdf5ParameterKind::Integer && precision->defaultValue == "32"));
    REQUIRE((findHdf5Parameter("CompressionLevel")->kind == Hdf5ParameterKind::IntegerRange &&
             findHdf5Parameter("CompressionLevel")->defaultValue.empty()));
    REQUIRE((findHdf5Parameter("CompressionMethod")->kind == Hdf5ParameterKind::Word &&
             findHdf5Parameter("CompressionMethod")->defaultValue == "Lz4Compression"));
    REQUIRE(findHdf5Parameter("NOPE") == nullptr);

    expectInvalid("ResultFileParams", "parameterName");
    expectInvalid("ResultFileParams;Precision", "parameterValue");
    expectInvalid("ResultFileParams;;32", "parameterName");
    expectInvalid("ResultFileParams;Precision;16", "parameterValue");
    expectInvalid("ResultFileParams;CompressionMethod;ZIP", "parameterValue");
    expectInvalid("ResultFileParams;CompressionMethod;3", "parameterValue");
    expectInvalid("ResultFileParams;CompressionLevel;0", "parameterValue");
    expectInvalid("ResultFileParams;CompressionLevel;10", "parameterValue");
    expectInvalid("ResultFileParams;SeparateModelFile;3", "parameterValue");
    expectInvalid("ResultFileParams;NodeElementTables;-1", "parameterValue");
    expectInvalid("ResultFileParams;LoadConstraintTables;2", "parameterValue");
    expectInvalid("ResultFileParams;MatrixData;1", "parameterValue");
    expectInvalid("ResultFileParams;-3;Precision;32", "parameterName");
    REQUIRE_NOTHROW(readBulk(
        "ResultFileParams;CompressionLevel;9;SeparateModelFile;1;NodeElementTables;1;LoadConstraintTables;-1"));
}

// ---------------------------------------------------------------------------------------------------------------------
// TABLRPC
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABLRPC is a dynamic table that reads a channel of a file, with pairs that filter it",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "TableTimeHistoryFile;92001;;;92011;LoadHistoryFile;6\n"
        "TableTimeHistoryFile;92002;;;92011;DataAcquisitionFile;;;;2.5;17.5;40.;72.5;EndTable\n"
        "TableTimeHistoryFile;92003;;;-92011;;5;;;2.5;17.5;40.;72.5;EndTable\n"
        "TableTimeHistoryFile;92004;Logarithmic;Logarithmic;92011;LoadHistoryFile;2;20.;Yes;0.;5.;Skip;10.;;30.;40.;50."
        ";EndTable\n"
        "TableTimeHistoryFile;92005;;;92012;LoadHistoryFile;1;12.5\n"
        "TableTimeHistoryFile;92006;;;92011;;;;;EndTable\n"
        "TableTimeHistoryFile;92007;;;92011;;;;;1.;EndTable");
    REQUIRE((TableTimeHistoryFileTable::INFO.category == CardCategory::Table &&
             TableTimeHistoryFileTable::INFO.idSpace == base::IdSpace::DynamicTable));
    const TableTimeHistoryFileCard sampleCard = cardAt<TableTimeHistoryFileTable>(model, 0);
    REQUIRE((sampleCard.id == 92001 && sampleCard.xAxisKind == TableTimeHistoryFileAxis::Linear &&
             sampleCard.yAxisKind == TableTimeHistoryFileAxis::Linear && sampleCard.externalFileId == 92011));
    REQUIRE((sampleCard.fileType == TableTimeHistoryFileFileType::LoadHistoryFile && sampleCard.channel == 6 &&
             !sampleCard.totalTime && !sampleCard.punch && sampleCard.windows.empty()));
    const TableTimeHistoryFileCard dac = cardAt<TableTimeHistoryFileTable>(model, 1);
    REQUIRE((dac.fileType == TableTimeHistoryFileFileType::DataAcquisitionFile && dac.channel == 0 &&
             dac.windows.size() == 2));
    REQUIRE((dac.windows[0].start.time == 2.5 && dac.windows[0].end.time == 17.5 && dac.windows[1].start.time == 40.0 &&
             dac.windows[1].end.time == 72.5));
    // A negative UID makes the pairs the parts to leave out; its sign stays on the card.
    REQUIRE((cardAt<TableTimeHistoryFileTable>(model, 2).externalFileId == -92011 &&
             cardAt<TableTimeHistoryFileTable>(model, 2).fileType == TableTimeHistoryFileFileType::LoadHistoryFile));
    const TableTimeHistoryFileCard full = cardAt<TableTimeHistoryFileTable>(model, 3);
    REQUIRE((full.xAxisKind == TableTimeHistoryFileAxis::Log && full.yAxisKind == TableTimeHistoryFileAxis::Log &&
             full.channel == 2 && full.totalTime == 20.0 && full.punch));
    REQUIRE(full.windows.size() == 4);
    REQUIRE((full.windows[1].start.skip && !full.windows[1].start.time && full.windows[1].end.time == 10.0));
    REQUIRE((!full.windows[2].start.skip && !full.windows[2].start.time && full.windows[2].end.time == 30.0));
    REQUIRE((full.windows[3].start.time == 40.0 && full.windows[3].end.time == 50.0));
    REQUIRE((cardAt<TableTimeHistoryFileTable>(model, 4).totalTime == 12.5 &&
             cardAt<TableTimeHistoryFileTable>(model, 4).windows.empty()));
    REQUIRE(cardAt<TableTimeHistoryFileTable>(model, 5).windows.empty()); // ENDT alone is no pair
    // ENDT in the field of the end of a pair: the pair has a start and no end.
    REQUIRE(cardAt<TableTimeHistoryFileTable>(model, 6).windows.size() == 1);
    REQUIRE((cardAt<TableTimeHistoryFileTable>(model, 6).windows[0].start.time == 1.0 &&
             !cardAt<TableTimeHistoryFileTable>(model, 6).windows[0].end.time));
    requireRoundTrip(model);

    // It is a table of the dynamic space: a TLOAD1 finds it as it finds a TABLED1, and the numbers are shared.
    const std::string load = "TransientTableLoad1;1;2;;;;7\n";
    const std::size_t withHistoryFile =
        unresolvedIn(readBulk(load + "UserFileName;3300;f.rsp\nTableTimeHistoryFile;7;;;3300"));
    REQUIRE(withHistoryFile == unresolvedIn(readBulk(load + "TableDynamic1;7;;;0.;1.;1.;2.;EndTable")));
    REQUIRE(unresolvedIn(readBulk(load + "TableTimeHistoryFile;7;;;3300")) ==
            withHistoryFile + 1); // and the UDNAME 3300 is missing
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;5;;;0.;1.;1.;2.;EndTable\nTableTimeHistoryFile;5;;;3300"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableTimeHistoryFile;5;;;3300\nTableUserRoutine;5;grp"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("TableDynamic1;6;;;0.;1.;1.;2.;EndTable\nTableTimeHistoryFile;5;;;3300"));

    // The UID is a UDNAME, by its absolute value, and follows its renumbering with its sign.
    Model named = readBulk("UserFileName;3300;f.rsp\nTableTimeHistoryFile;7;;;3300\nTableTimeHistoryFile;8;;;-3300");
    REQUIRE(unresolvedIn(named) == 0);
    const Visited visited = visitedBy<TableTimeHistoryFileTable>(named, 1);
    REQUIRE((visited.fields == std::vector<std::string>{"externalFileId"} &&
             visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::UserDefinedName}));
    Renumbering::apply(named, RenumberSpace::UserDefinedName, Renumbering::offset(1000));
    REQUIRE((cardAt<TableTimeHistoryFileTable>(named, 0).externalFileId == 4300 &&
             cardAt<TableTimeHistoryFileTable>(named, 1).externalFileId == -4300));
    REQUIRE(unresolvedIn(named) == 0);
    Renumbering::apply(named, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<TableTimeHistoryFileTable>(named, 0).id == 107 &&
             cardAt<TableTimeHistoryFileTable>(named, 1).id == 108));

    const auto where = model.locate(base::IdSpace::DynamicTable, 92002);
    REQUIRE(where.has_value());
    model.setId(*where, 92100);
    REQUIRE(cardAt<TableTimeHistoryFileTable>(model, where->row).id == 92100);
    REQUIRE_THROWS_AS(model.setId(*model.locate(base::IdSpace::DynamicTable, 92003), 92100), base::DuplicateIdError);

    expectInvalid("TableTimeHistoryFile;0;;;5", "id");
    expectInvalid("TableTimeHistoryFile;5", "externalFileId");
    expectInvalid("TableTimeHistoryFile;5;;;0", "externalFileId");
    expectInvalid("TableTimeHistoryFile;5;SQUARE;;5", "xAxisKind");
    expectInvalid("TableTimeHistoryFile;5;;CUBIC;5", "yAxisKind");
    expectInvalid("TableTimeHistoryFile;5;;;5;TXT", "fileType");
    expectInvalid("TableTimeHistoryFile;5;;;5;;-1", "channel");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;0.", "totalTime");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;MAYBE", "punch");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;0.;1.", "tableEnd"); // the pairs end with ENDT
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;0.;1.;EndTable;2.", "tableEnd");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;0.;1.;EndTable;;;;;;3.;4.", "tableEnd");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;0.;A;EndTable", "windowEnd");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;A;1.;EndTable", "windowStart");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;3.;2.;EndTable", "windowStart");
    expectInvalid("TableTimeHistoryFile;5;;;5;;;;;0.;1.;2.;3.;0.5;4.;EndTable", "windowStart");
    REQUIRE_NOTHROW(
        readBulk("TableTimeHistoryFile;5;;;5;;;;;5.;6.;Skip;1.;7.;8.;EndTable")); // a pair that is skipped has no order
}

// ---------------------------------------------------------------------------------------------------------------------
// MATDIGI
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MATDIGI names the UDNAME of its micromechanics file and follows its renumbering",
          "[model][cards][fatigue]")
{
    Model model = readBulk(
        "UserFileName;10;micro.dat\nMicromechanicsCompositeMat;6201;10;3.E+3\nMicromechanicsCompositeMat;6202;10");
    REQUIRE(unresolvedIn(model) == 0);
    const Visited visited = visitedBy<MicromechanicsCompositeMatTable>(model, 0);
    REQUIRE((visited.fields == std::vector<std::string>{"userDefinedNameId"} &&
             visited.targets == std::vector<ReferenceTarget>{ReferenceTarget::UserDefinedName}));
    REQUIRE(unresolvedIn(readBulk("MicromechanicsCompositeMat;6202;11")) == 1);
    Renumbering::apply(model, RenumberSpace::UserDefinedName, Renumbering::offset(1000));
    REQUIRE((cardAt<MicromechanicsCompositeMatTable>(model, 0).userDefinedNameId == 1010 &&
             cardAt<MicromechanicsCompositeMatTable>(model, 1).userDefinedNameId == 1010));
    REQUIRE(cardAt<UserFileNameTable>(model, 0).id == 1010);
    REQUIRE(unresolvedIn(model) == 0);
    // The material numbers are not touched by it.
    REQUIRE((cardAt<MicromechanicsCompositeMatTable>(model, 0).id == 6201 &&
             cardAt<MicromechanicsCompositeMatTable>(model, 1).id == 6202));
    requireRoundTrip(model);
}
