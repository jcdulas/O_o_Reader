#include "oor/model/cards/solution_control/timeToSpectrum/TimeToSpectrumTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 3> SECTION_KEYWORDS{"Event", "Delete", "Map"};

constexpr int MAX_DELETE_INTERVALS = 3; // three pairs of times to a record: fields 4 to 9
constexpr int CHANNELS_PER_LINE = 3;    // LCIDi and CHANi side by side from field 3

constexpr int EVENT_ID_FIELD = 3;
constexpr int EVENT_LENGTH_FIELD = 8;
constexpr int EVENT_OVERLAP_FIELD = 9;

// The format reference field of the start of the interval `index` (0 to 2) of a DELETE record; its end is the next
// field.
constexpr int intervalFormatField(int index) noexcept
{
    return 4 + 2 * index;
}

// The format reference field of the LCIDi of the pair `position` (0 to 2) of a MAP line; its CHANi is the next field.
constexpr int channelFormatField(int position) noexcept
{
    return 3 + 2 * position;
}

void readEvents(FieldReader& reader, const LineSection& section, TimeToSpectrumCard& card)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        requireBlankLineFields(reader, line, 4, 7, "Event");
        // A blank line below the keyword is padding; the keyword line itself must hold an event.
        if (line != section.firstLine && !lineHasData(reader, line))
            continue;
        TimeToSpectrumEvent event;
        event.eventId = reader.requiredInteger(fatigueField(line, EVENT_ID_FIELD), "eventIds");
        event.length = reader.requiredReal(fatigueField(line, EVENT_LENGTH_FIELD), "sliceTimes");
        event.overlap = reader.real(fatigueField(line, EVENT_OVERLAP_FIELD), "sliceDurations", 0.0);
        card.events.push_back(event);
    }
}

void readDeletions(FieldReader& reader, const LineSection& section, TimeToSpectrumCard& card)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        if (line != section.firstLine && !lineHasData(reader, line))
            continue;
        TimeToSpectrumDeletion deletion;
        deletion.eventId = reader.requiredInteger(fatigueField(line, 3), "eventIds");
        bool ended = false;
        for (int index = 0; index < MAX_DELETE_INTERVALS && !reader.failed(); ++index)
        {
            const int startField = fatigueField(line, intervalFormatField(index));
            const int endField = startField + 1;
            if (reader.isBlank(startField) && reader.isBlank(endField))
            {
                ended = true;
                continue;
            }
            if (ended)
            {
                reader.error("card.bad-value", "the pairs of times of a DELETE record are filled in order",
                             "sliceTimes");
                return;
            }
            const double start = reader.requiredReal(startField, "sliceStart");
            const double end = reader.requiredReal(endField, "sliceEnd");
            deletion.intervals.emplace_back(start, end);
        }
        card.deletions.push_back(std::move(deletion));
    }
}

void readChannels(FieldReader& reader, const LineSection& section, TimeToSpectrumCard& card)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        requireBlankLineFields(reader, line, 9, 9, "Map");
        for (int position = 0; position < CHANNELS_PER_LINE; ++position)
        {
            const int loadCaseField = fatigueField(line, channelFormatField(position));
            const int channelField = loadCaseField + 1;
            if (reader.isBlank(loadCaseField) && reader.isBlank(channelField))
                continue;
            TimeToSpectrumChannel channel;
            channel.loadCaseId = reader.requiredInteger(loadCaseField, "loadCaseIds");
            channel.channel = reader.requiredInteger(channelField, "channels");
            card.channels.push_back(channel);
        }
    }
}

void readSections(FieldReader& reader, TimeToSpectrumCard& card)
{
    bool seen[SECTION_KEYWORDS.size()] = {};
    for (const LineSection& section : cutLineSections(reader, SECTION_KEYWORDS))
    {
        if (reader.failed())
            return;
        std::size_t which = 0;
        while (SECTION_KEYWORDS[which] != section.keyword)
            ++which;
        if (seen[which])
        {
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " has one " + section.keyword + " section",
                         std::string(section.keyword).c_str());
            return;
        }
        seen[which] = true;
        if (which == 0)
            readEvents(reader, section, card);
        else if (which == 1)
            readDeletions(reader, section, card);
        else
            readChannels(reader, section, card);
    }
}

// MEANS is YES or NO; blank is NO.
bool readMeanStress(FieldReader& reader)
{
    const std::string_view word = reader.text(5);
    if (word.empty() || (word == "No"))
        return false;
    if ((word == "Yes"))
        return true;
    reader.error("card.bad-value", std::string("meanStress") + " must be YES or NO, not \"" + std::string(word) + "\"",
                 "meanStress");
    return false;
}

} // namespace

