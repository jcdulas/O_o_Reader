#include "oor/model/cards/solution_control/fatigueParams/FatigueParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 10> SECTION_KEYWORDS{
    "Stress",   "Strain",   "RainflowCounting", "CertaintyOfSurvival", "FactorOfSafety",
    "SpotWeld", "SeamWeld", "Multiaxial",       "NodeAverage",         "VibrationFatigue"};

// The static defaults of the lines that the format reference gives.
constexpr int DEFAULT_ANGLES = 36;
constexpr double DEFAULT_CERTAINTY = 50.0;
constexpr double DEFAULT_ACCURACY = 1.0;
constexpr double DEFAULT_MAXIMUM_FACTOR = 5.0;
constexpr double DEFAULT_MINIMUM_FACTOR = 0.2;
constexpr int DEFAULT_SPOT_ANGLES = 18;
constexpr double DEFAULT_MAXIMUM_STRESS = 10.0;
constexpr double DEFAULT_MAXIMUM_FREQUENCY = 99.9;
constexpr int SINES_BINS = 1280; // the default STRBINS of the ATYPE SINES, 32 for the others
constexpr int OTHER_BINS = 32;

// The RECOVER of a blank field: the corners for LOC NODA or NODE, the centre for LOC ELEM.
FatigueParamsRecovery defaultRecovery(FatigueParamsLocation location) noexcept
{
    return location == FatigueParamsLocation::Elem ? FatigueParamsRecovery::Center : FatigueParamsRecovery::Corner;
}

// A YES or NO field; blank is NO.
bool readYesNo(FieldReader& reader, int field, const char* name)
{
    const std::string_view word = reader.text(field);
    if (word.empty() || (word == "No"))
        return false;
    if ((word == "Yes"))
        return true;
    reader.error("card.bad-value", std::string(name) + " must be YES or NO, not \"" + std::string(word) + "\"", name);
    return false;
}

// A line of one line only: the lines below the keyword line are blank.
void requireOneLine(FieldReader& reader, const LineSection& section)
{
    for (int line = section.firstLine + 1; line <= section.lastLine; ++line)
        requireBlankLineFields(reader, line, 2, 9, section.keyword);
}

// A number the format does not describe: an integer or a real as written, blank when the field is.
std::optional<FatigueParamsNumber> readNumber(FieldReader& reader, int field, const char* name)
{
    return reader.integerOrReal(field, name);
}

void writeNumber(FieldWriter& writer, int field, const std::optional<FatigueParamsNumber>& number)
{
    if (!number)
        return;
    if (std::holds_alternative<std::int32_t>(*number))
        writer.integer(field, std::get<std::int32_t>(*number));
    else
        writer.real(field, std::get<double>(*number));
}

// ---------------------------------------------------------------------------------------------------------------------
// Reading the lines
// ---------------------------------------------------------------------------------------------------------------------

FatigueParamsResults readResults(FieldReader& reader, const LineSection& section)
{
    FatigueParamsResults results;
    results.kind = section.keyword == "Strain" ? FatigueParamsResultKind::Strain : FatigueParamsResultKind::Stress;
    const int line = section.firstLine;
    results.combination = readKeyword(reader, fatigueField(line, 3), "combination", FATIGUE_PARAMS_COMBINATION_WORDS,
                                      FatigueParamsCombination::AbsMaxPr);
    results.correction = readKeyword(reader, fatigueField(line, 4), "meanStressCorrection",
                                     FATIGUE_PARAMS_CORRECTION_WORDS, FatigueParamsCorrection::None);
    results.plasticity = readKeyword(reader, fatigueField(line, 5), "plasticityCorrection",
                                     FATIGUE_PARAMS_PLASTICITY_WORDS, FatigueParamsPlasticity::Neuber);
    results.location = readKeyword(reader, fatigueField(line, 6), "resultLocation", FATIGUE_PARAMS_LOCATION_WORDS,
                                   FatigueParamsLocation::Noda);
    results.interpolation = reader.integer(fatigueField(line, 7), "curveInterpolation", 0);
    results.recovery = readKeyword(reader, fatigueField(line, 8), "stressRecovery", FATIGUE_PARAMS_RECOVERY_WORDS,
                                   defaultRecovery(results.location));
    results.surfaceResolved = readYesNo(reader, fatigueField(line, 9), "surfaceResolved");
    // NANGLE has the line below the keyword, in its field 3.
    if (section.lastLine > line)
    {
        results.angles = reader.integer(fatigueField(line + 1, 3), "planeAngleCount", DEFAULT_ANGLES);
        requireBlankLineFields(reader, line + 1, 4, 9, section.keyword);
        for (int below = line + 2; below <= section.lastLine; ++below)
            requireBlankLineFields(reader, below, 2, 9, section.keyword);
    }
    return results;
}

