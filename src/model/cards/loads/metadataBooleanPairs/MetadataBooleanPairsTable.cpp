#include "oor/model/cards/loads/metadataBooleanPairs/MetadataBooleanPairsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/MetadataPairs.hpp"

#include <array>
#include <string_view>

namespace oor::model
{

namespace
{

constexpr int BLANK_FIELD = 2;
constexpr int FIRST_PAIR_FIELD = 3;
constexpr int FIELDS_PER_PAIR = 2;

// FALSE is the first word, so the Boolean is the position of the word.
constexpr std::array<std::string_view, 2> BOOLEAN_WORDS{"False", "True"};

} // namespace

std::optional<MetadataBooleanPairsCard> MetadataBooleanPairsFormat::read(FieldReader& reader)
{
    MetadataBooleanPairsCard card;
    card.metadataGroupId = reader.requiredInteger(1, "metadataGroupId");
    if (!reader.isBlank(BLANK_FIELD))
        reader.error("card.bad-value", "field 3 of the card is blank: the first pair starts in field 4", "entryName");
    const int last = reader.lastUsedField();
    // The pairs sit side by side from field 3. A pair whose two fields are blank is padding.
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += FIELDS_PER_PAIR)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field",
                         "a value needs the " + std::string("entryName") + " of its pair in the field before it",
                         "entryName");
            break;
        }
        MetadataBooleanPairsPair pair;
        pair.name = std::string(reader.text(field));
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field", "the name " + pair.name + " needs its value in the next field",
                         "entryValue");
            break;
        }
        const std::optional<bool> value = parseKeyword<bool>(BOOLEAN_WORDS, reader.text(field + 1));
        if (!value)
        {
            reader.error("card.bad-value",
                         std::string("entryValue") + " must be TRUE or FALSE, not \"" +
                             std::string(reader.text(field + 1)) + "\"",
                         "entryValue");
            break;
        }
        pair.value = *value;
        card.pairs.push_back(std::move(pair));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MetadataBooleanPairsFormat::check(const MetadataBooleanPairsCard& card)
{
    if (const auto problem = checkRequiredId(card.metadataGroupId, "metadataGroupId"))
        return problem;
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("entryValue") + " pair is required",
                           "entryName"};
    for (const MetadataBooleanPairsPair& pair : card.pairs)
    {
        if (const auto problem = metadataNameProblem(pair.name))
            return problem;
    }
    return std::nullopt;
}

void MetadataBooleanPairsFormat::write(const MetadataBooleanPairsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.metadataGroupId);
    int field = FIRST_PAIR_FIELD;
    for (const MetadataBooleanPairsPair& pair : card.pairs)
    {
        writer.text(field, pair.name);
        writer.text(field + 1, BOOLEAN_WORDS[pair.value ? 1 : 0]);
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t MetadataBooleanPairsTable::append(const MetadataBooleanPairsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_metadataGroupId.push(card.metadataGroupId);
    m_cards.push(card);
    return row;
}

void MetadataBooleanPairsTable::store(std::uint32_t row, const MetadataBooleanPairsCard& card)
{
    m_metadataGroupId.set(row, card.metadataGroupId);
    m_cards.set(row, card);
}

void MetadataBooleanPairsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_metadataGroupId.keepRows(rows);
    m_cards.keepRows(rows);
}

void MetadataBooleanPairsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_metadataGroupId.set(row, id);
    m_cards.edit(row).metadataGroupId = id;
}

void MetadataBooleanPairsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the group the pairs belong to: one of the ID of the ABSTRCT cards. The column follows the card.
    MetadataBooleanPairsCard& card = m_cards.edit(row);
    visitMetadataGroup(visitor, card.metadataGroupId);
    m_metadataGroupId.set(row, card.metadataGroupId);
}

} // namespace oor::model
