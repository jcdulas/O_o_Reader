#include "oor/model/cards/solution_control/fatigueSequence/FatigueSequenceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// A line of pairs holds four of them, from field 2 of the line: FIDi and Ni side by side.
constexpr int PAIRS_PER_LINE = 4;
constexpr int UNITS_NAME_FIELDS = 6; // EQNAME: fields 4 to 9 of the line

// The format reference field of the FIDi of the pair `position` (0 to 3) of a line; its Ni is the next one.
constexpr int fidFormatField(int position) noexcept
{
    return 2 + 2 * position;
}

// The line that holds the pair number `index` (from 0).
int pairLine(std::size_t index) noexcept
{
    return 1 + static_cast<int>(index) / PAIRS_PER_LINE;
}

bool isUnitsKeyword(const FieldReader& reader, int line)
{
    return (reader.text(fatigueField(line, 2)) == "Units");
}

} // namespace

std::optional<FatigueSequenceCard> FatigueSequenceFormat::read(FieldReader& reader)
{
    FatigueSequenceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.eventOutput = reader.integer(2, "eventOutput", 0);
    // Field 4 of the format reference is METHOD in the format of SOL 101, 103 and 112, blank in the other; TUNIT and
    // LDM follow it.
    card.combinationMethod = reader.integer(3, "combinationMethod", 0);
    card.timeUnit = readKeyword(reader, 4, "timeUnit", FATIGUE_SEQUENCE_TIME_UNIT_WORDS, FatigueSequenceTimeUnit::Secs);
    card.referenceLoadMagnitude = reader.real(5, "referenceLoadMagnitude", 1.0);
    requireBlankLineFields(reader, 0, 7, 9, "first");
    const int lastLine = lastUsedLine(reader);
    for (int line = 1; line <= lastLine && !reader.failed(); ++line)
    {
        if (isUnitsKeyword(reader, line))
        {
            if (line != lastLine)
            {
                reader.error(
                    "card.bad-value",
                    "the " + std::string("units") + " line is the last line of an " + std::string(reader.cardName()),
                    "units");
                break;
            }
            FatigueSequenceUnits units;
            units.equivalentUnits = reader.real(fatigueField(line, 3), "equivalentUnits", 1.0);
            units.name = reader.spanText(fatigueField(line, 4), fatigueField(line, 3 + UNITS_NAME_FIELDS));
            card.units = units;
            break;
        }
        for (int position = 0; position < PAIRS_PER_LINE; ++position)
        {
            const int itemField = fatigueField(line, fidFormatField(position));
            const int repeatField = itemField + 1;
            if (reader.isBlank(itemField) && reader.isBlank(repeatField))
                continue; // a pair that is not used
            if (reader.isBlank(itemField))
            {
                reader.error("card.missing-field", "a number of repeats needs the event or sequence before it",
                             "functionId1");
                break;
            }
            FatigueSequenceItem item;
            item.eventOrSequenceId = reader.requiredInteger(itemField, "functionId1");
            item.repeats = reader.real(repeatField, "repeats", 1.0);
            card.items.push_back(item);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueSequenceFormat::check(const FatigueSequenceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.eventOutput != 0 && card.eventOutput != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "eventOutput"};
    if (card.combinationMethod < 0 || card.combinationMethod > 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1 or 2",
                           "combinationMethod"};
    if (const auto problem = checkReal(card.referenceLoadMagnitude, "referenceLoadMagnitude", RealRule::Finite))
        return problem;
    if (card.referenceLoadMagnitude == 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero",
                           "referenceLoadMagnitude"};
    // The two formats are told apart by METHOD (SOL 101, 103, 112) against TUNIT and LDM (SOL 108, 111).
    if (card.combinationMethod != 0 &&
        (card.timeUnit != FatigueSequenceTimeUnit::Secs || card.referenceLoadMagnitude != 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is a field of the format of SOL 101, 103 and 112, " +
                               CardProblem::fieldNamed("timeUnit") + " and " +
                               CardProblem::fieldNamed("referenceLoadMagnitude") +
                               " of the format of "
                               "SOL 108 and 111: a card has one of them",
                           "combinationMethod"};
    if (card.items.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one event or sequence, " + std::string(CardProblem::FIELD),
                           "functionId1"};
    for (const FatigueSequenceItem& item : card.items)
    {
        if (const auto problem = checkRequiredId(item.eventOrSequenceId, "functionId1"))
            return problem;
        if (const auto problem = checkReal(item.repeats, "repeats", RealRule::Positive))
            return problem;
    }
    if (card.units)
    {
        if (const auto problem = checkReal(card.units->equivalentUnits, "equivalentUnits", RealRule::Positive))
            return problem;
        if (!card.units->name.empty())
            return checkSpannedText(card.units->name, "equivalentUnitName");
    }
    return std::nullopt;
}

void FatigueSequenceFormat::write(const FatigueSequenceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.eventOutput, 0);
    writer.integerUnlessDefault(3, card.combinationMethod, 0);
    writeKeywordUnlessDefault(writer, 4, FATIGUE_SEQUENCE_TIME_UNIT_WORDS, card.timeUnit,
                              FatigueSequenceTimeUnit::Secs);
    writer.realUnlessDefault(5, card.referenceLoadMagnitude, 1.0);
    for (std::size_t index = 0; index < card.items.size(); ++index)
    {
        const int itemField = fatigueField(pairLine(index), fidFormatField(static_cast<int>(index) % PAIRS_PER_LINE));
        writer.integer(itemField, card.items[index].eventOrSequenceId);
        writer.realUnlessDefault(itemField + 1, card.items[index].repeats, 1.0);
    }
    if (card.units)
    {
        const int line = card.items.empty() ? 1 : pairLine(card.items.size() - 1) + 1;
        writer.text(fatigueField(line, 2), "Units");
        writer.realUnlessDefault(fatigueField(line, 3), card.units->equivalentUnits, 1.0);
        writer.spanText(fatigueField(line, 4), UNITS_NAME_FIELDS, card.units->name);
    }
}

std::uint32_t FatigueSequenceTable::append(const FatigueSequenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueSequenceTable::store(std::uint32_t row, const FatigueSequenceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueSequenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueSequenceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueSequenceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (FatigueSequenceItem& item : m_cards.edit(row).items)
        visitId(visitor, "functionId1", ReferenceTarget::FatigueEvent, item.eventOrSequenceId);
}

} // namespace oor::model