void readRainflow(FieldReader& reader, const LineSection& section, FatigueParamsCard& card)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    if (reader.isBlank(fatigueField(line, 3)))
        reader.error("card.missing-field", "RAINFLOW needs its " + std::string("responseKind") + ": LOAD",
                     "responseKind");
    else if (!(reader.text(fatigueField(line, 3)) == FATIGUE_PARAMS_RAINFLOW_TYPE))
        reader.error("card.bad-value",
                     std::string("responseKind") + " must be LOAD, not \"" +
                         std::string(reader.text(fatigueField(line, 3))) + "\"",
                     "responseKind");
    requireBlankLineFields(reader, line, 4, 9, "RainflowCounting");
    card.rainflow = true;
}

void readCertainty(FieldReader& reader, const LineSection& section, FatigueParamsCard& card)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    card.certainty = reader.real(fatigueField(line, 3), "survivalCertainty", DEFAULT_CERTAINTY);
    requireBlankLineFields(reader, line, 4, 9, "CertaintyOfSurvival");
}

FatigueParamsSafety readSafety(FieldReader& reader, const LineSection& section)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    FatigueParamsSafety safety;
    safety.option = readKeyword(reader, fatigueField(line, 3), "safetyFactorOption", FATIGUE_PARAMS_SAFETY_OPTION_WORDS,
                                FatigueParamsSafetyOption::Life);
    safety.life = reader.optionalReal(fatigueField(line, 4), "targetLife");
    safety.accuracy = reader.real(fatigueField(line, 5), "backCalculationAccuracy", DEFAULT_ACCURACY);
    safety.maximumFactor = reader.real(fatigueField(line, 6), "maxSafetyFactor", DEFAULT_MAXIMUM_FACTOR);
    safety.minimumFactor = reader.real(fatigueField(line, 7), "minSafetyFactor", DEFAULT_MINIMUM_FACTOR);
    requireBlankLineFields(reader, line, 8, 9, "FactorOfSafety");
    return safety;
}

FatigueParamsSpotWeld readSpotWeld(FieldReader& reader, const LineSection& section)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    FatigueParamsSpotWeld weld;
    weld.combination = readKeyword(reader, fatigueField(line, 3), "combination", FATIGUE_PARAMS_COMBINATION_WORDS,
                                   FatigueParamsCombination::Standard);
    weld.correction = readKeyword(reader, fatigueField(line, 4), "meanStressCorrection",
                                  FATIGUE_PARAMS_CORRECTION_WORDS, FatigueParamsCorrection::None);
    weld.angles = reader.integer(fatigueField(line, 5), "planeAngleCount", DEFAULT_SPOT_ANGLES);
    weld.location = reader.integer(fatigueField(line, 6), "spotWeldLocation", 0);
    weld.middleSheets = reader.integer(fatigueField(line, 7), "middleSheets", 1);
    weld.torsion = reader.integer(fatigueField(line, 8), "torsion", 0);
    requireBlankLineFields(reader, line, 9, 9, "SpotWeld");
    return weld;
}

FatigueParamsSeamWeld readSeamWeld(FieldReader& reader, const LineSection& section)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    FatigueParamsSeamWeld weld;
    weld.combination = readKeyword(reader, fatigueField(line, 3), "combination", FATIGUE_PARAMS_COMBINATION_WORDS,
                                   FatigueParamsCombination::AbsMaxPr);
    weld.correction = readKeyword(reader, fatigueField(line, 4), "meanStressCorrection",
                                  FATIGUE_PARAMS_CORRECTION_WORDS, FatigueParamsCorrection::None);
    weld.thickness = reader.integer(fatigueField(line, 5), "thicknessCorrection", 0);
    weld.location = readKeyword(reader, fatigueField(line, 6), "seamWeldLocation", FATIGUE_PARAMS_SEAM_LOCATION_WORDS,
                                FatigueParamsSeamLocation::Node);
    // RESENT has the one word STRESS: it is checked and not kept.
    if (!reader.isBlank(fatigueField(line, 7)) && !(reader.text(fatigueField(line, 7)) == "Stress"))
        reader.error("card.bad-value",
                     std::string("seamWeldResult") + " must be Stress, not \"" +
                         std::string(reader.text(fatigueField(line, 7))) + "\"",
                     "seamWeldResult");
    requireBlankLineFields(reader, line, 8, 9, "SeamWeld");
    return weld;
}

