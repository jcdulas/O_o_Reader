#include "oor/model/cards/solution_control/userFileName/UserFileNameTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

namespace oor::model
{

namespace
{

// The name goes in the fields 2 to 9 of the continuation lines, from field 9 of the reader on.
constexpr int NAME_FIELD = 9;
constexpr int PIECE_WIDTH = 8; // the small format cuts the name in pieces of one field

} // namespace

std::optional<UserFileNameCard> UserFileNameFormat::read(FieldReader& reader)
{
    UserFileNameCard card;
    card.id = reader.requiredInteger(1, "id");
    // A name that starts on the first line (fields 3 to 9 of the format reference) is the same name; then every line of
    // fields 2 to 9 goes on with it. Each line is put together on its own, because the small format keeps the blanks
    // inside a line, and a name with a blank is refused by check().
    card.entryName = reader.spanText(2, FATIGUE_FIELDS_PER_LINE);
    const int last = reader.lastUsedField();
    for (int first = NAME_FIELD; first <= last; first += FATIGUE_FIELDS_PER_LINE)
        card.entryName += reader.spanText(first, first + FATIGUE_FIELDS_PER_LINE - 1);
    if (card.entryName.empty() && !reader.failed())
        reader.error("card.missing-field",
                     "the " + std::string("entryName") + " of a " + std::string(reader.cardName()) +
                         " is required: the name of the file",
                     "entryName");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserFileNameFormat::check(const UserFileNameCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkSpannedText(card.entryName, "entryName");
}

void UserFileNameFormat::write(const UserFileNameCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    const int pieces = static_cast<int>((card.entryName.size() + PIECE_WIDTH - 1) / PIECE_WIDTH);
    if (pieces > 0)
        writer.spanText(NAME_FIELD, pieces, card.entryName);
}

std::uint32_t UserFileNameTable::append(const UserFileNameCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserFileNameTable::store(std::uint32_t row, const UserFileNameCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserFileNameTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserFileNameTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
