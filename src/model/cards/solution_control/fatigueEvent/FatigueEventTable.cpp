#include "oor/model/cards/solution_control/fatigueEvent/FatigueEventTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

namespace oor::model
{

namespace
{

// FLOAD1 is format reference field 3 of the first line; a loads line has seven (fields 3 to 9).
constexpr int LOADS_PER_LINE = 7;
constexpr int FIRST_LOAD_FORMAT_FIELD = 3;

// The field of the reader where the load number `index` (from 0) is, and the line it is on.
int loadLine(std::size_t index) noexcept
{
    return index < LOADS_PER_LINE ? 0 : 1 + static_cast<int>((index - LOADS_PER_LINE) / LOADS_PER_LINE);
}

int loadField(std::size_t index) noexcept
{
    const std::size_t position = index < LOADS_PER_LINE ? index : (index - LOADS_PER_LINE) % LOADS_PER_LINE;
    return fatigueField(loadLine(index), FIRST_LOAD_FORMAT_FIELD + static_cast<int>(position));
}

} // namespace

std::optional<FatigueEventCard> FatigueEventFormat::read(FieldReader& reader)
{
    FatigueEventCard card;
    card.id = reader.requiredInteger(1, "id");
    const int lastLine = lastUsedLine(reader);
    for (int line = 0; line <= lastLine && !reader.failed(); ++line)
    {
        const std::string_view keyword = line == 0 ? std::string_view{} : reader.text(fatigueField(line, 2));
        if (!keyword.empty())
        {
            // The line of the event name: the keyword NAME and the name in fields 3 to 9, the last line of the card.
            if (!(keyword == "Name"))
            {
                reader.error("card.bad-value",
                             "field 2 of a continuation line of an " + std::string(reader.cardName()) +
                                 " is blank, or " + std::string("entryName"),
                             "entryName");
                break;
            }
            if (line != lastLine)
            {
                reader.error("card.bad-value",
                             "the " + std::string("entryName") + " line is the last line of an " +
                                 std::string(reader.cardName()),
                             "entryName");
                break;
            }
            card.entryName = reader.spanText(fatigueField(line, 3), fatigueField(line, 9));
            if (card.entryName.empty())
                reader.error("card.missing-field",
                             std::string("entryName") + " needs the event name, " + std::string("eventName"),
                             "eventName");
            break;
        }
        for (int formatField = FIRST_LOAD_FORMAT_FIELD; formatField <= 9; ++formatField)
        {
            const int field = fatigueField(line, formatField);
            if (!reader.isBlank(field))
                card.fatigueLoadIds.push_back(reader.requiredInteger(field, "fatigueLoadIds"));
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueEventFormat::check(const FatigueEventCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.fatigueLoadIds.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one " + CardProblem::cardNamed("FatigueLoad") + ", " +
                               std::string(CardProblem::FIELD),
                           "fatigueLoadIds"};
    for (const base::EntityId loadId : card.fatigueLoadIds)
    {
        if (const auto problem = checkRequiredId(loadId, "fatigueLoadIds"))
            return problem;
    }
    if (!card.entryName.empty())
        return checkSpannedText(card.entryName, "eventName");
    return std::nullopt;
}

void FatigueEventFormat::write(const FatigueEventCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.fatigueLoadIds.size(); ++index)
        writer.integer(loadField(index), card.fatigueLoadIds[index]);
    if (!card.entryName.empty())
    {
        // The name has the line after the last line of loads.
        const int nameLine = card.fatigueLoadIds.empty() ? 1 : loadLine(card.fatigueLoadIds.size() - 1) + 1;
        writer.text(fatigueField(nameLine, 2), "Name");
        writer.spanText(fatigueField(nameLine, 3), LOADS_PER_LINE, card.entryName);
    }
}

std::uint32_t FatigueEventTable::append(const FatigueEventCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueEventTable::store(std::uint32_t row, const FatigueEventCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueEventTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueEventTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueEventTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (base::EntityId& loadId : m_cards.edit(row).fatigueLoadIds)
        visitId(visitor, "fatigueLoadIds", ReferenceTarget::FatigueLoad, loadId);
}

} // namespace oor::model