FatigueParamsMulti readMulti(FieldReader& reader, const LineSection& section)
{
    const int line = section.firstLine;
    FatigueParamsMulti multi;
    multi.method = readOptionalKeyword<FatigueParamsMultiMethod>(reader, fatigueField(line, 3), "multiaxialMethod",
                                                                 FATIGUE_PARAMS_MULTI_METHOD_WORDS);
    multi.nonProportionalityLower = readNumber(reader, fatigueField(line, 4), "nonProportionalityLower");
    multi.nonProportionalityUpper = readNumber(reader, fatigueField(line, 5), "nonProportionalityUpper");
    multi.biaxialLower = readNumber(reader, fatigueField(line, 6), "biaxialityLower");
    multi.biaxialMiddle = readNumber(reader, fatigueField(line, 7), "biaxialityMiddle");
    multi.biaxialUpper = readNumber(reader, fatigueField(line, 8), "biaxialityUpper");
    multi.zero = readNumber(reader, fatigueField(line, 9), "zeroThreshold");
    // GATE has the line below the keyword, in its field 3.
    if (section.lastLine > line)
    {
        multi.gate = readNumber(reader, fatigueField(line + 1, 3), "gateThreshold");
        requireBlankLineFields(reader, line + 1, 4, 9, "Multiaxial");
        for (int below = line + 2; below <= section.lastLine; ++below)
            requireBlankLineFields(reader, below, 2, 9, "Multiaxial");
    }
    return multi;
}

FatigueParamsAverage readAverage(FieldReader& reader, const LineSection& section)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    FatigueParamsAverage average;
    average.method = readKeyword(reader, fatigueField(line, 3), "averagingMethod", FATIGUE_PARAMS_AVERAGING_WORDS,
                                 FatigueParamsAveraging::Topological);
    average.output = reader.integer(fatigueField(line, 4), "output", 0);
    average.normal = readKeyword(reader, fatigueField(line, 5), "surfaceNormal", FATIGUE_PARAMS_NORMAL_WORDS,
                                 FatigueParamsNormal::R);
    requireBlankLineFields(reader, line, 6, 9, "NodeAverage");
    return average;
}

FatigueParamsVibration readVibration(FieldReader& reader, const LineSection& section)
{
    requireOneLine(reader, section);
    const int line = section.firstLine;
    FatigueParamsVibration vibration;
    vibration.method = readKeyword(reader, fatigueField(line, 3), "spectrumMethod", FATIGUE_PARAMS_SPECTRUM_WORDS,
                                   FatigueParamsSpectrumMethod::Dirlik);
    vibration.maximumStress = reader.real(fatigueField(line, 4), "maxStressRms", DEFAULT_MAXIMUM_STRESS);
    // The defaults that depend on the fields before them are resolved here.
    vibration.clipLevel = reader.real(fatigueField(line, 5), "clipLevel", vibration.maximumStress);
    vibration.maximumPeak = reader.real(fatigueField(line, 6), "maxPeak", vibration.clipLevel / 2.0);
    vibration.bins = reader.integer(fatigueField(line, 7), "stressBins",
                                    vibration.method == FatigueParamsSpectrumMethod::Sines ? SINES_BINS : OTHER_BINS);
    vibration.maximumFrequency = reader.real(fatigueField(line, 8), "maxFrequencyFraction", DEFAULT_MAXIMUM_FREQUENCY);
    vibration.lifeMethod = readKeyword(reader, fatigueField(line, 9), "binLifeMethod", FATIGUE_PARAMS_LIFE_METHOD_WORDS,
                                       FatigueParamsLifeMethod::Avg);
    return vibration;
}

