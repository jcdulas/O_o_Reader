#include "oor/model/cards/solution_control/resultMetadata/ResultMetadataTable.hpp"

#include "oor/model/cards/matrices/SpanTextRule.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int METADATA_NAME_FIRST_FIELD = 1;
constexpr int METADATA_NAME_FIELDS = 3; // 24 characters

} // namespace

std::optional<ResultMetadataCard> ResultMetadataFormat::read(FieldReader& reader)
{
    ResultMetadataCard card;
    card.handle = reader.spanText(METADATA_NAME_FIRST_FIELD, METADATA_NAME_FIELDS);
    for (int field = METADATA_NAME_FIELDS + 1; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         std::string("metadataName") + " is the three fields after the name: field " +
                             std::to_string(field + 1) + " is blank in the layout",
                         "metadataName");
            break;
        }
    }
    // Each continuation line is one METAINFO, from its first to its last used field; a line that is blank in the middle
    // is an empty text.
    const int last = reader.lastUsedField();
    for (int line = 1; line * FIELDS_PER_LINE < last; ++line)
        card.lines.push_back(reader.spanText(line * FIELDS_PER_LINE + 1, (line + 1) * FIELDS_PER_LINE));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ResultMetadataFormat::check(const ResultMetadataCard& card)
{
    if (card.handle.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "metadataName"};
    if (const auto problem = spanTextProblem(card.handle, "metadataName"))
        return problem;
    for (const std::string& line : card.lines)
    {
        if (const auto problem = spanTextProblem(line, "metadataInfo"))
            return problem;
    }
    if (!card.lines.empty() && card.lines.back().empty())
        return CardProblem{
            "card.bad-value",
            "the last " + std::string(CardProblem::FIELD) + " cannot be blank: a blank line ends the card",
            "metadataInfo"};
    return std::nullopt;
}

void ResultMetadataFormat::write(const ResultMetadataCard& card, FieldWriter& writer)
{
    writer.spanText(METADATA_NAME_FIRST_FIELD, METADATA_NAME_FIELDS, card.handle);
    int line = 1;
    for (const std::string& text : card.lines)
        writer.spanText(line++ * FIELDS_PER_LINE + 1, FIELDS_PER_LINE, text);
}

std::uint32_t ResultMetadataTable::append(const ResultMetadataCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_handle.push(card.handle);
    m_cards.push(card);
    return row;
}

void ResultMetadataTable::store(std::uint32_t row, const ResultMetadataCard& card)
{
    m_handle.set(row, card.handle);
    m_cards.set(row, card);
}

void ResultMetadataTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_handle.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