std::optional<TimeToSpectrumCard> TimeToSpectrumFormat::read(FieldReader& reader)
{
    TimeToSpectrumCard card;
    card.id = reader.requiredInteger(1, "id");
    card.samplingRate = reader.requiredReal(2, "samplingRate");
    card.window = readKeyword(reader, 3, "window", TIME_TO_SPECTRUM_WINDOW_WORDS, TimeToSpectrumWindow::Hanning);
    card.format =
        readKeyword(reader, 4, "fileFormat", TIME_TO_SPECTRUM_FILE_FORMAT_WORDS, TimeToSpectrumFileFormat::Csv);
    card.meanStress = readMeanStress(reader);
    card.skipCount = reader.integer(6, "skipCount", 0);
    requireBlankLineFields(reader, 0, 8, 9, "first");
    readSections(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TimeToSpectrumFormat::check(const TimeToSpectrumCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.samplingRate, "samplingRate", RealRule::Positive))
        return problem;
    if (card.skipCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "skipCount"};
    if (card.events.empty())
        return CardProblem{"card.missing-field", "the card needs an EVENT section with at least one event", "eventIds"};
    for (std::size_t index = 0; index < card.events.size(); ++index)
    {
        const TimeToSpectrumEvent& event = card.events[index];
        if (const auto problem = checkRequiredId(event.eventId, "eventIds"))
            return problem;
        if (const auto problem = checkReal(event.length, "sliceTimes", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(event.overlap, "sliceDurations", RealRule::Finite))
            return problem;
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.events[earlier].eventId == event.eventId)
                return CardProblem{
                    "card.bad-value",
                    "an event is listed once: " + std::string(CardProblem::FIELD) + " " + std::to_string(event.eventId),
                    "eventIds"};
        }
    }
    for (const TimeToSpectrumDeletion& deletion : card.deletions)
    {
        const bool known = std::any_of(card.events.begin(), card.events.end(), [&](const TimeToSpectrumEvent& event)
                                       { return event.eventId == deletion.eventId; });
        if (!known)
            return CardProblem{
                "card.bad-value",
                "the event " + std::to_string(deletion.eventId) + " of a DELETE record is not in the EVENT section",
                "eventIds"};
        if (deletion.intervals.empty() || deletion.intervals.size() > static_cast<std::size_t>(MAX_DELETE_INTERVALS))
            return CardProblem{"card.bad-value", "a DELETE record has one to three pairs of times", "sliceStart"};
        double previousEnd = 0.0;
        for (const auto& [start, end] : deletion.intervals)
        {
            if (!(start > 0.0) || !std::isfinite(start) || !(end > 0.0) || !std::isfinite(end))
                return CardProblem{"card.bad-value", "the times of a DELETE record must be greater than zero",
                                   "sliceStart"};
            if (end < start || start < previousEnd)
                return CardProblem{"card.bad-value", "the times of a DELETE record must not decrease", "sliceStart"};
            previousEnd = end;
        }
    }
    int previousLoadCase = 0;
    int previousChannel = 0;
    for (const TimeToSpectrumChannel& channel : card.channels)
    {
        if (channel.loadCaseId <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "loadCaseIds"};
        if (channel.channel <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "channels"};
        if (channel.loadCaseId <= previousLoadCase)
            return CardProblem{"card.bad-value",
                               "the subcases " + std::string(CardProblem::FIELD) + " of a MAP must increase",
                               "loadCaseIds"};
        if (channel.channel <= previousChannel)
            return CardProblem{"card.bad-value",
                               "the channels " + std::string(CardProblem::FIELD) + " of a MAP must increase",
                               "channels"};
        previousLoadCase = channel.loadCaseId;
        previousChannel = channel.channel;
    }
    return std::nullopt;
}

void TimeToSpectrumFormat::write(const TimeToSpectrumCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.samplingRate);
    writeKeywordUnlessDefault(writer, 3, TIME_TO_SPECTRUM_WINDOW_WORDS, card.window, TimeToSpectrumWindow::Hanning);
    writeKeywordUnlessDefault(writer, 4, TIME_TO_SPECTRUM_FILE_FORMAT_WORDS, card.format,
                              TimeToSpectrumFileFormat::Csv);
    writer.textUnlessDefault(5, card.meanStress ? "Yes" : "No", "No");
    writer.integerUnlessDefault(6, card.skipCount, 0);
    int line = 1;
    for (std::size_t index = 0; index < card.events.size(); ++index)
    {
        const int eventLine = line + static_cast<int>(index);
        if (index == 0)
            writer.text(fatigueField(eventLine, 2), "Event");
        writer.integer(fatigueField(eventLine, EVENT_ID_FIELD), card.events[index].eventId);
        writer.real(fatigueField(eventLine, EVENT_LENGTH_FIELD), card.events[index].length);
        writer.realUnlessDefault(fatigueField(eventLine, EVENT_OVERLAP_FIELD), card.events[index].overlap, 0.0);
    }
    line += static_cast<int>(card.events.size());
    for (std::size_t index = 0; index < card.deletions.size(); ++index)
    {
        const TimeToSpectrumDeletion& deletion = card.deletions[index];
        const int deleteLine = line + static_cast<int>(index);
        if (index == 0)
            writer.text(fatigueField(deleteLine, 2), "Delete");
        writer.integer(fatigueField(deleteLine, 3), deletion.eventId);
        for (std::size_t interval = 0; interval < deletion.intervals.size(); ++interval)
        {
            const int startField = fatigueField(deleteLine, intervalFormatField(static_cast<int>(interval)));
            writer.real(startField, deletion.intervals[interval].first);
            writer.real(startField + 1, deletion.intervals[interval].second);
        }
    }
    line += static_cast<int>(card.deletions.size());
    if (!card.channels.empty())
    {
        writer.text(fatigueField(line, 2), "Map");
        for (std::size_t index = 0; index < card.channels.size(); ++index)
        {
            const int mapLine = line + static_cast<int>(index) / CHANNELS_PER_LINE;
            const int loadCaseField =
                fatigueField(mapLine, channelFormatField(static_cast<int>(index) % CHANNELS_PER_LINE));
            writer.integer(loadCaseField, card.channels[index].loadCaseId);
            writer.integer(loadCaseField + 1, card.channels[index].channel);
        }
    }
}

std::uint32_t TimeToSpectrumTable::append(const TimeToSpectrumCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TimeToSpectrumTable::store(std::uint32_t row, const TimeToSpectrumCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TimeToSpectrumTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TimeToSpectrumTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
