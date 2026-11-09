#include "oor/model/cards/properties/tiltingPadBearingProp/TiltingPadBearingPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <utility>

namespace oor::model
{

namespace
{

// The card is a grid of lines of eight fields. A place is a column of the line, 1 to 8 as the format reference draws
// them after the card name; the field of the FieldReader is the line number (from 0) times eight, plus the place.
constexpr int FIELDS_PER_LINE = 8;

constexpr int LINE_HEAD = 0;   // RID  TLIMIT
constexpr int LINE_TITLE1 = 1; // text over the eight places
constexpr int LINE_TITLE2 = 2;
constexpr int LINE_BODY = 3;   // RADIUS ... ALPHS
constexpr int LINE_COUNTS = 4; // NPADS  NEL  IECC
constexpr int LINE_FIRST_PAD = 5;

// The lines after the pads, counted from the first of them (the one after the last pad).
constexpr int AFTER_THERMAL = 0;    // KFILM ... CCAV
constexpr int AFTER_FLUID = 1;      // DENSITY ... TMANU
constexpr int AFTER_SOLVER = 2;     // ERROR XG YG . . NAX FACTOR XFACT
constexpr int AFTER_FLAGS = 3;      // ITJ IBC ITUR IDIM . ICOND ITB ITG
constexpr int AFTER_MORE_FLAGS = 4; // . . ICAV ICROSS IDEF IFLEX IUN ISUMP
constexpr int AFTER_PIVOT = 5;      // KTHETA DEL0 HOTOVER COLDOVER
constexpr int AFTER_CASES = 6;      // NCASE MAXC IALPH
constexpr int AFTER_FIRST_CASE = 7;

constexpr int fieldOf(int line, int place) noexcept
{
    return FIELDS_PER_LINE * line + place;
}

// The values of the cells of the grid.
std::optional<double> realAt(FieldReader& reader, int line, int place, const char* name)
{
    return reader.optionalReal(fieldOf(line, place), name);
}

std::optional<std::int32_t> integerAt(FieldReader& reader, int line, int place, const char* name)
{
    return reader.optionalInteger(fieldOf(line, place), name);
}

void writeRealAt(FieldWriter& writer, int line, int place, const std::optional<double>& value)
{
    if (value)
        writer.real(fieldOf(line, place), *value);
}

void writeIntegerAt(FieldWriter& writer, int line, int place, const std::optional<std::int32_t>& value)
{
    if (value)
        writer.integer(fieldOf(line, place), *value);
}

// The cells of a line that the layout leaves blank: the deck may not fill them.
void requireBlankCells(FieldReader& reader, int line, std::initializer_list<int> places)
{
    for (const int place : places)
    {
        if (!reader.isBlank(fieldOf(line, place)))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(fieldOf(line, place)) + " of a " + std::string(reader.cardName()) +
                             " is a blank cell of its layout and must be blank",
                         "");
            return;
        }
    }
}

// The name of the value `index` of a row (ARC3, RPM2): the format numbers the rows from 1.
std::string rowValueName(const char* name, int row)
{
    return std::string(name) + std::to_string(row + 1);
}

std::optional<CardProblem> checkTitle(const std::string& title, const char* field)
{
    return title.empty() ? std::nullopt : checkNameWord(title, field);
}

// A flag takes one of the values the format lists.
std::optional<CardProblem> checkFlag(const std::optional<std::int32_t>& value, const char* field,
                                     std::initializer_list<std::int32_t> allowed)
{
    if (!value || std::find(allowed.begin(), allowed.end(), *value) != allowed.end())
        return std::nullopt;
    std::string phrase;
    for (const std::int32_t option : allowed)
        phrase += (phrase.empty() ? "" : ", ") + std::to_string(option);
    return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be one of " + phrase, field};
}

std::optional<CardProblem> checkRowCount(std::size_t count, const char* field)
{
    if (count < 1 || count > static_cast<std::size_t>(TILTING_PAD_BEARING_MAXIMUM_ROWS))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be between 1 and " +
                               std::to_string(TILTING_PAD_BEARING_MAXIMUM_ROWS),
                           field};
    return std::nullopt;
}

} // namespace

