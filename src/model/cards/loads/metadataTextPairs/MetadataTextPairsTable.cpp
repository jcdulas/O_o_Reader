#include "oor/model/cards/loads/metadataTextPairs/MetadataTextPairsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/MetadataPairs.hpp"
#include "oor/model/cards/matrices/SpanTextRule.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_PAIR_LINE = 1;
constexpr int VALUE_FIELDS = 7; // 56 characters

} // namespace

std::optional<MetadataTextPairsCard> MetadataTextPairsFormat::read(FieldReader& reader)
{
    MetadataTextPairsCard card;
    card.metadataGroupId = reader.requiredInteger(1, "metadataGroupId");
    for (int field = 2; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "the first line of the card holds the " + std::string("metadataGroupId") + " only",
                         "entryName");
            break;
        }
    }
    // Each pair is a line: the NAME in its first field and the value over the next seven. A line of blank fields is
    // padding.
    const int last = reader.lastUsedField();
    for (int line = FIRST_PAIR_LINE; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        std::string value = reader.spanText(lineBase + 2, lineBase + 1 + VALUE_FIELDS);
        if (reader.isBlank(lineBase + 1))
        {
            if (!value.empty())
                reader.error("card.missing-field",
                             "a value needs the " + std::string("entryName") + " of its pair in the field before it",
                             "entryName");
            continue;
        }
        card.pairs.push_back(MetadataTextPairsPair{std::string(reader.text(lineBase + 1)), std::move(value)});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MetadataTextPairsFormat::check(const MetadataTextPairsCard& card)
{
    if (const auto problem = checkRequiredId(card.metadataGroupId, "metadataGroupId"))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("entryValue") + " pair is required",
                           "entryName"};
    for (const MetadataTextPairsPair& pair : card.pairs)
    {
        if (const auto problem = metadataNameProblem(pair.name))
            return problem;
        if (const auto problem = spanTextProblem(pair.value, "entryValue"))
            return problem;
    }
    return std::nullopt;
}

void MetadataTextPairsFormat::write(const MetadataTextPairsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.metadataGroupId);
    int line = FIRST_PAIR_LINE;
    for (const MetadataTextPairsPair& pair : card.pairs)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        writer.text(lineBase + 1, pair.name);
        writer.spanText(lineBase + 2, VALUE_FIELDS, pair.value);
        ++line;
    }
}

std::uint32_t MetadataTextPairsTable::append(const MetadataTextPairsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_metadataGroupId.push(card.metadataGroupId);
    m_cards.push(card);
    return row;
}

void MetadataTextPairsTable::store(std::uint32_t row, const MetadataTextPairsCard& card)
{
    m_metadataGroupId.set(row, card.metadataGroupId);
    m_cards.set(row, card);
}

void MetadataTextPairsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_metadataGroupId.keepRows(rows);
    m_cards.keepRows(rows);
}

void MetadataTextPairsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_metadataGroupId.set(row, id);
    m_cards.edit(row).metadataGroupId = id;
}

void MetadataTextPairsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the group the pairs belong to: one of the ID of the ABSTRCT cards. The column follows the card.
    MetadataTextPairsCard& card = m_cards.edit(row);
    visitMetadataGroup(visitor, card.metadataGroupId);
    m_metadataGroupId.set(row, card.metadataGroupId);
}

} // namespace oor::model