void readSections(FieldReader& reader, FatigueParamsCard& card)
{
    bool seen[SECTION_KEYWORDS.size()] = {};
    for (const LineSection& section : cutLineSections(reader, SECTION_KEYWORDS))
    {
        if (reader.failed())
            return;
        std::size_t which = 0;
        while (SECTION_KEYWORDS[which] != section.keyword)
            ++which;
        // STRESS and STRAIN are the two flags of one line: the card has one of them.
        const std::size_t slot = which == 1 ? 0 : which;
        if (seen[slot])
        {
            reader.error("card.bad-value",
                         which <= 1 ? std::string("an " + std::string(reader.cardName()) + " has one Stress or " +
                                                  std::string("Strain") + " section")
                                    : "an " + std::string(reader.cardName()) + " has one " + section.keyword + " line",
                         which == 1 ? "strainResults" : std::string(section.keyword).c_str());
            return;
        }
        seen[slot] = true;
        switch (which)
        {
        case 0:
        case 1:
            card.results = readResults(reader, section);
            break;
        case 2:
            readRainflow(reader, section, card);
            break;
        case 3:
            readCertainty(reader, section, card);
            break;
        case 4:
            card.safety = readSafety(reader, section);
            break;
        case 5:
            card.spotWeld = readSpotWeld(reader, section);
            break;
        case 6:
            card.seamWeld = readSeamWeld(reader, section);
            break;
        case 7:
            card.multi = readMulti(reader, section);
            break;
        case 8:
            card.average = readAverage(reader, section);
            break;
        default:
            card.vibration = readVibration(reader, section);
            break;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Writing the lines: each starts at `line` and returns the line after it
// ---------------------------------------------------------------------------------------------------------------------

int writeResults(FieldWriter& writer, int line, const FatigueParamsResults& results)
{
    writer.text(fatigueField(line, 2), keywordText(FATIGUE_PARAMS_RESULT_WORDS, results.kind));
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_COMBINATION_WORDS, results.combination,
                              FatigueParamsCombination::AbsMaxPr);
    writeKeywordUnlessDefault(writer, fatigueField(line, 4), FATIGUE_PARAMS_CORRECTION_WORDS, results.correction,
                              FatigueParamsCorrection::None);
    writeKeywordUnlessDefault(writer, fatigueField(line, 5), FATIGUE_PARAMS_PLASTICITY_WORDS, results.plasticity,
                              FatigueParamsPlasticity::Neuber);
    writeKeywordUnlessDefault(writer, fatigueField(line, 6), FATIGUE_PARAMS_LOCATION_WORDS, results.location,
                              FatigueParamsLocation::Noda);
    writer.integerUnlessDefault(fatigueField(line, 7), results.interpolation, 0);
    // RECOVER: its default depends on LOC, so it is always written.
    writeKeyword(writer, fatigueField(line, 8), FATIGUE_PARAMS_RECOVERY_WORDS, results.recovery);
    writer.textUnlessDefault(fatigueField(line, 9), results.surfaceResolved ? "Yes" : "No", "No");
    int next = line + 1;
    if (!writer.omitsDefaults() || results.angles != DEFAULT_ANGLES)
        writer.integer(fatigueField(next++, 3), results.angles);
    return next;
}

int writeSafety(FieldWriter& writer, int line, const FatigueParamsSafety& safety)
{
    writer.text(fatigueField(line, 2), "FactorOfSafety");
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_SAFETY_OPTION_WORDS, safety.option,
                              FatigueParamsSafetyOption::Life);
    if (safety.life)
        writer.real(fatigueField(line, 4), *safety.life);
    writer.realUnlessDefault(fatigueField(line, 5), safety.accuracy, DEFAULT_ACCURACY);
    writer.realUnlessDefault(fatigueField(line, 6), safety.maximumFactor, DEFAULT_MAXIMUM_FACTOR);
    writer.realUnlessDefault(fatigueField(line, 7), safety.minimumFactor, DEFAULT_MINIMUM_FACTOR);
    return line + 1;
}

int writeSpotWeld(FieldWriter& writer, int line, const FatigueParamsSpotWeld& weld)
{
    writer.text(fatigueField(line, 2), "SpotWeld");
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_COMBINATION_WORDS, weld.combination,
                              FatigueParamsCombination::Standard);
    writeKeywordUnlessDefault(writer, fatigueField(line, 4), FATIGUE_PARAMS_CORRECTION_WORDS, weld.correction,
                              FatigueParamsCorrection::None);
    writer.integerUnlessDefault(fatigueField(line, 5), weld.angles, DEFAULT_SPOT_ANGLES);
    writer.integerUnlessDefault(fatigueField(line, 6), weld.location, 0);
    writer.integerUnlessDefault(fatigueField(line, 7), weld.middleSheets, 1);
    writer.integerUnlessDefault(fatigueField(line, 8), weld.torsion, 0);
    return line + 1;
}

