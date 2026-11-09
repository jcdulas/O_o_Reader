#include "oor/model/cards/solution_control/fatigueLoad/FatigueLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 3> SECTION_KEYWORDS{"Units", "DeterministicLoad", "SweepLoad"};

constexpr int TONES_PER_LINE = 2;  // a DETLOAD line has two bands: fields 3 to 5 and 6 to 8
constexpr int FIELDS_PER_TONE = 3; // Fi, Gi or Hi, Bi
constexpr int UNIT_NAME_LAST_FIELD = 9;

// The OFFSET (or MIN) of a blank field: 0.0, and the second peak of a constant amplitude cycle, -1.0, for TYPE CONST.
double defaultOffset(const std::optional<FatigueLoadType>& type) noexcept
{
    return type == FatigueLoadType::Const ? -1.0 : 0.0;
}

// The first format reference field of the band `position` (0 or 1) of a DETLOAD line.
constexpr int toneFormatField(int position) noexcept
{
    return 3 + position * FIELDS_PER_TONE;
}

void readUnits(FieldReader& reader, const LineSection& section, FatigueLoadCard& card)
{
    for (int line = section.firstLine + 1; line <= section.lastLine; ++line)
        requireBlankLineFields(reader, line, 2, 9, "Units");
    FatigueLoadUnits units;
    units.equivalentUnits = reader.real(fatigueField(section.firstLine, 3), "equivalentUnits", 1.0);
    units.name =
        reader.spanText(fatigueField(section.firstLine, 4), fatigueField(section.firstLine, UNIT_NAME_LAST_FIELD));
    card.units = units;
}

void readTones(FieldReader& reader, const LineSection& section, FatigueLoadCard& card)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        requireBlankLineFields(reader, line, 9, 9, "DeterministicLoad");
        for (int position = 0; position < TONES_PER_LINE; ++position)
        {
            const int frequencyField = fatigueField(line, toneFormatField(position));
            const int heightField = frequencyField + 1;
            const int widthField = frequencyField + 2;
            if (reader.isBlank(frequencyField) && reader.isBlank(heightField) && reader.isBlank(widthField))
                continue; // a band that is not used
            FatigueLoadTone tone;
            tone.frequency = reader.requiredReal(frequencyField, "bandFrequency");
            tone.height = reader.requiredReal(heightField, "bandAmplitude");
            tone.width = reader.optionalReal(widthField, "bandWidth");
            card.tones.push_back(tone);
        }
    }
    if (card.tones.empty() && !reader.failed())
        reader.error("card.missing-field",
                     "a DETLOAD line needs at least one band, " + std::string("bandFrequency") + " and " +
                         std::string("bandAmplitude"),
                     "bandFrequency");
}

void readSweep(FieldReader& reader, const LineSection& section, FatigueLoadCard& card)
{
    if (section.lastLine > section.firstLine + 1)
    {
        reader.error("card.bad-value",
                     "a " + std::string("sweep") + " has two lines: the frequencies and then " +
                         std::string("sweepKind") + " to " + std::string("loadQuantity"),
                     "sweep");
        return;
    }
    const int first = section.firstLine;
    FatigueLoadSweep sweep;
    sweep.firstFrequency = reader.requiredReal(fatigueField(first, 3), "sweepStartFrequency");
    sweep.firstHeight = reader.requiredReal(fatigueField(first, 4), "sweepStartAmplitude");
    sweep.lastFrequency = reader.requiredReal(fatigueField(first, 6), "sweepEndFrequency");
    sweep.lastHeight = reader.requiredReal(fatigueField(first, 7), "sweepEndAmplitude");
    requireBlankLineFields(reader, first, 5, 5, "sweep");
    requireBlankLineFields(reader, first, 8, 9, "sweep");
    if (section.lastLine == first)
    {
        reader.error("card.missing-field",
                     "a " + std::string("sweep") + " needs its second line: " + std::string("sweepKind") + ", " +
                         std::string("sweepNumber") + ", " + std::string("sweepRate") + ", " +
                         std::string("sweepCount") + ", " + std::string("transferFunctionQuantity") + ", " +
                         std::string("loadQuantity"),
                     "sweepRate");
        return;
    }
    const int second = first + 1;
    sweep.type = readKeyword(reader, fatigueField(second, 3), "sweepKind", FATIGUE_LOAD_SWEEP_TYPE_WORDS,
                             FatigueLoadSweepType::Hz);
    sweep.count = reader.integer(fatigueField(second, 4), "sweepNumber", 50);
    sweep.rate = reader.requiredReal(fatigueField(second, 5), "sweepRate");
    sweep.passes = reader.requiredInteger(fatigueField(second, 6), "sweepCount");
    sweep.frfUnit = readKeyword(reader, fatigueField(second, 7), "transferFunctionQuantity",
                                FATIGUE_LOAD_QUANTITY_WORDS, FatigueLoadQuantity::Acceleration);
    sweep.loadUnit = readKeyword(reader, fatigueField(second, 8), "loadQuantity", FATIGUE_LOAD_QUANTITY_WORDS,
                                 FatigueLoadQuantity::Acceleration);
    requireBlankLineFields(reader, second, 9, 9, "sweep");
    card.sweep = sweep;
}