std::optional<TiltingPadBearingPropCard> TiltingPadBearingPropFormat::read(FieldReader& reader)
{
    TiltingPadBearingPropCard card;
    card.id = reader.requiredInteger(fieldOf(LINE_HEAD, 1), "id");
    card.timeLimit = realAt(reader, LINE_HEAD, 2, "timeLimit");
    requireBlankCells(reader, LINE_HEAD, {3, 4, 5, 6, 7, 8});
    card.title1 = reader.spanText(fieldOf(LINE_TITLE1, 1), fieldOf(LINE_TITLE1, FIELDS_PER_LINE));
    card.title2 = reader.spanText(fieldOf(LINE_TITLE2, 1), fieldOf(LINE_TITLE2, FIELDS_PER_LINE));

    card.journalRadius = realAt(reader, LINE_BODY, 1, "journalRadius");
    card.radialClearance = realAt(reader, LINE_BODY, 2, "radialClearance");
    card.padOuterRadius = realAt(reader, LINE_BODY, 3, "padOuterRadius");
    card.bearingLength = realAt(reader, LINE_BODY, 4, "bearingLength");
    card.padModulus = realAt(reader, LINE_BODY, 5, "padModulus");
    card.padExpansion = realAt(reader, LINE_BODY, 6, "padExpansion");
    card.journalExpansion = realAt(reader, LINE_BODY, 7, "journalExpansion");
    card.housingExpansion = realAt(reader, LINE_BODY, 8, "housingExpansion");

    const std::int32_t pads = reader.requiredInteger(fieldOf(LINE_COUNTS, 1), "padCount");
    card.elementCount = integerAt(reader, LINE_COUNTS, 2, "elementCount");
    card.eccentricityInput = integerAt(reader, LINE_COUNTS, 3, "eccentricityInput");
    requireBlankCells(reader, LINE_COUNTS, {4, 5, 6, 7, 8});
    if (pads < 1 || pads > TILTING_PAD_BEARING_MAXIMUM_ROWS)
    {
        // The count decides where every later line is: nothing more can be read.
        if (!reader.failed())
            reader.error(
                "card.bad-value",
                std::string("padCount") + " must be between 1 and " + std::to_string(TILTING_PAD_BEARING_MAXIMUM_ROWS),
                "padCount");
        return card;
    }
    for (int pad = 0; pad < pads; ++pad)
    {
        TiltingPadBearingPropPad values;
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_PAD_VALUES; ++value)
            values[value] = realAt(reader, LINE_FIRST_PAD + pad, static_cast<int>(value) + 1,
                                   rowValueName(TILTING_PAD_BEARING_PAD_NAMES[value], pad).c_str());
        card.pads.push_back(values);
    }

    const int after = LINE_FIRST_PAD + pads;
    card.oilConductivity = realAt(reader, after + AFTER_THERMAL, 1, "oilConductivity");
    card.padConductivity = realAt(reader, after + AFTER_THERMAL, 2, "padConductivity");
    card.padBackTemperature = realAt(reader, after + AFTER_THERMAL, 3, "padBackTemperature");
    card.journalTemperature = realAt(reader, after + AFTER_THERMAL, 4, "journalTemperature");
    card.sumpPressure = realAt(reader, after + AFTER_THERMAL, 5, "sumpPressure");
    card.inletTemperature = realAt(reader, after + AFTER_THERMAL, 6, "inletTemperature");
    card.cavitationConductivity = realAt(reader, after + AFTER_THERMAL, 7, "cavitationConductivity");
    card.cavitationSpecificHeat = realAt(reader, after + AFTER_THERMAL, 8, "cavitationSpecificHeat");

    card.oilDensity = realAt(reader, after + AFTER_FLUID, 1, "oilDensity");
    card.oilSpecificHeat = realAt(reader, after + AFTER_FLUID, 2, "oilSpecificHeat");
    card.viscosityTemperatureA = realAt(reader, after + AFTER_FLUID, 3, "viscosityTemperatureA");
    card.viscosityA = realAt(reader, after + AFTER_FLUID, 4, "viscosityA");
    card.viscosityTemperatureB = realAt(reader, after + AFTER_FLUID, 5, "viscosityTemperatureB");
    card.viscosityB = realAt(reader, after + AFTER_FLUID, 6, "viscosityB");
    card.sumpMixingFactor = realAt(reader, after + AFTER_FLUID, 7, "sumpMixingFactor");
    card.manufacturingTemperature = realAt(reader, after + AFTER_FLUID, 8, "manufacturingTemperature");

    card.convergenceTolerance = realAt(reader, after + AFTER_SOLVER, 1, "convergenceTolerance");
    card.initialPositionX = realAt(reader, after + AFTER_SOLVER, 2, "initialPositionX");
    card.initialPositionY = realAt(reader, after + AFTER_SOLVER, 3, "initialPositionY");
    requireBlankCells(reader, after + AFTER_SOLVER, {4, 5});
    card.axialNodeCount = realAt(reader, after + AFTER_SOLVER, 6, "axialNodeCount");
    card.pressureRelaxation = realAt(reader, after + AFTER_SOLVER, 7, "pressureRelaxation");
    card.temperatureRelaxation = realAt(reader, after + AFTER_SOLVER, 8, "temperatureRelaxation");

    card.journalThermalModel = integerAt(reader, after + AFTER_FLAGS, 1, "journalThermalModel");
    card.inletThermalBoundary = integerAt(reader, after + AFTER_FLAGS, 2, "inletThermalBoundary");
    card.turbulence = integerAt(reader, after + AFTER_FLAGS, 3, "turbulence");
    card.dimensionalOutput = integerAt(reader, after + AFTER_FLAGS, 4, "dimensionalOutput");
    requireBlankCells(reader, after + AFTER_FLAGS, {5});
    card.padConduction = integerAt(reader, after + AFTER_FLAGS, 6, "padConduction");
    card.padBackThermal = integerAt(reader, after + AFTER_FLAGS, 7, "padBackThermal");
    card.journalThermalGradient = integerAt(reader, after + AFTER_FLAGS, 8, "journalThermalGradient");

    requireBlankCells(reader, after + AFTER_MORE_FLAGS, {1, 2});
    card.cavitation = integerAt(reader, after + AFTER_MORE_FLAGS, 3, "cavitation");
    card.crossCoupling = integerAt(reader, after + AFTER_MORE_FLAGS, 4, "crossCoupling");
    card.padDeformation = integerAt(reader, after + AFTER_MORE_FLAGS, 5, "padDeformation");
    card.pivotFlexibility = integerAt(reader, after + AFTER_MORE_FLAGS, 6, "pivotFlexibility");
    card.unitSystem = integerAt(reader, after + AFTER_MORE_FLAGS, 7, "unitSystem");
    card.floodedSump = integerAt(reader, after + AFTER_MORE_FLAGS, 8, "floodedSump");

    card.pivotRotationStiffness = reader.real(fieldOf(after + AFTER_PIVOT, 1), "pivotRotationStiffness",
                                              TILTING_PAD_BEARING_DEFAULT_PIVOT_ROTATION_STIFFNESS);
    card.initialPivotDeflection = reader.real(fieldOf(after + AFTER_PIVOT, 2), "initialPivotDeflection",
                                              TILTING_PAD_BEARING_DEFAULT_INITIAL_PIVOT_DEFLECTION);
    card.hotOilCarryOver =
        reader.real(fieldOf(after + AFTER_PIVOT, 3), "hotOilCarryOver", TILTING_PAD_BEARING_DEFAULT_HOT_OIL_CARRY_OVER);
    card.coldOilCarryOver = reader.real(fieldOf(after + AFTER_PIVOT, 4), "coldOilCarryOver",
                                        TILTING_PAD_BEARING_DEFAULT_COLD_OIL_CARRY_OVER);
    requireBlankCells(reader, after + AFTER_PIVOT, {5, 6, 7, 8});

    const std::int32_t cases = reader.requiredInteger(fieldOf(after + AFTER_CASES, 1), "caseCount");
    card.maxIterations =
        reader.real(fieldOf(after + AFTER_CASES, 2), "maxIterations", TILTING_PAD_BEARING_DEFAULT_MAX_ITERATIONS);
    card.stiffnessSmoothing = realAt(reader, after + AFTER_CASES, 3, "stiffnessSmoothing");
    requireBlankCells(reader, after + AFTER_CASES, {4, 5, 6, 7, 8});
    if (cases < 1 || cases > TILTING_PAD_BEARING_MAXIMUM_ROWS)
    {
        if (!reader.failed())
            reader.error(
                "card.bad-value",
                std::string("caseCount") + " must be between 1 and " + std::to_string(TILTING_PAD_BEARING_MAXIMUM_ROWS),
                "caseCount");
        return card;
    }
    for (int index = 0; index < cases; ++index)
    {
        TiltingPadBearingPropCase values;
        const int line = after + AFTER_FIRST_CASE + index;
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_CASE_VALUES; ++value)
            values[value] = realAt(reader, line, static_cast<int>(value) + 1,
                                   rowValueName(TILTING_PAD_BEARING_CASE_NAMES[value], index).c_str());
        requireBlankCells(reader, line, {8});
        card.cases.push_back(values);
    }
    if (reader.lastUsedField() > fieldOf(after + AFTER_FIRST_CASE + cases - 1, FIELDS_PER_LINE))
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field after the last of its " +
                         std::string("caseCount") + " case lines",
                     "caseCount");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TiltingPadBearingPropFormat::check(const TiltingPadBearingPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkTitle(card.title1, "title1"))
        return problem;
    if (const auto problem = checkTitle(card.title2, "title2"))
        return problem;
    const std::pair<const char*, const std::optional<double>*> reals[] = {
        {"timeLimit", &card.timeLimit},
        {"journalRadius", &card.journalRadius},
        {"radialClearance", &card.radialClearance},
        {"padOuterRadius", &card.padOuterRadius},
        {"bearingLength", &card.bearingLength},
        {"padModulus", &card.padModulus},
        {"padExpansion", &card.padExpansion},
        {"journalExpansion", &card.journalExpansion},
        {"housingExpansion", &card.housingExpansion},
        {"oilConductivity", &card.oilConductivity},
        {"padConductivity", &card.padConductivity},
        {"padBackTemperature", &card.padBackTemperature},
        {"journalTemperature", &card.journalTemperature},
        {"sumpPressure", &card.sumpPressure},
        {"inletTemperature", &card.inletTemperature},
        {"cavitationConductivity", &card.cavitationConductivity},
        {"cavitationSpecificHeat", &card.cavitationSpecificHeat},
        {"oilDensity", &card.oilDensity},
        {"oilSpecificHeat", &card.oilSpecificHeat},
        {"viscosityTemperatureA", &card.viscosityTemperatureA},
        {"viscosityA", &card.viscosityA},
        {"viscosityTemperatureB", &card.viscosityTemperatureB},
        {"viscosityB", &card.viscosityB},
        {"sumpMixingFactor", &card.sumpMixingFactor},
        {"manufacturingTemperature", &card.manufacturingTemperature},
        {"convergenceTolerance", &card.convergenceTolerance},
        {"initialPositionX", &card.initialPositionX},
        {"initialPositionY", &card.initialPositionY},
        {"axialNodeCount", &card.axialNodeCount},
        {"pressureRelaxation", &card.pressureRelaxation},
        {"temperatureRelaxation", &card.temperatureRelaxation},
        {"stiffnessSmoothing", &card.stiffnessSmoothing}};
    for (const auto& [name, value] : reals)
    {
        if (const auto problem = checkOptionalReal(*value, name, RealRule::Finite))
            return problem;
    }
    if (const auto problem = checkReal(card.pivotRotationStiffness, "pivotRotationStiffness", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.initialPivotDeflection, "initialPivotDeflection", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.hotOilCarryOver, "hotOilCarryOver", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.coldOilCarryOver, "coldOilCarryOver", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.maxIterations, "maxIterations", RealRule::Finite))
        return problem;

    if (card.elementCount && *card.elementCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "elementCount"};
    if (const auto problem = checkFlag(card.eccentricityInput, "eccentricityInput", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.journalThermalModel, "journalThermalModel", {0, 1, 2}))
        return problem;
    if (const auto problem = checkFlag(card.inletThermalBoundary, "inletThermalBoundary", {0, 1, 2}))
        return problem;
    if (const auto problem = checkFlag(card.turbulence, "turbulence", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.dimensionalOutput, "dimensionalOutput", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.padConduction, "padConduction", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.padBackThermal, "padBackThermal", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.journalThermalGradient, "journalThermalGradient", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.cavitation, "cavitation", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.crossCoupling, "crossCoupling", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.padDeformation, "padDeformation", {0, 1, 2, 4}))
        return problem;
    if (const auto problem = checkFlag(card.pivotFlexibility, "pivotFlexibility", {0, 1, 2}))
        return problem;
    if (const auto problem = checkFlag(card.unitSystem, "unitSystem", {0, 1}))
        return problem;
    if (const auto problem = checkFlag(card.floodedSump, "floodedSump", {0, 1}))
        return problem;

    if (const auto problem = checkRowCount(card.pads.size(), "padCount"))
        return problem;
    for (std::size_t pad = 0; pad < card.pads.size(); ++pad)
    {
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_PAD_VALUES; ++value)
        {
            if (const auto problem =
                    checkOptionalReal(card.pads[pad][value], TILTING_PAD_BEARING_PAD_NAMES[value], RealRule::Finite))
                return problem;
        }
    }
    if (const auto problem = checkRowCount(card.cases.size(), "caseCount"))
        return problem;
    for (std::size_t index = 0; index < card.cases.size(); ++index)
    {
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_CASE_VALUES; ++value)
        {
            if (const auto problem = checkOptionalReal(card.cases[index][value], TILTING_PAD_BEARING_CASE_NAMES[value],
                                                       RealRule::Finite))
                return problem;
        }
    }
    return std::nullopt;
}