int writeSeamWeld(FieldWriter& writer, int line, const FatigueParamsSeamWeld& weld)
{
    writer.text(fatigueField(line, 2), "SeamWeld");
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_COMBINATION_WORDS, weld.combination,
                              FatigueParamsCombination::AbsMaxPr);
    writeKeywordUnlessDefault(writer, fatigueField(line, 4), FATIGUE_PARAMS_CORRECTION_WORDS, weld.correction,
                              FatigueParamsCorrection::None);
    writer.integerUnlessDefault(fatigueField(line, 5), weld.thickness, 0);
    writeKeywordUnlessDefault(writer, fatigueField(line, 6), FATIGUE_PARAMS_SEAM_LOCATION_WORDS, weld.location,
                              FatigueParamsSeamLocation::Node);
    return line + 1;
}

int writeMulti(FieldWriter& writer, int line, const FatigueParamsMulti& multi)
{
    writer.text(fatigueField(line, 2), "Multiaxial");
    writeOptionalKeyword(writer, fatigueField(line, 3), FATIGUE_PARAMS_MULTI_METHOD_WORDS, multi.method);
    writeNumber(writer, fatigueField(line, 4), multi.nonProportionalityLower);
    writeNumber(writer, fatigueField(line, 5), multi.nonProportionalityUpper);
    writeNumber(writer, fatigueField(line, 6), multi.biaxialLower);
    writeNumber(writer, fatigueField(line, 7), multi.biaxialMiddle);
    writeNumber(writer, fatigueField(line, 8), multi.biaxialUpper);
    writeNumber(writer, fatigueField(line, 9), multi.zero);
    int next = line + 1;
    if (multi.gate)
        writeNumber(writer, fatigueField(next++, 3), multi.gate);
    return next;
}

int writeAverage(FieldWriter& writer, int line, const FatigueParamsAverage& average)
{
    writer.text(fatigueField(line, 2), "NodeAverage");
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_AVERAGING_WORDS, average.method,
                              FatigueParamsAveraging::Topological);
    writer.integerUnlessDefault(fatigueField(line, 4), average.output, 0);
    writeKeywordUnlessDefault(writer, fatigueField(line, 5), FATIGUE_PARAMS_NORMAL_WORDS, average.normal,
                              FatigueParamsNormal::R);
    return line + 1;
}

int writeVibration(FieldWriter& writer, int line, const FatigueParamsVibration& vibration)
{
    writer.text(fatigueField(line, 2), "VibrationFatigue");
    writeKeywordUnlessDefault(writer, fatigueField(line, 3), FATIGUE_PARAMS_SPECTRUM_WORDS, vibration.method,
                              FatigueParamsSpectrumMethod::Dirlik);
    writer.realUnlessDefault(fatigueField(line, 4), vibration.maximumStress, DEFAULT_MAXIMUM_STRESS);
    // CLIPLVL, MAXPEAK and STRBINS have defaults that depend on other fields: they are always written.
    writer.real(fatigueField(line, 5), vibration.clipLevel);
    writer.real(fatigueField(line, 6), vibration.maximumPeak);
    writer.integer(fatigueField(line, 7), vibration.bins);
    writer.realUnlessDefault(fatigueField(line, 8), vibration.maximumFrequency, DEFAULT_MAXIMUM_FREQUENCY);
    writeKeywordUnlessDefault(writer, fatigueField(line, 9), FATIGUE_PARAMS_LIFE_METHOD_WORDS, vibration.lifeMethod,
                              FatigueParamsLifeMethod::Avg);
    return line + 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// Checking the lines
// ---------------------------------------------------------------------------------------------------------------------

std::optional<CardProblem> checkInteger(int value, const char* field, int low, int high)
{
    if (value < low || value > high)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be from " + std::to_string(low) + " to " + std::to_string(high),
            field};
    return std::nullopt;
}

