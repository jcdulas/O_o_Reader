#include "oor/model/cards/tables/tableTimeHistoryFile/TableTimeHistoryFileTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

// The pairs start on the first continuation line, in field 2 of its format reference numbering: field 9 of the reader.
constexpr int FIRST_PAIR_FIELD = 9;

bool isWord(std::string_view text, std::string_view word)
{
    return (text == word);
}

TableTimeHistoryFileBound readBound(FieldReader& reader, int field, const char* name)
{
    TableTimeHistoryFileBound bound;
    if (isWord(reader.text(field), "Skip"))
        bound.skip = true;
    else
        bound.time = reader.optionalReal(field, name);
    return bound;
}

void writeBound(FieldWriter& writer, int field, const TableTimeHistoryFileBound& bound)
{
    if (bound.skip)
        writer.text(field, "Skip");
    else if (bound.time)
        writer.real(field, *bound.time);
}

// PUNCH is YES or NO; blank is NO.
bool readPunch(FieldReader& reader)
{
    const std::string_view word = reader.text(8);
    if (word.empty() || isWord(word, "No"))
        return false;
    if (isWord(word, "Yes"))
        return true;
    reader.error("card.bad-value", std::string("punch") + " must be YES or NO, not \"" + std::string(word) + "\"",
                 "punch");
    return false;
}

// Reads the pairs of the continuation lines and the word ENDT that ends them.
void readWindows(FieldReader& reader, TableTimeHistoryFileCard& card)
{
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        const std::string_view start = reader.text(field);
        const std::string_view end = reader.text(field + 1);
        if (ended)
        {
            if (!start.empty() || !end.empty())
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            continue;
        }
        if (isWord(start, "EndTable"))
        {
            ended = true;
            if (!end.empty())
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            continue;
        }
        TableTimeHistoryFileWindow window;
        window.start = readBound(reader, field, "windowStart");
        if (isWord(end, "EndTable"))
        {
            // The word is in the field of the end of a pair: the pair is the start only (or nothing when the start is
            // blank).
            ended = true;
            if (window.start.skip || window.start.time)
                card.windows.push_back(window);
            continue;
        }
        window.end = readBound(reader, field + 1, "windowEnd");
        card.windows.push_back(window);
    }
    if (!ended && !card.windows.empty() && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
}

} // namespace

std::optional<TableTimeHistoryFileCard> TableTimeHistoryFileFormat::read(FieldReader& reader)
{
    TableTimeHistoryFileCard card;
    card.id = reader.requiredInteger(1, "id");
    card.xAxisKind =
        readKeyword(reader, 2, "xAxisKind", TIME_HISTORY_FILE_AXIS_WORDS, TableTimeHistoryFileAxis::Linear);
    card.yAxisKind =
        readKeyword(reader, 3, "yAxisKind", TIME_HISTORY_FILE_AXIS_WORDS, TableTimeHistoryFileAxis::Linear);
    card.externalFileId = reader.requiredInteger(4, "externalFileId");
    card.fileType =
        readKeyword(reader, 5, "fileType", TIME_HISTORY_FILE_TYPE_NAMES, TableTimeHistoryFileFileType::LoadHistoryFile);
    card.channel = reader.integer(6, "channel", 0);
    card.totalTime = reader.optionalReal(7, "totalTime");
    card.punch = readPunch(reader);
    readWindows(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableTimeHistoryFileFormat::check(const TableTimeHistoryFileCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.externalFileId == 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) +
                " must not be zero: it is the number of an external file (negative to leave the pairs out)",
            "externalFileId"};
    if (const auto problem = checkOptionalId(card.channel, "channel"))
        return problem;
    if (const auto problem = checkOptionalReal(card.totalTime, "totalTime", RealRule::Positive))
        return problem;
    double previous = -std::numeric_limits<double>::infinity();
    for (const TableTimeHistoryFileWindow& window : card.windows)
    {
        for (const TableTimeHistoryFileBound* bound : {&window.start, &window.end})
        {
            if (bound->skip && bound->time)
                return CardProblem{"card.bad-value", "a field of a pair is a time or SKIP, not both", "windowStart"};
            if (bound->time && !std::isfinite(*bound->time))
                return CardProblem{"card.bad-value", "the times of the pairs must be finite numbers", "windowStart"};
        }
        if (window.start.skip || window.end.skip)
            continue; // a pair that is ignored has no order
        for (const TableTimeHistoryFileBound* bound : {&window.start, &window.end})
        {
            if (!bound->time)
                continue;
            if (*bound->time < previous)
                return CardProblem{
                    "card.bad-value",
                    std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("windowEnd") +
                        " must be in the order of the points of the file: no value is below the one before it",
                    "windowStart"};
            previous = *bound->time;
        }
    }
    return std::nullopt;
}

void TableTimeHistoryFileFormat::write(const TableTimeHistoryFileCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, TIME_HISTORY_FILE_AXIS_WORDS, card.xAxisKind,
                              TableTimeHistoryFileAxis::Linear);
    writeKeywordUnlessDefault(writer, 3, TIME_HISTORY_FILE_AXIS_WORDS, card.yAxisKind,
                              TableTimeHistoryFileAxis::Linear);
    writer.integer(4, card.externalFileId);
    writeKeywordUnlessDefault(writer, 5, TIME_HISTORY_FILE_TYPE_NAMES, card.fileType,
                              TableTimeHistoryFileFileType::LoadHistoryFile);
    if (card.channel > 0)
        writer.integer(6, card.channel);
    if (card.totalTime)
        writer.real(7, *card.totalTime);
    writer.textUnlessDefault(8, card.punch ? "Yes" : "No", "No");
    int field = FIRST_PAIR_FIELD;
    for (const TableTimeHistoryFileWindow& window : card.windows)
    {
        writeBound(writer, field, window.start);
        writeBound(writer, field + 1, window.end);
        field += 2;
    }
    if (!card.windows.empty())
        writer.text(field, "EndTable");
}

std::uint32_t TableTimeHistoryFileTable::append(const TableTimeHistoryFileCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TableTimeHistoryFileTable::store(std::uint32_t row, const TableTimeHistoryFileCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TableTimeHistoryFileTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TableTimeHistoryFileTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TableTimeHistoryFileTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    TableTimeHistoryFileCard& card = m_cards.edit(row);
    if (card.externalFileId == 0)
        return;
    // A negative UID inverts the pairs: the sign stays on the card, the UDNAME has the absolute value.
    const base::EntityId updated =
        visitor.visit("externalFileId", ReferenceTarget::UserDefinedName, std::abs(card.externalFileId));
    card.externalFileId = card.externalFileId < 0 ? -updated : updated;
}

} // namespace oor::model