void TiltingPadBearingPropFormat::write(const TiltingPadBearingPropCard& card, FieldWriter& writer)
{
    writer.integer(fieldOf(LINE_HEAD, 1), card.id);
    writeRealAt(writer, LINE_HEAD, 2, card.timeLimit);
    writer.spanText(fieldOf(LINE_TITLE1, 1), FIELDS_PER_LINE, card.title1);
    writer.spanText(fieldOf(LINE_TITLE2, 1), FIELDS_PER_LINE, card.title2);

    writeRealAt(writer, LINE_BODY, 1, card.journalRadius);
    writeRealAt(writer, LINE_BODY, 2, card.radialClearance);
    writeRealAt(writer, LINE_BODY, 3, card.padOuterRadius);
    writeRealAt(writer, LINE_BODY, 4, card.bearingLength);
    writeRealAt(writer, LINE_BODY, 5, card.padModulus);
    writeRealAt(writer, LINE_BODY, 6, card.padExpansion);
    writeRealAt(writer, LINE_BODY, 7, card.journalExpansion);
    writeRealAt(writer, LINE_BODY, 8, card.housingExpansion);

    const int pads = static_cast<int>(card.pads.size());
    writer.integer(fieldOf(LINE_COUNTS, 1), pads);
    writeIntegerAt(writer, LINE_COUNTS, 2, card.elementCount);
    writeIntegerAt(writer, LINE_COUNTS, 3, card.eccentricityInput);
    for (int pad = 0; pad < pads; ++pad)
    {
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_PAD_VALUES; ++value)
            writeRealAt(writer, LINE_FIRST_PAD + pad, static_cast<int>(value) + 1,
                        card.pads[static_cast<std::size_t>(pad)][value]);
    }

    const int after = LINE_FIRST_PAD + pads;
    writeRealAt(writer, after + AFTER_THERMAL, 1, card.oilConductivity);
    writeRealAt(writer, after + AFTER_THERMAL, 2, card.padConductivity);
    writeRealAt(writer, after + AFTER_THERMAL, 3, card.padBackTemperature);
    writeRealAt(writer, after + AFTER_THERMAL, 4, card.journalTemperature);
    writeRealAt(writer, after + AFTER_THERMAL, 5, card.sumpPressure);
    writeRealAt(writer, after + AFTER_THERMAL, 6, card.inletTemperature);
    writeRealAt(writer, after + AFTER_THERMAL, 7, card.cavitationConductivity);
    writeRealAt(writer, after + AFTER_THERMAL, 8, card.cavitationSpecificHeat);

    writeRealAt(writer, after + AFTER_FLUID, 1, card.oilDensity);
    writeRealAt(writer, after + AFTER_FLUID, 2, card.oilSpecificHeat);
    writeRealAt(writer, after + AFTER_FLUID, 3, card.viscosityTemperatureA);
    writeRealAt(writer, after + AFTER_FLUID, 4, card.viscosityA);
    writeRealAt(writer, after + AFTER_FLUID, 5, card.viscosityTemperatureB);
    writeRealAt(writer, after + AFTER_FLUID, 6, card.viscosityB);
    writeRealAt(writer, after + AFTER_FLUID, 7, card.sumpMixingFactor);
    writeRealAt(writer, after + AFTER_FLUID, 8, card.manufacturingTemperature);

    writeRealAt(writer, after + AFTER_SOLVER, 1, card.convergenceTolerance);
    writeRealAt(writer, after + AFTER_SOLVER, 2, card.initialPositionX);
    writeRealAt(writer, after + AFTER_SOLVER, 3, card.initialPositionY);
    writeRealAt(writer, after + AFTER_SOLVER, 6, card.axialNodeCount);
    writeRealAt(writer, after + AFTER_SOLVER, 7, card.pressureRelaxation);
    writeRealAt(writer, after + AFTER_SOLVER, 8, card.temperatureRelaxation);

    writeIntegerAt(writer, after + AFTER_FLAGS, 1, card.journalThermalModel);
    writeIntegerAt(writer, after + AFTER_FLAGS, 2, card.inletThermalBoundary);
    writeIntegerAt(writer, after + AFTER_FLAGS, 3, card.turbulence);
    writeIntegerAt(writer, after + AFTER_FLAGS, 4, card.dimensionalOutput);
    writeIntegerAt(writer, after + AFTER_FLAGS, 6, card.padConduction);
    writeIntegerAt(writer, after + AFTER_FLAGS, 7, card.padBackThermal);
    writeIntegerAt(writer, after + AFTER_FLAGS, 8, card.journalThermalGradient);

    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 3, card.cavitation);
    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 4, card.crossCoupling);
    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 5, card.padDeformation);
    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 6, card.pivotFlexibility);
    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 7, card.unitSystem);
    writeIntegerAt(writer, after + AFTER_MORE_FLAGS, 8, card.floodedSump);

    writer.realUnlessDefault(fieldOf(after + AFTER_PIVOT, 1), card.pivotRotationStiffness,
                             TILTING_PAD_BEARING_DEFAULT_PIVOT_ROTATION_STIFFNESS);
    writer.realUnlessDefault(fieldOf(after + AFTER_PIVOT, 2), card.initialPivotDeflection,
                             TILTING_PAD_BEARING_DEFAULT_INITIAL_PIVOT_DEFLECTION);
    writer.realUnlessDefault(fieldOf(after + AFTER_PIVOT, 3), card.hotOilCarryOver,
                             TILTING_PAD_BEARING_DEFAULT_HOT_OIL_CARRY_OVER);
    writer.realUnlessDefault(fieldOf(after + AFTER_PIVOT, 4), card.coldOilCarryOver,
                             TILTING_PAD_BEARING_DEFAULT_COLD_OIL_CARRY_OVER);

    writer.integer(fieldOf(after + AFTER_CASES, 1), static_cast<std::int64_t>(card.cases.size()));
    writer.realUnlessDefault(fieldOf(after + AFTER_CASES, 2), card.maxIterations,
                             TILTING_PAD_BEARING_DEFAULT_MAX_ITERATIONS);
    writeRealAt(writer, after + AFTER_CASES, 3, card.stiffnessSmoothing);
    for (std::size_t index = 0; index < card.cases.size(); ++index)
    {
        for (std::size_t value = 0; value < TILTING_PAD_BEARING_CASE_VALUES; ++value)
            writeRealAt(writer, after + AFTER_FIRST_CASE + static_cast<int>(index), static_cast<int>(value) + 1,
                        card.cases[index][value]);
    }
}

std::uint32_t TiltingPadBearingPropTable::append(const TiltingPadBearingPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TiltingPadBearingPropTable::store(std::uint32_t row, const TiltingPadBearingPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TiltingPadBearingPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TiltingPadBearingPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TiltingPadBearingPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PBUSH2D (and of the ELEMUDS) of the bearing. The ID column follows the card.
    TiltingPadBearingPropCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