std::optional<CardProblem> checkNumber(const std::optional<FatigueParamsNumber>& number, const char* field)
{
    if (number && std::holds_alternative<double>(*number) && !std::isfinite(std::get<double>(*number)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", field};
    return std::nullopt;
}

std::optional<CardProblem> checkResults(const FatigueParamsResults& results, FatigueParamsType type)
{
    if (results.kind == FatigueParamsResultKind::Strain && type == FatigueParamsType::Sn)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " line is not valid for " +
                               CardProblem::fieldNamed("lifeModel") + " StressLife: use Stress, or " +
                               CardProblem::fieldNamed("lifeModel") + " EN",
                           "strainResults"};
    if (results.combination == FatigueParamsCombination::Standard)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " STNDRD is for the SPOTW line (spot welds only)",
                           "combination"};
    if (results.correction == FatigueParamsCorrection::Simple)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " SIMPLE is for the SPOTW line (spot welds only)",
                           "meanStressCorrection"};
    if (const auto problem = checkInteger(results.interpolation, "curveInterpolation", 0, 1))
        return problem;
    return checkInteger(results.angles, "planeAngleCount", 1, 360);
}

std::optional<CardProblem> checkSafety(const FatigueParamsSafety& safety)
{
    if (const auto problem = checkOptionalReal(safety.life, "targetLife", RealRule::Positive))
        return problem;
    if (const auto problem = checkRealRange(safety.accuracy, "backCalculationAccuracy", 0.01, 100.0))
        return problem;
    if (const auto problem = checkRealRange(safety.maximumFactor, "maxSafetyFactor", 2.0, 5.0e6))
        return problem;
    return checkRealRange(safety.minimumFactor, "minSafetyFactor", 0.0, 0.5);
}

std::optional<CardProblem> checkSpotWeld(const FatigueParamsSpotWeld& weld)
{
    if (weld.correction != FatigueParamsCorrection::None && weld.correction != FatigueParamsCorrection::Fkm &&
        weld.correction != FatigueParamsCorrection::Simple)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " of the SPOTW line must be NONE, FKM or SIMPLE",
                           "meanStressCorrection"};
    if (const auto problem = checkInteger(weld.angles, "planeAngleCount", 10, 360))
        return problem;
    if (const auto problem = checkInteger(weld.location, "spotWeldLocation", 0, 3))
        return problem;
    if (const auto problem = checkInteger(weld.middleSheets, "middleSheets", 0, 1))
        return problem;
    return checkInteger(weld.torsion, "torsion", 0, 1);
}

std::optional<CardProblem> checkSeamWeld(const FatigueParamsSeamWeld& weld)
{
    if (weld.combination != FatigueParamsCombination::AbsMaxPr &&
        weld.combination != FatigueParamsCombination::Critical)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " of the SEAMW line must be ABSMAXPR or CRITICAL",
                           "combination"};
    if (weld.correction != FatigueParamsCorrection::None && weld.correction != FatigueParamsCorrection::Fkm)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " of the SEAMW line must be NONE or FKM",
                           "meanStressCorrection"};
    return checkInteger(weld.thickness, "thicknessCorrection", 0, 1);
}

std::optional<CardProblem> checkMulti(const FatigueParamsMulti& multi)
{
    for (const auto& [number, field] : {std::pair{&multi.nonProportionalityLower, "nonProportionalityLower"},
                                        {&multi.nonProportionalityUpper, "nonProportionalityUpper"},
                                        {&multi.biaxialLower, "biaxialityLower"},
                                        {&multi.biaxialMiddle, "biaxialityMiddle"},
                                        {&multi.biaxialUpper, "biaxialityUpper"},
                                        {&multi.zero, "zeroThreshold"},
                                        {&multi.gate, "gateThreshold"}})
    {
        if (const auto problem = checkNumber(*number, field))
            return problem;
    }
    return std::nullopt;
}