void readSections(FieldReader& reader, FatigueLoadCard& card)
{
    bool hasUnits = false;
    bool hasDetload = false;
    bool hasSweep = false;
    for (const LineSection& section : cutLineSections(reader, SECTION_KEYWORDS))
    {
        if (reader.failed())
            return;
        bool& seen =
            section.keyword == "Units" ? hasUnits : (section.keyword == "DeterministicLoad" ? hasDetload : hasSweep);
        if (seen)
        {
            reader.error("card.bad-value",
                         "an " + std::string(reader.cardName()) + " has one " + section.keyword + " line",
                         std::string(section.keyword).c_str());
            return;
        }
        seen = true;
        if (section.keyword == "Units")
            readUnits(reader, section, card);
        else if (section.keyword == "DeterministicLoad")
            readTones(reader, section, card);
        else
            readSweep(reader, section, card);
    }
}

} // namespace

std::optional<FatigueLoadCard> FatigueLoadFormat::read(FieldReader& reader)
{
    FatigueLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tableId = reader.integer(2, "tableId", 0);
    card.loadCaseId = reader.requiredInteger(3, "loadCaseId");
    card.referenceLoadMagnitude = reader.real(4, "referenceLoadMagnitude", 1.0);
    card.scaleFactor = reader.real(5, "scaleFactor", 1.0);
    card.type = readOptionalKeyword<FatigueLoadType>(reader, 7, "historyKind", FATIGUE_LOAD_TYPE_WORDS);
    card.offset = reader.real(6, "offset", defaultOffset(card.type));
    card.channel = reader.integer(8, "channel", 0);
    readSections(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueLoadFormat::check(const FatigueLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.tableId, "tableId"))
        return problem;
    const bool needsTable = !card.type || *card.type == FatigueLoadType::Db ||
                            *card.type == FatigueLoadType::DataAcquisitionFile ||
                            *card.type == FatigueLoadType::LoadHistoryFile;
    if (needsTable && card.tableId == 0)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required when " +
                               CardProblem::fieldNamed("historyKind") +
                               " is blank or names a file: the table or the file of the load history",
                           "tableId"};
    if (const auto problem = checkReal(card.referenceLoadMagnitude, "referenceLoadMagnitude", RealRule::Finite))
        return problem;
    if (card.referenceLoadMagnitude == 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero",
                           "referenceLoadMagnitude"};
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.offset, "offset", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.channel, "channel"))
        return problem;
    if (card.units && (!card.tones.empty() || card.sweep))
        return CardProblem{
            "card.bad-value",
            "a UNITS line belongs to the format of SOL 101 and 103, DETLOAD and SWEEP lines to the format "
            "of SOL 108 and 111: a card has one of them",
            "Units"};
    if (card.units)
    {
        if (const auto problem = checkReal(card.units->equivalentUnits, "equivalentUnits", RealRule::Positive))
            return problem;
        if (!card.units->name.empty())
            if (const auto problem = checkSpannedText(card.units->name, "equivalentUnitName"))
                return problem;
    }
    for (const FatigueLoadTone& tone : card.tones)
    {
        if (const auto problem = checkReal(tone.frequency, "bandFrequency", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(tone.height, "bandAmplitude", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkOptionalReal(tone.width, "bandWidth", RealRule::NonNegative))
            return problem;
    }
    if (card.sweep)
    {
        const FatigueLoadSweep& sweep = *card.sweep;
        if (const auto problem = checkReal(sweep.firstFrequency, "sweepStartFrequency", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(sweep.firstHeight, "sweepStartAmplitude", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkReal(sweep.lastFrequency, "sweepEndFrequency", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(sweep.lastHeight, "sweepEndAmplitude", RealRule::NonNegative))
            return problem;
        if (sweep.count <= 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "sweepNumber"};
        if (const auto problem = checkReal(sweep.rate, "sweepRate", RealRule::Positive))
            return problem;
        if (sweep.passes <= 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "sweepCount"};
    }
    return std::nullopt;
}

void FatigueLoadFormat::write(const FatigueLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.tableId, 0);
    writer.integer(3, card.loadCaseId);
    writer.realUnlessDefault(4, card.referenceLoadMagnitude, 1.0);
    writer.realUnlessDefault(5, card.scaleFactor, 1.0);
    writer.real(6, card.offset);
    writeOptionalKeyword(writer, 7, FATIGUE_LOAD_TYPE_WORDS, card.type);
    if (card.channel > 0)
        writer.integer(8, card.channel);
    int line = 1;
    if (card.units)
    {
        writer.text(fatigueField(line, 2), "Units");
        writer.realUnlessDefault(fatigueField(line, 3), card.units->equivalentUnits, 1.0);
        writer.spanText(fatigueField(line, 4), UNIT_NAME_LAST_FIELD - 3, card.units->name);
        ++line;
    }
    if (!card.tones.empty())
    {
        writer.text(fatigueField(line, 2), "DeterministicLoad");
        for (std::size_t index = 0; index < card.tones.size(); ++index)
        {
            const FatigueLoadTone& tone = card.tones[index];
            const int toneLine = line + static_cast<int>(index) / TONES_PER_LINE;
            const int frequencyField =
                fatigueField(toneLine, toneFormatField(static_cast<int>(index) % TONES_PER_LINE));
            writer.real(frequencyField, tone.frequency);
            writer.real(frequencyField + 1, tone.height);
            if (tone.width)
                writer.real(frequencyField + 2, *tone.width);
        }
        line += (static_cast<int>(card.tones.size()) + TONES_PER_LINE - 1) / TONES_PER_LINE;
    }
    if (card.sweep)
    {
        const FatigueLoadSweep& sweep = *card.sweep;
        writer.text(fatigueField(line, 2), "SweepLoad");
        writer.real(fatigueField(line, 3), sweep.firstFrequency);
        writer.real(fatigueField(line, 4), sweep.firstHeight);
        writer.real(fatigueField(line, 6), sweep.lastFrequency);
        writer.real(fatigueField(line, 7), sweep.lastHeight);
        const int second = line + 1;
        writeKeywordUnlessDefault(writer, fatigueField(second, 3), FATIGUE_LOAD_SWEEP_TYPE_WORDS, sweep.type,
                                  FatigueLoadSweepType::Hz);
        writer.integerUnlessDefault(fatigueField(second, 4), sweep.count, 50);
        writer.real(fatigueField(second, 5), sweep.rate);
        writer.integer(fatigueField(second, 6), sweep.passes);
        writeKeywordUnlessDefault(writer, fatigueField(second, 7), FATIGUE_LOAD_QUANTITY_WORDS, sweep.frfUnit,
                                  FatigueLoadQuantity::Acceleration);
        writeKeywordUnlessDefault(writer, fatigueField(second, 8), FATIGUE_LOAD_QUANTITY_WORDS, sweep.loadUnit,
                                  FatigueLoadQuantity::Acceleration);
    }
}

std::optional<ReferenceTarget> fatigueLoadTidTarget(const FatigueLoadCard& card) noexcept
{
    if (card.tableId <= 0)
        return std::nullopt;
    if (!card.type)
        return ReferenceTarget::FatigueTable;
    switch (*card.type)
    {
    case FatigueLoadType::Db:
    case FatigueLoadType::DataAcquisitionFile:
    case FatigueLoadType::LoadHistoryFile:
    case FatigueLoadType::Psd:
    case FatigueLoadType::Static:
        return ReferenceTarget::UserDefinedName;
    case FatigueLoadType::Const:
    case FatigueLoadType::Sine:
    case FatigueLoadType::Narrow:
        break;
    }
    return std::nullopt;
}

std::uint32_t FatigueLoadTable::append(const FatigueLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueLoadTable::store(std::uint32_t row, const FatigueLoadCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueLoadTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FatigueLoadCard& card = m_cards.edit(row);
    if (const std::optional<ReferenceTarget> target = fatigueLoadTidTarget(card))
        card.tableId = visitor.visit("tableId", *target, card.tableId);
    // The LCID of a random load names the RANDPS cards of the set; a negative one is the correlation of the inputs of a
    // time history converted by a TIM2PSD, and 0 a constant offset: not references.
    if (card.type == FatigueLoadType::Psd && card.loadCaseId > 0)
        card.loadCaseId = visitor.visit("loadCaseId", ReferenceTarget::RandomSet, card.loadCaseId);
}

} // namespace oor::model