std::optional<CardProblem> checkVibration(const FatigueParamsVibration& vibration)
{
    if (const auto problem = checkReal(vibration.maximumStress, "maxStressRms", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(vibration.clipLevel, "clipLevel", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(vibration.maximumPeak, "maxPeak", RealRule::Positive))
        return problem;
    if (vibration.bins <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "stressBins"};
    return checkRealRange(vibration.maximumFrequency, "maxFrequencyFraction", 0.0, 100.0);
}

} // namespace

std::optional<FatigueParamsCard> FatigueParamsFormat::read(FieldReader& reader)
{
    FatigueParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "lifeModel", FATIGUE_PARAMS_TYPE_WORDS, FatigueParamsType::Sn);
    card.factor = reader.real(3, "factor", 1.0);
    card.threads = reader.optionalInteger(4, "threadCount");
    card.logLevel = reader.integer(5, "logLevel", 0);
    card.layer = reader.integer(6, "layer", 0);
    requireBlankLineFields(reader, 0, 8, 9, "first");
    readSections(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueParamsFormat::check(const FatigueParamsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.factor, "factor", RealRule::Positive))
        return problem;
    if (card.threads && *card.threads < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "threadCount"};
    if (const auto problem = checkInteger(card.logLevel, "logLevel", 0, 5))
        return problem;
    if (const auto problem = checkInteger(card.layer, "layer", 0, 2))
        return problem;
    if (card.results)
    {
        if (const auto problem = checkResults(*card.results, card.type))
            return problem;
    }
    if (card.certainty)
    {
        if (const auto problem = checkRealRange(*card.certainty, "survivalCertainty", 0.1, 99.9))
            return problem;
    }
    if (card.safety)
    {
        if (const auto problem = checkSafety(*card.safety))
            return problem;
    }
    if (card.spotWeld)
    {
        if (const auto problem = checkSpotWeld(*card.spotWeld))
            return problem;
    }
    if (card.seamWeld)
    {
        if (const auto problem = checkSeamWeld(*card.seamWeld))
            return problem;
    }
    if (card.multi)
    {
        if (const auto problem = checkMulti(*card.multi))
            return problem;
    }
    if (card.average && card.average->output < -1)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be -1, 0 or the number of a coordinate system",
                           "output"};
    if (card.vibration)
    {
        if (const auto problem = checkVibration(*card.vibration))
            return problem;
    }
    return std::nullopt;
}

void FatigueParamsFormat::write(const FatigueParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, FATIGUE_PARAMS_TYPE_WORDS, card.type, FatigueParamsType::Sn);
    writer.realUnlessDefault(3, card.factor, 1.0);
    if (card.threads)
        writer.integer(4, *card.threads);
    writer.integerUnlessDefault(5, card.logLevel, 0);
    writer.integerUnlessDefault(6, card.layer, 0);
    int line = 1;
    if (card.results)
        line = writeResults(writer, line, *card.results);
    if (card.rainflow)
    {
        writer.text(fatigueField(line, 2), "RainflowCounting");
        writer.text(fatigueField(line, 3), FATIGUE_PARAMS_RAINFLOW_TYPE);
        ++line;
    }
    if (card.certainty)
    {
        writer.text(fatigueField(line, 2), "CertaintyOfSurvival");
        writer.realUnlessDefault(fatigueField(line, 3), *card.certainty, DEFAULT_CERTAINTY);
        ++line;
    }
    if (card.safety)
        line = writeSafety(writer, line, *card.safety);
    if (card.spotWeld)
        line = writeSpotWeld(writer, line, *card.spotWeld);
    if (card.seamWeld)
        line = writeSeamWeld(writer, line, *card.seamWeld);
    if (card.multi)
        line = writeMulti(writer, line, *card.multi);
    if (card.average)
        line = writeAverage(writer, line, *card.average);
    if (card.vibration)
        line = writeVibration(writer, line, *card.vibration);
}

std::uint32_t FatigueParamsTable::append(const FatigueParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueParamsTable::store(std::uint32_t row, const FatigueParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FatigueParamsCard& card = m_cards.edit(row);
    // OUTPUT: -1 is the element system and 0 the basic system, which no card defines; a positive number is a CORDij.
    if (card.average && card.average->output > 0)
        visitId(visitor, "output", ReferenceTarget::Coordinate, card.average->output);
}

} // namespace oor::model
